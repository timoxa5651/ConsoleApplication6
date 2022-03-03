#define FMT_HEADER_ONLY
#include "fmt/format.h"
#include "SFML/Graphics.hpp"
#include <string>
#include <iostream>
#include <cassert>
#include <random>
#include <chrono>
#include <fstream>
#include <any>
#include <unordered_set>
#include "list.h"
#include <set>

using namespace std;
using namespace sf;

Font g_Font;

class ReadExpection : exception {
public:
	int position;
	string msg;
	string fmsg;

	ReadExpection(int position, string message = "") {
		this->position = position;
		this->msg = message;
	}


	virtual const char* what() throw()
	{
		this->fmsg = this->msg + " at pos " + to_string(position + 1);
		return this->fmsg.c_str();
	}
};
class Stream {
	int off;
public:
	String str;
	Stream(String s = "") {
		this->str = s;
		this->off = 0;
	}

	int get_cur() {
		return this->off;
	}
	int get_size() {
		return this->str.getSize();
	}
	void set_cur(int v) {
		this->off = v;
	}
	void seek(int c) {
		this->off += c;
	}

	bool is_end() {
		return this->off >= this->str.getSize();
	}

	int read_expect(vector<String> val, string errmsg = "") {
		for (int i = 0; i < val.size(); ++i) {
			String& s = val[i];
			if (this->str.substring(this->off, s.getSize()) == s) {
				this->off += s.getSize();
				return i;
			}
		}
		throw ReadExpection(this->off, errmsg);
	}

	int peek_expect(vector<String> val, string errmsg = "") {
		for (int i = 0; i < val.size(); ++i) {
			String& s = val[i];
			if (this->str.substring(this->off, s.getSize()) == s) {
				return i;
			}
		}
		throw ReadExpection(this->off, errmsg);
	}

	void read_while(char chr) {
		while (this->str[this->off] == chr) {
			this->off += 1;
		}
	}
	wchar_t read_char() {
		return this->str[this->off++];
	}
	wchar_t peek_char() {
		return this->str[this->off];
	}

	String get_closing_block(int st, char add, char term, int cnt = 1) {
		bool flag = false;
		String rstr;
		for (int i = st; i < this->str.getSize(); ++i) {
			rstr += this->str[i];
			if (this->str[i] == term) {
				--cnt;
				flag = true;
			}
			else if (this->str[i] == add) {
				++cnt;
				flag = true;
			}

			if (flag && cnt <= 0) {
				break;
			}
		}
		return rstr;
	}

	String get_block(int st, char term) {
		String rstr;
		for (int i = st; i < this->str.getSize(); ++i) {
			rstr += this->str[i];
			if (this->str[i] == term)
				break;
		}
		return rstr;
	}

	String get_block(int st, vector<char> term, int* ch = 0) {
		String rstr;
		for (int i = st; i < this->str.getSize(); ++i) {
			bool flag = true;
			for (int j = 0; j < term.size(); ++j) {
				if (term[j] == this->str[i]) {
					flag = false;
					if (ch) {
						*ch = j;
					}
					break;
				}
			}
			rstr += this->str[i];
			if (!flag) {
				break;
			}
		}
		return rstr;
	}

	int get_num(int st, bool read, bool negat = false) {
		bool ng = this->str[st] == '-';
		if (!negat && ng)
			throw ReadExpection(this->off, "Only positive numbers are allowed");
		if (ng) {
			st += 1;
			if (read)
				this->off += 1;
		}

		int rd = 0, rs = 0;
		for (int i = st; i < this->str.getSize(); ++i) {
			if (this->str[i] >= '0' && this->str[i] <= '9') {
				rd = max(1, rd * 10);
			}
			else {
				break;
			}
		}
		if (!rd) {
			throw ReadExpection(this->off, "Number expected");
		}
		for (int i = st; rd; ++i, rd /= 10) {
			rs += (this->str[i] - '0') * rd;
			if (read) {
				this->off += 1;
			}
		}
		return rs * (ng ? -1 : 1);
	}
};

class Term {
public:
	double coeff;
	map<char, int> vars; // maps x^7 -> x : 7

	Term() {
		this->coeff = 0;
		this->vars = map<char, int>();
	}

	~Term() {

	}

	int Compare(Term* rhs) {
		if (this->vars.size() < rhs->vars.size()) {
			return -1;
		}
		else if (this->vars.size() > rhs->vars.size()) {
			return 1;
		}
		int cnt = this->vars.size();
		auto it = this->vars.begin();
		auto it2 = rhs->vars.begin();
		while (cnt--) {
			if ((*it).first < (*it2).first)
				return -1;
			else if ((*it).first > (*it2).first)
				return 1;

			if ((*it).second < (*it2).second)
				return -1;
			else if ((*it).second > (*it2).second)
				return 1;
			it++;
			it2++;
		}
		return 0;
	}
};

