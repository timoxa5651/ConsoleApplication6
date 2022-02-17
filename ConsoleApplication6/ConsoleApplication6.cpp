#define FMT_HEADER_ONLY
#include "fmt/format.h"
#include "SFML/Graphics.hpp"
#include <string>
#include <iostream>
#include <cassert>
#include <random>
#include <chrono>
#include <any>
#include <unordered_set>
#include "list.h"

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

	int get_num(int st, bool read) {
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
			if (this->str[st] == '-')
				throw ReadExpection(this->off, "Only positive numbers are allowed");
			throw ReadExpection(this->off, "Number expected");
		}
		for (int i = st; rd; ++i, rd /= 10) {
			rs += (this->str[i] - '0') * rd;
			if (read) {
				this->off += 1;
			}
		}
		return rs;
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
};

enum class Op_Sign {
	Plus, Minus
};

class Polynomial {
	Polynomial(Stream source);
	~Polynomial();
	void try_parse();
	void simplify();
	Op_Sign parser_read_sign(bool first);

	Stream initial_stream;
public:
	List<Term*>* terms;

	string to_string();

	static Polynomial* parse(sf::String source);
};

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
				throw ReadExpection(this->initial_stream.get_cur(), "Term without power should be the last one");
			}

			char chr = this->initial_stream.read_char();
			if (chr < 'a' || chr > 'z') {
				throw ReadExpection(this->initial_stream.get_cur(), "Invalid char");
			}

			if (term->vars.find(chr) != term->vars.end()) {
				throw ReadExpection(this->initial_stream.get_cur(), string("\'") + chr + "\' appeared twice");
			}
			this->initial_stream.read_while(' '); // x       ^2...

			int num = 1;
			if (this->initial_stream.peek_char() == '^') {
				this->initial_stream.read_expect({ '^' }, "Expected \'^\'");

				this->initial_stream.read_while(' '); // x^       2...

				num = this->initial_stream.get_num(this->initial_stream.get_cur(), true);
			}
			else {
				had_last = true;
			}

			term->vars.insert({ chr, num });

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
		if (lhs->vars.empty())
			return true;
		else if (rhs->vars.empty())
			return false;
		auto mxa = max_element(lhs->vars.begin(), lhs->vars.end(), [](auto& p1, auto& p2) {return p1.second < p2.second; });
		auto mxb = max_element(rhs->vars.begin(), rhs->vars.end(), [](auto& p1, auto& p2) {return p1.second < p2.second; });
		if ((*mxa).second == (*mxb).second)
			return abs(lhs->coeff) < abs(rhs->coeff);
		return (*mxa).second < (*mxb).second;
	});
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
	Vector2f pos;
	Vector2f size;
	bool(*_Comp)(wchar_t);
	void(WndClass::*_Eval)(String);
	String name;
	WndClass* window;
public:
	String value;

	InputField(WndClass* wnd, String name, Vector2f pos, Vector2f size, bool(*_Comp)(wchar_t), void(WndClass::* _Eval)(String));
	void on_mousedown(Vector2f vec);
	void text_entered(Event evnt);
	void draw(RenderWindow* wnd);
};

class WndClass {
	RenderWindow* wnd;
	Vector2f size;

public:
	List<Polynomial*>* polys;
	vector<InputField*> activeFields;
	InputField* selectedField;

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
		return this->add_poly(str, false);
	}

	bool create_inputs() {
		auto all_allowed = [](wchar_t c) {
			return true;
		};

		this->activeFields.push_back(new InputField(this, String("Add polynomial"), Vector2f(320, 700), Vector2f(250, 30), all_allowed, &WndClass::add_poly_user));

		return true;
	}

	WndClass(RenderWindow* wnd, Vector2f size) {
		this->wnd = wnd;
		this->size = size;
		this->polys = new List<Polynomial*>();
		this->selectedField = 0;
		this->create_inputs();
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
				text.setCharacterSize(16);
				text.setFillColor(Color(180, 180, 180, 255));
				text.setPosition(Vector2f(field_pos));
				if(!idx)
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
		this->draw_poly_list();
		for (auto v : this->activeFields) {
			v->draw(this->wnd);
		}
			
	}


	void on_mousedown(Vector2f pos) {
		for (auto v : this->activeFields) {
			v->on_mousedown(pos);
		}
	}

	void on_mouseup(Vector2f pos) {

	}

	void text_entered(Event evnt) {
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
	text.setPosition(this->pos);
	if (this->value.getSize()) {
		text.setString(this->value);
	}
	else {
		text.setFillColor(Color(180, 180, 180, 200));
		text.setString(this->name);
	}
	wnd->draw(text);
}

void InputField::on_mousedown(Vector2f vec) {
	if (vec.x >= this->pos.x && vec.x <= this->pos.x + this->size.x && vec.y >= this->pos.y && vec.y <= this->pos.y + this->size.y) {
		window->selectedField = this;
	}
}
void InputField::text_entered(Event evnt) {
	if (this != window->selectedField)
		return;
	if(evnt.text.unicode == 13){
		(this->window->*_Eval)(this->value); //enter
		this->value = String();
	}
	else if (evnt.text.unicode == 8) { //backspace
		if(this->value.getSize())
			this->value = this->value.substring(0, this->value.getSize() - 1);
	}
	else {
		this->value += evnt.text.unicode;
	}
	//cout << evnt.text.unicode << endl;
}

int main()
{

	RenderWindow window(VideoMode(600, 900), "123");
	WndClass* wnd = new WndClass(&window, Vector2f(600, 900));

	g_Font.loadFromFile("arial.ttf");

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
