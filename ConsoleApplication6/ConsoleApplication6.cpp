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
				it = pn->prev;
				if (!it)
					it = this->terms->begin();
			}
		}
	}

	this->terms->Sort([](Term* lhs, Term* rhs) {
		if (lhs->vars.empty())
			return true;
		else if (rhs->vars.empty())
			return false;
		auto mxa = max_element(lhs->vars.begin(), lhs->vars.end(), [](auto& p1, auto& p2) {return p1.second > p2.second; });
		auto mxb = max_element(rhs->vars.begin(), rhs->vars.end(), [](auto& p1, auto& p2) {return p1.second > p2.second; });
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
		if (abs(it->data->coeff) == 1) {
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

class WndClass {
	RenderWindow* wnd;
	Vector2f size;

	List<Polynomial*>* polys;
public:
	WndClass(RenderWindow* wnd, Vector2f size) {
		this->wnd = wnd;
		this->size = size;
		this->polys = new List<Polynomial*>();
	}

	void draw_poly_list() {
		Vector2f field_size = Vector2f(200, this->size.y);
		Vector2f field_pos = Vector2f(0, 0);

		RectangleShape rect(field_size);
		rect.setPosition(field_pos);
		rect.setOutlineColor(Color(0, 0, 0, 255));
		rect.setOutlineThickness(1.f);
		rect.setFillColor(Color(90, 90, 90, 255));
		this->wnd->draw(rect);

		for(this->)
	}

	void frame() {
		this->draw_poly_list();
		Text text;
		text.setFont(g_Font);
		text.setCharacterSize(20);
		text.setFillColor(Color(200, 200, 200, 255));
		text.setString("[None]");
		text.setPosition(Vector2f(250 + 60, 150 + 400 + 70));

	}


	void on_mousedown(Vector2f pos) {
		
	}

	void on_mouseup(Vector2f pos) {

	}

	void text_entered(Event evnt) {
		
	}
};

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