enum class Op_Sign {
	Plus, Minus
};

class Polynomial {
	void try_parse();
	void simplify();
	Op_Sign parser_read_sign(bool first);

	Stream initial_stream;

	Polynomial();
public:
	Polynomial(Stream source);
	~Polynomial();

	List<Term*>* terms;

	string to_string();

	static Polynomial* sum(Polynomial* lhs, Polynomial* rhs);
	static Polynomial* multiply(Polynomial* lhs, Polynomial* rhs);
	static Polynomial* parse(sf::String source);
};

Polynomial::Polynomial() {
	this->terms = new List<Term*>();
	this->initial_stream = String("");
}

Polynomial::Polynomial(Stream source) {
	this->terms = new List<Term*>();
	this->initial_stream = source;
}
Polynomial::~Polynomial() {
	if (this->terms) {
		delete this->terms;
		this->terms = nullptr;
	}
}

Polynomial* Polynomial::parse(sf::String source) {
	Polynomial* poly = new Polynomial(source);
	try {
		poly->try_parse();
	}
	catch (const ReadExpection& ex) {
		delete poly;
		throw;
	}
	catch (...) {
		delete poly;
		return nullptr;
	}
	poly->simplify();

	Node<Term*>* prev = poly->terms->begin();
	for (auto nxt = prev->next; nxt != nullptr; nxt = nxt->next) {
		int rs = prev->data->Compare(nxt->data);
		assert(rs == -1);
	}
	return poly;
}

void Polynomial::try_parse() {
	this->initial_stream.read_while(' ');

	while (this->initial_stream.get_cur() < this->initial_stream.get_size()) {
		unique_ptr<Term> term(new Term());

		bool first = this->terms->size == 0;
		Op_Sign next_sign = this->parser_read_sign(first);

		this->initial_stream.read_while(' ');

		bool had_coeff = false;
		int coeff = 1;
		try {
			coeff = this->initial_stream.get_num(this->initial_stream.get_cur(), true);
			had_coeff = true;
		}
		catch (ReadExpection& ex) {};
		if (next_sign == Op_Sign::Minus) {
			coeff *= -1;
		}
		this->initial_stream.read_while(' ');

		term->coeff = coeff;
		bool had_nums = false;
		bool had_last = false;
		while (this->initial_stream.get_cur() < this->initial_stream.get_size() && !(this->initial_stream.peek_char() == '-' || this->initial_stream.peek_char() == '+')) { // read chars
			this->initial_stream.read_while(' ');

			if (had_last) {
				char chr = this->initial_stream.peek_char();
				if (chr < 'a' || chr > 'z') {
					throw ReadExpection(this->initial_stream.get_cur(), "Invalid char");
				}
				//throw ReadExpection(this->initial_stream.get_cur(), "Term without power should be the last one");
			}

			char chr = this->initial_stream.peek_char();
			if (chr < 'a' || chr > 'z') {
				throw ReadExpection(this->initial_stream.get_cur(), "Invalid char");
			}

			int num = 1;
			this->initial_stream.read_char();
			this->initial_stream.read_while(' '); // x       ^2...

			if (this->initial_stream.peek_char() == '^') {
				this->initial_stream.read_expect({ '^' }, "Expected \'^\'");

				this->initial_stream.read_while(' '); // x^       2...

				num = this->initial_stream.get_num(this->initial_stream.get_cur(), true);
			}
			else {
				had_last = true;
			}

			auto it3 = term->vars.find(chr);
			if (it3 != term->vars.end()) {
				//throw ReadExpection(this->initial_stream.get_cur(), string("\'") + chr + "\' appeared twice");
				(*it3).second += num;
			}
			else {
				term->vars.insert({ chr, num });
			}

			this->initial_stream.read_while(' ');
			had_nums = true;
		}

		if (!had_coeff && !had_nums) {
			throw ReadExpection(this->initial_stream.get_cur(), "Term expected");
		}

		this->terms->InsertFirst(term.release());
	}

	if (!this->terms->size) {
		throw ReadExpection(0, "No terms found");
	}
}

