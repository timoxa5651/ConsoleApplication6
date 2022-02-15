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
	Op_Sign parser_read_sign(bool first);

	Stream initial_stream;
public:	
	List<Term*>* terms;

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

		term->coeff = coeff;
		bool had_nums = false;
		while (this->initial_stream.get_cur() < this->initial_stream.get_size() && !(this->initial_stream.peek_char() == '-' || this->initial_stream.peek_char() == '+')) { // read chars
			this->initial_stream.read_while(' ');

			char chr = this->initial_stream.read_char();
			if (chr < 'a' || chr > 'z') {
				throw ReadExpection(this->initial_stream.get_cur(), "Invalid char");
			}

			if (term->vars.find(chr) != term->vars.end()) {
				throw ReadExpection(this->initial_stream.get_cur(), string("\'") + chr + "\' appeared twice");
			}
			this->initial_stream.read_while(' '); // x       ^2...

			this->initial_stream.read_expect({ '^' }, "Expected \'^\'");

			this->initial_stream.read_while(' '); // x^       2...

			int num = this->initial_stream.get_num(this->initial_stream.get_cur(), true);
			term->vars.insert({ chr, num });

			this->initial_stream.read_while(' ');
			had_nums = true;
		}

		if (!had_coeff && !had_nums) {
			throw ReadExpection(this->initial_stream.get_cur(), "Term expected");
		}

		this->terms->InsertFirst(term.release());
	}
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

	String current_text;
	bool button_state;

public:
	WndClass(RenderWindow* wnd, Vector2f size) {
		this->wnd = wnd;
		this->size = size;
		this->button_state = 0;
	}

	void do_buttons() {
		Vector2f button_pos = Vector2f(250, 150);
		Vector2f button_size = Vector2f(25, 25);

		Text text;
		text.setFont(g_Font);
		text.setCharacterSize(18);
		text.setFillColor(Color(255, 255, 255, 255));
		text.setString("Disabled = 1st, Enabled = 2nd");
		text.setPosition(button_pos + Vector2f(button_size.x * 3, 0));
		this->wnd->draw(text);

		RectangleShape rect(button_size);
		rect.setPosition(button_pos);
		rect.setOutlineColor(Color(0, 0, 0, 255));
		rect.setOutlineThickness(2.f);
		rect.setFillColor(Color(90, 90, 90, 255));
		this->wnd->draw(rect);
		if (this->button_state) {
			RectangleShape rect(button_size / 2.f);
			rect.setPosition(button_pos + button_size / 4.f);
			rect.setFillColor(Color(0, 0, 0, 255));
			this->wnd->draw(rect);
		}
	}

	void frame() {
		this->do_buttons();

		Vector2f field_size = Vector2f(500, 400);
		Vector2f field_pos = Vector2(this->size.x / 2 - field_size.x / 2, this->size.y / 2 - field_size.y / 2);

		RectangleShape rect(field_size);
		rect.setPosition(field_pos);
		rect.setOutlineColor(Color(0, 0, 0, 255));
		rect.setOutlineThickness(1.f);
		rect.setFillColor(Color(90, 90, 90, 255));
		this->wnd->draw(rect);

		Text text;
		text.setFont(g_Font);
		text.setCharacterSize(20);
		text.setFillColor(Color(200, 200, 200, 255));
		text.setString("[None]");
		text.setPosition(Vector2f(250 + 60, 150 + 400 + 70));


	}


	void on_mousedown(Vector2f pos) {
		Vector2f button_pos = Vector2f(250, 150);
		Vector2f button_size = Vector2f(25, 25);
		if (Rect(button_pos, button_size).contains(pos)) {
			this->button_state = !this->button_state;
		}
	}

	void on_mouseup(Vector2f pos) {

	}

	void text_entered(Event evnt) {
		if (evnt.text.unicode == 8) { // backspace
			this->current_text = this->current_text.substring(0, this->current_text.getSize() - 1);
		}
		else if (evnt.text.unicode) {
			this->current_text += evnt.text.unicode;
		}
	}
};

int main()
{
	String tests[] = {
		String("-9+17x^3z^9-19z^2+0-x^2"), // 
	};
	int num = 1;
	for (String s : tests) {
		try {
			Polynomial* nom = Polynomial::parse(s);
			cout << "Test " << num << ": OK (" << nom->terms->size << ")" << endl;
			int idx = 1;
			for (auto it = nom->terms->begin(); it != nullptr; it = it->next) {
				cout << "   Term " << idx++ << endl;
				cout << "   --- Coeff " << it->data->coeff << endl;

				for (auto [d, p] : it->data->vars) {
					cout << "   --- " << d << " : " << p << endl;
				}
			}
		}
		catch (ReadExpection& ex) {
			cout << "Test " << num << ": Fail (" << ex.what() << ")" << endl;
		}
		catch (...) {
			cout << "Test " << num << ": Fail (Generic)" << endl;
		}
		++num;
	}

	cin.get();

	RenderWindow window(VideoMode(800, 800), "123");
	WndClass* wnd = new WndClass(&window, Vector2f(800, 800));

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
		window.clear(Color(30, 30, 30, 255));

		wnd->frame();

		window.display();
	}

	delete wnd;
	return 0;
}
