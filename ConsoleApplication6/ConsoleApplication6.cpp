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

using namespace std;
using namespace sf;

Font g_Font;

class ReadExpection : exception {
public:
	int position;
	string msg;

	ReadExpection(int position, string message = "") {
		this->position = position;
		this->msg = message + " at pos " + to_string(position);
	}


	virtual const char* what() const throw()
	{
		return this->msg.c_str();
	}
};


class Stream {
	String str;
	int off;
public:
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
};

enum class Parser_op {
	And,
	Or
};
enum class Parser_val {
	True,
	False,
	Not,
	Expr
};

class Parser1 {
	Stream stream;

	Parser_op next_opcode(bool read);
	Parser_val next_opval(bool read);
	vector<Parser_val> next_opvals(bool read);

	int next_expr(int cur, bool read);
	int val_to_value(Parser_val opval, bool read);

public:
	Parser1(String str) {
		this->stream = Stream(str);
	}
	Parser1(Stream str) {
		this->stream = str;
	}

	int parse() {
		int res = this->next_expr(0, true);
		if (this->stream.get_cur() < this->stream.get_size()) {
			throw ReadExpection(this->stream.get_cur(), "Unexpected end");
		}
		return res;
	}
};

int Parser1::val_to_value(Parser_val opval, bool read) {
	switch (opval) {
	case Parser_val::True:
		this->stream.read_expect({ "true" }, "Expected true");
		return 1;
	case Parser_val::False:
		this->stream.read_expect({ "false" }, "Expected false");
		return 0;
	case Parser_val::Not:
		if (read) {
			this->stream.read_expect({ "not ", "not"}, "Expected not[space]");
		}
		else {
			this->stream.peek_expect({ "not ", "not" }, "Expected not[space]");
		}
		return !val_to_value(this->next_opval(false), read);
	case Parser_val::Expr:
		cout << this->stream.get_cur() << endl;
		if (read) {
			this->stream.read_expect({ "(" }, "Expected (");
		}
		else {
			this->stream.peek_expect({ "(" }, "Expected (");
		}
		String block = this->stream.get_block(this->stream.get_cur() + !read, ')');
		try {
			int ret = Parser1(block).parse();
			if (read) {
				this->stream.set_cur(this->stream.get_cur() + block.getSize() + 1);
			}
			return ret;
		}
		catch (ReadExpection& ex) {
			throw ReadExpection(this->stream.get_cur() + ex.position, ex.msg);
		}
	}
}

Parser_op Parser1::next_opcode(bool read) {
	int vl;
	if (read) {
		vl = stream.read_expect({ "and", "or" }, "Expected and/or");
	}
	else {
		vl = stream.peek_expect({ "and", "or" }, "Expected and/or");
	}

	if (vl == 0) {
		return Parser_op::And;
	}
	else {
		return Parser_op::Or;
	}
}

Parser_val Parser1::next_opval(bool read) {
	int vl;
	if (read) {
		vl = stream.read_expect({ "true", "false", "not", "(" }, "Expected true/false/not/()");
	}
	else {
		vl = stream.peek_expect({ "true", "false", "not", "(" }, "Expected true/false/not/()");
	}

	if (vl == 0) {
		return Parser_val::True;
	}
	else if (vl == 1) {
		return Parser_val::False;
	}
	else if (vl == 2) {
		return Parser_val::Not;
	}
	else if (vl == 3) {
		String block = this->stream.get_block(this->stream.get_cur() + (read ? 0 : 1), ')');
		cout << "expr: " << block.toAnsiString() << endl;
		try {
			int val = Parser1(block).parse();
			if (read) {
				this->stream.set_cur(this->stream.get_cur() + block.getSize() + 1);
			}
		}
		catch (ReadExpection& ex) {
			throw ReadExpection(this->stream.get_cur() + ex.position, ex.msg);
		}

		return Parser_val::Expr;
	}
}

vector<Parser_val> Parser1::next_opvals(bool read) {
	if (read) {
		stream.read_expect({ "(" }, "Expected (");

		vector<Parser_val> vec;
		while (true) {
			int v = 0;
			String block = this->stream.get_block(this->stream.get_cur(), {',', ')'}, &v );
			if (v == 0) {
				block = block.substring(0, block.getSize() - 1);
			}
			cout << block.toAnsiString() << endl;

			Parser_val vl = Parser1(block).next_opval(true);
			vec.push_back(vl);
			try {
				this->stream.set_cur(this->stream.get_cur() + block.getSize());
				int k = stream.read_expect({ ",", ")"}, "Expected , or )");
				if (k == 1) {
					break;
				}
			}
			catch (ReadExpection& ex) {
				throw ReadExpection(this->stream.get_cur() - block.getSize() + ex.position, ex.msg);
			}
		}
		return vec;
	}
	else {
		Stream stream2 = this->stream;
		return Parser1(stream2).next_opvals(true);
	}
}