void Polynomial::simplify() {
	for (auto it = this->terms->begin(); it != this->terms->end(); it = it->next) {
		for (auto it2 = this->terms->begin(); it2 != this->terms->end(); it2 = it2->next) {
			if (it2 == it)
				continue;
			bool flag = it->data->vars == it2->data->vars;
			if (flag) {
				it->data->coeff += it2->data->coeff;

				auto pnext = it2->next;
				this->terms->Delete(it2);
				it2 = pnext ? pnext->prev : nullptr;
				if (!it2)
					break;
			}
		}
	}

	bool has_nonzero = false;
	for (auto it = this->terms->begin(); it != this->terms->end(); it = it->next) {
		if (abs(it->data->coeff) > 0)
			has_nonzero = true;
	}
	for (auto it = this->terms->begin(); it != this->terms->end(); it = it->next) {
		if (it->data->coeff == 0) {
			it->data->vars.clear();
			if (has_nonzero) {
				auto pn = it->next;
				this->terms->Delete(it);
				if (!pn)
					break;
				it = pn->prev;
				if (!it)
					it = this->terms->begin();
			}
		}
	}

	for (auto it = this->terms->begin(); it != this->terms->end(); it = it->next) {
		bool flag = false;
		do {
			flag = false;
			for (auto it2 = it->data->vars.begin(); it2 != it->data->vars.end(); ++it2) {
				auto [d, p] = *it2;
				if (p == 0) {
					it->data->vars.erase(it2++);
					flag = true;
					break;
				}
			}
		} while (flag);
	}

	this->terms->Sort([](Term* lhs, Term* rhs) {
		int cmp = lhs->Compare(rhs);
		assert(cmp);
		if (cmp == 0)
			return abs(lhs->coeff) < abs(rhs->coeff);
		return (cmp == 1);
		});
}

Polynomial* Polynomial::sum(Polynomial* lhs, Polynomial* rhs) {
	Polynomial* result = new Polynomial();
	Node<Term*>* left = lhs->terms->begin();
	Node<Term*>* right = rhs->terms->begin();

	Node<Term*>* node = result->terms->begin();
	while (left || right) {
		Term* rsl = nullptr;
		bool flag = false;
		Node<Term*>* next = nullptr;
		if (!left) {
			next = right;
			right = right->next;
		}
		else if (!right) {
			next = left;
			left = left->next;
		}
		else {
			int cmp = left->data->Compare(right->data);
			if (cmp == -1) {
				next = left;
				left = left->next;
			}
			else if (cmp == 1) {
				next = right;
				right = right->next;
			}
			else {
				if (right->data->coeff + left->data->coeff != 0) {
					rsl = new Term();
					rsl->vars = right->data->vars;
					rsl->coeff = right->data->coeff + left->data->coeff;
				}
				right = right->next;
				left = left->next;
				flag = true;
			}
		}

		if (!flag) {
			if (node) {
				node = result->terms->InsertAfter(node, new Term());
			}
			else {
				node = result->terms->InsertFirst(new Term());
			}

			node->data->coeff = next->data->coeff;
			node->data->vars = next->data->vars;
		}
		else if (rsl) {
			if (node) {
				node = result->terms->InsertAfter(node, rsl);
			}
			else {
				node = result->terms->InsertFirst(rsl);
			}
		}
	}

	if (!result->terms->size)
		result->terms->InsertFirst(new Term());
	return result;
}

Polynomial* Polynomial::multiply(Polynomial* lhs, Polynomial* rhs) {
	Polynomial* result2 = new Polynomial();

	for (auto t1 = lhs->terms->begin(); t1; t1 = t1->next) {
		for (auto t2 = rhs->terms->begin(); t2; t2 = t2->next) {
			Term* result = new Term();
			result->coeff = t1->data->coeff * t2->data->coeff;
			result->vars = t1->data->vars;
			for (auto& [p, v] : t2->data->vars)
				result->vars[p] += v;
			result2->terms->InsertFirst(result);
		}
	}

	result2->simplify();
	return result2;
}

string Polynomial::to_string() {
	string s = "";
	for (auto it = this->terms->begin(); it != this->terms->end(); it = it->next) {
		char buffer[128];
		sprintf_s(buffer, "%g", abs(it->data->coeff));
		string impl = string(buffer);
		if (abs(it->data->coeff) == 1 && !it->data->vars.empty()) {
			impl = "";
		}

		vector<pair<char, int>> smp;

		for (auto [d, p] : it->data->vars) {
			smp.push_back({ d, p });
		}
		sort(smp.begin(), smp.end(), [](auto& p1, auto& p2) { return p1.second > p2.second; });
		for (auto [d, p] : smp) {
			if (p == 1) {
				impl += (char)d;
			}
			else {
				impl += (char)d + string("^") + std::to_string(p);
			}
		}

		if (it->prev) {
			if (it->data->coeff < 0) {
				s += " - " + impl;
			}
			else {
				s += " + " + impl;
			}
		}
		else {
			if (it->data->coeff < 0) {
				s += "-" + impl;
			}
			else {
				s += impl;
			}
		}
	}
	return s;
}

Op_Sign Polynomial::parser_read_sign(bool first) {
	if (first) {
		do {
			try {
				int vl = this->initial_stream.read_expect({ '-', ' ' });
				if (vl == 0)
					return Op_Sign::Minus;
			}
			catch (ReadExpection& ex) {
				return Op_Sign::Plus;
			}
		} while (this->initial_stream.get_cur() < this->initial_stream.get_size());
	}
	else {
		do {
			int vl = this->initial_stream.read_expect({ '+', '-', ' ' });
			if (vl == 0)
				return Op_Sign::Plus;
			else
				return Op_Sign::Minus;
		} while (this->initial_stream.get_cur() < this->initial_stream.get_size());
	}
	throw ReadExpection(this->initial_stream.get_cur(), "Generic error");
}

class WndClass;
class InputField {
	bool(*_Comp)(wchar_t);
	void(WndClass::* _Eval)(String);
public:
	String name;
	Vector2f pos;
	Vector2f size;
	void(*_Upd)(InputField*);
	WndClass* window;
	String value;
	String lastDrawValue;

	InputField(WndClass* wnd, String name, Vector2f pos, Vector2f size, bool(*_Comp)(wchar_t), void(WndClass::* _Eval)(String));
	void on_mousedown(Vector2f vec);
	void text_entered(Event evnt);
	void draw(RenderWindow* wnd);
};

class InputFieldCollection {
public:
	vector<InputField*> fields;
	String desc;
	void(WndClass::* _Send)(vector<String>&);

	InputFieldCollection(vector<InputField*> vec, void(WndClass::* call)(vector<String>&), String desc) {
		this->fields = vec;
		this->_Send = call;
		this->desc = desc;
	}
	InputFieldCollection(InputField* fl) {
		this->fields.push_back(fl);
		this->_Send = 0;
	}

	void on_mousedown(Vector2f vec) {
		for (auto a : fields)
			a->on_mousedown(vec);
	}
	void text_entered(Event evnt);
	void draw(RenderWindow* wnd);
};

struct _EvalState {
	Polynomial* poly;
	map<char, int> vals;
	char chur;
};

template<typename T = void*>
class BlockWindow {
	static_assert(is_pointer_v<T>);
public:
	String text;
	bool(*_Call)(BlockWindow*, T, int);
	Vector2f pos;
	T arg0;
	WndClass* wnd;
	void(*_Draw)(BlockWindow*, RenderWindow*);
	vector<InputField*> fields;

	BlockWindow(WndClass* wnd, String str, bool(*_Call)(BlockWindow*, T, int), void(*_Draw)(BlockWindow*, RenderWindow*), T arg) {
		this->text = str;
		this->_Call = _Call;
		this->_Draw = _Draw;
		this->arg0 = arg;
		this->wnd = wnd;
		this->pos = Vector2f(0, 0);
	};

	void draw(RenderWindow* wnd) {
		RectangleShape rect(Vector2f(wnd->getSize().x, wnd->getSize().y));
		rect.setPosition(pos);
		rect.setOutlineColor(Color(0, 0, 0, 255));
		rect.setOutlineThickness(1.f);
		rect.setFillColor(Color(255, 255, 255, 255));
		wnd->draw(rect);

		for (auto a : this->fields)
			a->draw(wnd);
		if (_Draw)
			return _Draw(this, wnd);
		Text text;
		text.setFont(g_Font);
		text.setCharacterSize(16);
		text.setFillColor(Color(0, 0, 0, 255));
		text.setString(this->text + "\n\n\nPress Y to save it, N to delete it");
		text.setPosition(this->pos);
		wnd->draw(text);
	}
	void on_mousedown(Vector2f vec2) {
		for (auto a : this->fields)
			a->on_mousedown(vec2);
	}

	void text_entered(Event evnt) {
		for (auto a : this->fields)
			a->text_entered(evnt);
		if (tolower(evnt.text.unicode) == L'y' || evnt.text.unicode == 13) {
			if(this->_Call(this, this->arg0, 1))
				this->_Call = 0;
		}
		else if(tolower(evnt.text.unicode) == L'n') {
			if(this->_Call(this, this->arg0, 0))
				this->_Call = 0;
		}
	}
};

class WndClass {
	friend class InputFieldCollection;
	friend class InputField;

	Vector2f size;
	RenderWindow* wnd;
	InputField* selectedField;
	vector<InputFieldCollection*> activeFields;
public:
	BlockWindow<>* blockWindow;
	List<Polynomial*>* polys;