int Parser1::next_expr(int cur, bool read) {
	try {
		 // операнд
		Parser_val opval = this->next_opval(false);
		return this->val_to_value(opval, read);
	}
	catch (ReadExpection& ex) {
		// операция(операнд,операнд)
		Parser_op oper = this->next_opcode(false);
		if (this->next_opcode(true) != oper) {
			throw 0;
		}

		vector<Parser_val> vals = this->next_opvals(false);
		if (vals.size() < 2) {
			throw ReadExpection(this->stream.get_cur(), "Less than 2 operands");
		}

		if (read) {
			stream.read_expect({ "(" }, "Expected (");
		}
		else {
			stream.peek_expect({ "(" }, "Expected (");
		}
		vector<int> values(vals.size());
		for (int i = 0; i < vals.size(); ++i) {
			values[i] = this->val_to_value(vals[i], read);
			if (read) {
				stream.read_expect({ ",", ")" }, "Expected , or )");
			}
			else {
				stream.peek_expect({ ",", ")" }, "Expected , or )");
			}
		}

		int flag = 0;
		switch (oper) {
		case Parser_op::And:
			flag = values[0] & values[1];
			for (int i = 2; i < values.size(); ++i) {
				flag &= values[i];
			}
			return flag;
		case Parser_op::Or:
			flag = values[0] | values[1];
			for (int i = 2; i < values.size(); ++i) {
				flag |= values[i];
			}
			return flag;
		}
		throw 0;
	}
	throw 0;
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
		// eval
		Vector2f eval_pos = Vector2f(250 + 140, 150 + 400 + 100);
		Vector2f eval_size = Vector2f(80, 25);

		Text text2;
		text2.setFont(g_Font);
		text2.setCharacterSize(18);
		text2.setFillColor(Color(200, 200, 200, 255));
		text2.setString("Evaluate");
		text2.setPosition(eval_pos);

		FloatRect rc = text2.getLocalBounds();
		RectangleShape rect2(Vector2f(rc.width * 1.2f, rc.height * 2.f));
		rect2.setPosition(eval_pos - Vector2f(5, 0));
		rect2.setFillColor(Color(90, 90, 90, 255 / 2));
		this->wnd->draw(rect2);

		this->wnd->draw(text2);
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

		Vector2f offset = Vector2f(5, 0);
		for (int i = 0; i < this->current_text.getSize(); ++i) {
			Text text;
			text.setFont(g_Font);
			text.setCharacterSize(14);
			text.setFillColor(Color(200, 200, 200, 255));
			text.setString(this->current_text.substring(i, 1));

			FloatRect bRect = text.getLocalBounds();
			if (offset.x + bRect.width >= field_size.x) {
				offset.y += 20;
				offset.x = 5;
			}
			text.setPosition(field_pos + offset);
			this->wnd->draw(text);
			offset.x += bRect.width;
		}
	}

	void on_eval() {

	}

	void on_mousedown(Vector2f pos) {
		Vector2f button_pos = Vector2f(250, 150);
		Vector2f button_size = Vector2f(25, 25);
		if (Rect(button_pos, button_size).contains(pos)) {
			this->button_state = !this->button_state;
			return;
		}

		Vector2f eval_pos = Vector2f(250 + 140, 150 + 400 + 100);
		Vector2f eval_size = Vector2f(80, 25);
		Text text2;
		text2.setFont(g_Font);
		text2.setCharacterSize(18);
		text2.setFillColor(Color(200, 200, 200, 255));
		text2.setString("Evaluate");
		text2.setPosition(eval_pos);

		FloatRect rc = text2.getLocalBounds();
		Rect rct = Rect(eval_pos - Vector2f(5, 0), Vector2f(rc.width * 1.2f, rc.height * 2.f));
		if (rct.contains(pos)) {
			this->on_eval();
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
	Parser1 pr(String("or(false,not true,(false),(or (false,true)))"));
	try {
		int rs = pr.parse();
		cout << "ans: " << rs << endl;
	}
	catch (ReadExpection& ex) {
		cout << "th: " << ex.what() << endl;
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

	return 0;
}