	void add_poly(String str, bool silent) {
		try {
			Polynomial* poly = Polynomial::parse(str);
			this->polys->InsertLast(poly);
		}
		catch (ReadExpection& ex) {
			cout << ex.what() << endl;
		}
	}

	void add_poly_user(String str) {
		return this->add_poly(str, true);
	}
	void add_file_user(String str) {
		ifstream stream(str.toAnsiString());
		if (!stream.good())
			return;
		string line;
		while (getline(stream, line)) {
			this->add_poly_user(line);
		}
		stream.close();
	}
	void save_file_user(String str) {
		ofstream stream(str.toAnsiString());
		if (!stream.good())
			return;
		for (auto it = this->polys->begin(); it; it = it->next) {
			stream << it->data->to_string() << endl;
		}
		stream.close();
	}
	void del_poly_user(String str) {
		try {
			int num = Stream(str).get_num(0, true);
			if (num < 1 || num > this->polys->size) {
				throw ReadExpection(0, "Invalid input");
			}
			this->polys->Delete(this->polys->operator[](num - 1));
		}
		catch (ReadExpection& ex) {

		}
	}

	void sum_user(vector<String>& str) {
		try {
			int num1 = Stream(str[0]).get_num(0, false);
			int num2 = Stream(str[1]).get_num(0, false);
			if (num1 < 1 || num1 > this->polys->size)
				throw ReadExpection(0, "Invalid arg1");
			if (num2 < 1 || num2 > this->polys->size)
				throw ReadExpection(0, "Invalid arg2");

			try {
				auto fs = this->polys->operator[](num1 - 1)->data;
				auto sc = this->polys->operator[](num2 - 1)->data;
				Polynomial* poly2 = Polynomial::sum(fs, sc);
				String str = "Sum of \n   " + fs->to_string() + "\nAND\n   " + sc->to_string() + "\nEQUALS\n   " + poly2->to_string();
				this->blockWindow = new BlockWindow<>(this, str, [](BlockWindow<>* blockWindow, void* ptr, int rs) {
					Polynomial* pl = (Polynomial*)ptr;
					if (rs > 0) {
						blockWindow->wnd->polys->InsertLast(pl);
					}
					else {
						delete pl;
					}
					return true;
				}, 0, poly2);
			}
			catch (ReadExpection& ex) {
				throw;
			}
		}
		catch (ReadExpection& ex) {
			cout << "Error: " << ex.what() << endl;
		}
	}

	void multiply_user(vector<String>& str) {
		try {
			int num1 = Stream(str[0]).get_num(0, false);
			int num2 = Stream(str[1]).get_num(0, false);
			if (num1 < 1 || num1 > this->polys->size)
				throw ReadExpection(0, "Invalid arg1");
			if (num2 < 1 || num2 > this->polys->size)
				throw ReadExpection(0, "Invalid arg2");
			auto fs = this->polys->operator[](num1 - 1)->data;
			auto sc = this->polys->operator[](num2 - 1)->data;
			Polynomial* poly2 = Polynomial::multiply(fs, sc);
			String str = "Multiplication of \n   " + fs->to_string() + "\nAND\n   " + sc->to_string() + "\nEQUALS\n   " + poly2->to_string();
			this->blockWindow = new BlockWindow<>(this, str, [](BlockWindow<>* blockWindow, void* ptr, int rs) {
				Polynomial* pl = (Polynomial*)ptr;
				if (rs > 0) {
					blockWindow->wnd->polys->InsertLast(pl);
				}
				else {
					delete pl;
				}
				return true;
			}, 0, poly2);
		}
		catch (ReadExpection& ex) {
			cout << "Error: " << ex.what() << endl;
		}
	}

	void evaluate_user(vector<String>& str) {
		try {
			int num1 = Stream(str[0]).get_num(0, false);
			if (num1 < 1 || num1 > this->polys->size)
				throw ReadExpection(0, "Invalid arg1");
			_EvalState* state = new _EvalState();
			state->poly = this->polys->operator[](num1 - 1)->data;
			set<char> pls;
			for (auto t1 = state->poly->terms->begin(); t1; t1 = t1->next) {
				for (auto& [p, v] : t1->data->vars)
					pls.insert(p);
			}
			if (!pls.size()) {
				state->chur = '\0';
			}
			else {
				state->chur = *pls.begin();
			}

			this->blockWindow = new BlockWindow<>(this, "", [](BlockWindow<>* blockWindow, void* ptr, int rs) {
				_EvalState* pl = (_EvalState*)ptr;
				if (!pl->chur) {
					delete pl;
					return true;
				}

				set<char> pls;
				for (auto t1 = pl->poly->terms->begin(); t1; t1 = t1->next) {
					for (auto& [p, v] : t1->data->vars)
						pls.insert(p);
				}

				bool flag3 = 0;
				try {
					auto str2 = Stream(blockWindow->fields[0]->lastDrawValue);
					int jk = str2.get_num(0, 1, true);
					if (str2.get_cur() < str2.get_size())
						throw 0;
					pl->vals[pl->chur] = jk;
					flag3 = 1;
				}
				catch (...) {};
				if (!flag3)
					return false;

				bool flag = 0, flag2 = 0;
				for (auto& p : pls) {
					if (flag) {
						pl->chur = p;
						flag2 = 1;
						break;
					}
					if (p == pl->chur)
						flag = 1;
				}

				if (!flag2) {
					pl->chur = 0;
					delete blockWindow->fields[0];
					blockWindow->fields.clear();
				}
				return false;
				}, [](BlockWindow<>* blockWindow, RenderWindow* wnd) {
					_EvalState* pl = (_EvalState*)blockWindow->arg0;

					Text text;
					text.setFont(g_Font);
					text.setCharacterSize(16);
					text.setFillColor(Color(0, 0, 0, 255));
					if(pl->chur)
						text.setString("\nEnter value for variable \'" + String(pl->chur) + "\'");
					else {
						double result = 0;
						for (auto t1 = pl->poly->terms->begin(); t1; t1 = t1->next) {
							double psum = 1;
							for (auto& [p, v] : t1->data->vars) {
								psum *= pow(pl->vals[p], v);
							}
							result += psum * t1->data->coeff;
						}
						text.setString("\nResult is " + to_string(result));
					}
					text.setPosition(blockWindow->pos);
					wnd->draw(text);
				}, state);

			auto numbers_only = [](wchar_t c) -> bool {
				return (c >= L'0' && c <= L'9') || c == L'-';
			};
			this->blockWindow->fields.push_back(new InputField(this, "Value", Vector2f(140, 100), Vector2f(130, 25), numbers_only, 0));
		}
		catch (ReadExpection& ex) {
			cout << "Error: " << ex.what() << endl;
		}
	}

	bool create_inputs() {
		auto numbers_only = [](wchar_t c) -> bool {
			return c >= L'0' && c <= L'9';
		};
		// SHRUG

		this->activeFields.push_back(new InputFieldCollection({
			new InputField(this, String("First"), Vector2f(440, 100), Vector2f(70, 25), numbers_only, nullptr),
			new InputField(this, String("Second"), Vector2f(520, 100), Vector2f(70, 25), numbers_only, nullptr)
		}, &WndClass::sum_user, "Sum polynoms"));
		this->activeFields.push_back(new InputFieldCollection({
			new InputField(this, String("First"), Vector2f(440, 140), Vector2f(70, 25), numbers_only, nullptr),
			new InputField(this, String("Second"), Vector2f(520, 140), Vector2f(70, 25), numbers_only, nullptr)
		}, &WndClass::multiply_user, "Multiply polynoms"));
		this->activeFields.push_back(new InputFieldCollection({
			new InputField(this, String("Index"), Vector2f(440, 180), Vector2f(70, 25), numbers_only, nullptr),
		}, &WndClass::evaluate_user, "Evaluate"));

		this->activeFields.push_back(new InputFieldCollection(new InputField(this, String("Save to file"), Vector2f(320, 540), Vector2f(250, 30), nullptr, &WndClass::save_file_user)));
		this->activeFields.push_back(new InputFieldCollection(new InputField(this, String("Add from file"), Vector2f(320, 620), Vector2f(250, 30), nullptr, &WndClass::add_file_user)));
		this->activeFields[this->activeFields.size() - 1]->fields[0]->_Upd = [](InputField* field) {
			try {
				ifstream str(field->value.toAnsiString());
				if (!str.good())
					throw 0;
				str.close();
				Text text2;
				text2.setFont(g_Font);
				text2.setCharacterSize(12);
				text2.setString("Good");
				text2.setFillColor(Color(0, 128, 0, 255));
				text2.setPosition(field->pos + Vector2f(field->size.x - text2.getLocalBounds().width, field->size.y + 5));
				field->window->wnd->draw(text2);
			}
			catch (...) {
				if (field->value.getSize()) {
					Text text2;
					text2.setFont(g_Font);
					text2.setCharacterSize(12);
					text2.setString("File error");
					text2.setFillColor(Color(255, 0, 0, 255));
					text2.setPosition(field->pos + Vector2f(field->size.x - text2.getLocalBounds().width, field->size.y + 5));
					field->window->wnd->draw(text2);
				}
			}
		};

		this->activeFields.push_back(new InputFieldCollection(new InputField(this, String("Add polynomial"), Vector2f(320, 700), Vector2f(250, 30), nullptr, &WndClass::add_poly_user)));
		this->activeFields[this->activeFields.size() - 1]->fields[0]->_Upd = [](InputField* field) {
			try {
				Polynomial* poly = Polynomial::parse(field->value);
				if (!poly)
					throw ReadExpection(field->value.getSize(), "Unknown error");
				delete poly;
			}
			catch (ReadExpection& ex) {
				if (ex.position >= field->value.getSize())
					ex.position = field->value.getSize() - 1;
				bool shouldMark = field->lastDrawValue.getSize() > field->value.getSize() - ex.position - 1;
				if (shouldMark) {
					Text text;
					text.setFont(g_Font);
					text.setCharacterSize(16);
					text.setString(field->lastDrawValue);

					int tp = field->lastDrawValue.getSize() - (field->value.getSize() - ex.position - 1) - 1;
					Vector2f markPos = text.findCharacterPos(tp);
					Vector2f markSize = (tp < field->lastDrawValue.getSize() ? text.findCharacterPos(tp + 1) : Vector2f()) - markPos;

					RectangleShape rect(Vector2f(markSize.x, 16));
					rect.setPosition(field->pos + Vector2f(field->size.x - text.getLocalBounds().width, 0) + markPos);
					rect.setFillColor(Color(255, 0, 0, 255 * 0.5));
					field->window->wnd->draw(rect);

					Text text2;
					text2.setFont(g_Font);
					text2.setCharacterSize(12);
					text2.setString(ex.what());
					text2.setFillColor(Color(0, 0, 0, 255));
					text2.setPosition(field->pos + Vector2f(field->size.x - text2.getLocalBounds().width + 30, field->size.y + 20));
					field->window->wnd->draw(text2);
				}
			}
		};

		this->activeFields.push_back(new InputFieldCollection(new InputField(this, String("Delete"), Vector2f(320, 740), Vector2f(80, 30), numbers_only, &WndClass::del_poly_user)));
		return true;
	}

	WndClass(RenderWindow* wnd, Vector2f size) {
		this->wnd = wnd;
		this->size = size;
		this->polys = new List<Polynomial*>();
		this->selectedField = 0;
		this->blockWindow = nullptr;
		this->create_inputs();
	}
	~WndClass() {
		if (this->polys)
			delete this->polys;
		this->polys = nullptr;
		for (auto& a : this->activeFields)
			delete a;
		this->activeFields.clear();
	}

	void draw_poly_list() {
		Vector2f field_size = Vector2f(300, this->size.y);
		Vector2f field_pos = Vector2f(0, 0);

		RectangleShape rect(field_size);
		rect.setPosition(field_pos);
		rect.setOutlineColor(Color(0, 0, 0, 255));
		rect.setOutlineThickness(1.f);
		rect.setFillColor(Color(90, 90, 90, 255));
		this->wnd->draw(rect);

		int cnt = 1;
		for (auto it = this->polys->begin(); it != nullptr; it = it->next) {
			String str = it->data->to_string();
			int idx = 0;

			while (idx < str.getSize()) {
				Text text;
				text.setFont(g_Font);
				text.setCharacterSize(18);
				text.setFillColor(Color(255, 255, 255, 255));
				text.setPosition(Vector2f(field_pos));
				if (!idx)
					text.setString(to_string(cnt) + ". ");
				while (idx < str.getSize() && text.getLocalBounds().width + 8 < field_size.x) {
					text.setString(text.getString() + str[idx]);
					++idx;
				}
				this->wnd->draw(text);
				field_pos.y += text.getLocalBounds().height + 3;
			}
			field_pos.y += 5;

			RectangleShape rect2(Vector2f(field_size.x, 0));
			rect2.setPosition(field_pos);
			rect2.setOutlineThickness(1.f);
			rect2.setFillColor(Color(180, 180, 180, 255));
			this->wnd->draw(rect2);

			++cnt;
		}
	}

	void frame() {
		if (this->blockWindow && !this->blockWindow->_Call) {
			delete this->blockWindow;
			this->blockWindow = 0;
		}
		this->draw_poly_list();
		for (auto v : this->activeFields) {
			v->draw(this->wnd);
		}
		if (this->blockWindow)
			this->blockWindow->draw(this->wnd);
	}

	void on_mousedown(Vector2f pos) {
		if (this->blockWindow) {
			return this->blockWindow->on_mousedown(pos);
		}

		for (auto v : this->activeFields) {
			v->on_mousedown(pos);
		}
	}

	void on_mouseup(Vector2f pos) {

	}

	void text_entered(Event evnt) {
		if (this->blockWindow) {
			return this->blockWindow->text_entered(evnt);
		}

		for (auto v : this->activeFields) {
			v->text_entered(evnt);
		}
	}
};

InputField::InputField(WndClass* wnd, String name, Vector2f pos, Vector2f size, bool(*_Comp)(wchar_t), void(WndClass::* _Eval)(String)) {
	this->pos = pos;
	this->size = size;
	this->_Comp = _Comp;
	this->_Eval = _Eval;
	this->name = name;
	this->window = wnd;
}

void InputFieldCollection::draw(RenderWindow* wnd) {
	for (auto a : fields)
		a->draw(wnd);
	if (this->desc.getSize()) {
		Text text;
		text.setFont(g_Font);
		text.setCharacterSize(16);
		text.setFillColor(Color(0, 0, 0, 255));
		text.setString(this->desc);
		text.setPosition(this->fields[0]->pos + Vector2f(-text.getLocalBounds().width - 15, 0));
		this->fields[0]->window->wnd->draw(text);
	}
}

void InputFieldCollection::text_entered(Event evnt) {
	if (this->_Send && evnt.text.unicode == 13) {
		bool flag = false;
		for (auto a : fields)
			flag = flag || a == a->window->selectedField;
		if (flag) {
			vector<String> str;
			for (auto a : fields)
				str.push_back(a->value);
			(this->fields[0]->window->*_Send)(str);
			for (auto a : fields)
				a->value = String();
			return;
		}
	}

	for (auto a : fields)
		a->text_entered(evnt);
}

void InputField::draw(RenderWindow* wnd) {
	RectangleShape rect(size);
	rect.setPosition(pos);
	rect.setOutlineColor(Color(0, 0, 0, 255));
	rect.setOutlineThickness(1.f);
	rect.setFillColor(Color(255, 255, 255, 255));
	wnd->draw(rect);

	Text text;
	text.setFont(g_Font);
	text.setCharacterSize(16);
	text.setFillColor(Color(0, 0, 0, 255));
	if (this->value.getSize()) {
		int cnt = this->value.getSize() - 1;
		while (cnt >= 0 && text.getLocalBounds().width + 10 < size.x) {
			text.setString(this->value[cnt--] + text.getString());
		}
		this->lastDrawValue = text.getString();
	}
	else {
		text.setFillColor(Color(180, 180, 180, 200));
		text.setString(this->name);
		this->lastDrawValue = "";
	}
	text.setPosition(this->pos + Vector2f(this->size.x - text.getLocalBounds().width, 0));
	wnd->draw(text);

	if (this->_Upd)
		this->_Upd(this);
}

void InputField::on_mousedown(Vector2f vec) {
	if (vec.x >= this->pos.x && vec.x <= this->pos.x + this->size.x && vec.y >= this->pos.y && vec.y <= this->pos.y + this->size.y) {
		window->selectedField = this;
	}
}
void InputField::text_entered(Event evnt) {
	if (this != window->selectedField)
		return;
	if (evnt.text.unicode == 13) {
		if(this->_Eval)
			(this->window->*_Eval)(this->value); //enter
		this->value = String();
	}
	else if (evnt.text.unicode == 8) { //backspace
		if (this->value.getSize())
			this->value = this->value.substring(0, this->value.getSize() - 1);
	}
	else if (!this->_Comp || this->_Comp(evnt.text.unicode)) {
		this->value += evnt.text.unicode;
	}
	//cout << evnt.text.unicode << endl;
}

int main()
{
	g_Font.loadFromFile("arial.ttf");
	static RenderWindow window(VideoMode(600, 900), "123");
	WndClass* wnd = new WndClass(&window, Vector2f(600, 900));
	while (window.isOpen())
	{
		Event event;
		while (window.pollEvent(event))
		{
			if (event.type == Event::Closed) {
				window.close();
			}
			else if (event.type == Event::MouseButtonPressed) {
				if (event.mouseButton.button == 0) {
					wnd->on_mousedown(Vector2f(event.mouseButton.x, event.mouseButton.y));
				}
			}
			else if (event.type == Event::MouseButtonReleased) {
				if (event.mouseButton.button == 0) {
					wnd->on_mouseup(Vector2f(event.mouseButton.x, event.mouseButton.y));
				}
			}
			else if (event.type == Event::TextEntered) {
				wnd->text_entered(event);
			}
		}
		window.clear(Color(200, 200, 200, 255));

		wnd->frame();

		window.display();
	}

	delete wnd;
	return 0;
}
