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
	bool debug;

	Parser_op next_opcode(bool read);
	Parser_val next_opval(bool read);
	vector<pair<Parser_val, int>> next_opvals(bool read);

	int next_expr(int cur, bool read);
	int val_to_value(Parser_val opval, bool read);

public:
	Parser1(String str) {
		this->stream = Stream(str);
		this->debug = false;
	}
	Parser1(Stream str) {
		this->stream = str;
		this->debug = false;
	}

	void set_debug(bool s) {
		this->debug = s;
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
			this->stream.read_expect({ "not ", "not" }, "Expected not[space]");
		}
		else {
			this->stream.peek_expect({ "not ", "not" }, "Expected not[space]");
		}
		return !val_to_value(this->next_opval(false), read);
	case Parser_val::Expr:
		if (read) {
			this->stream.read_expect({ "(" }, "Expected (");
		}
		else {
			this->stream.peek_expect({ "(" }, "Expected (");
		}
		String block = this->stream.get_closing_block(this->stream.get_cur() + !read, '(', ')');
		block = block.substring(0, block.getSize() - 1);
		if (this->debug) {
			cout << "expr parse: " << block.toAnsiString() << endl;
		}
		try {
			int ret = Parser1(block).parse();
			if (read) {
				this->stream.set_cur(this->stream.get_cur() + block.getSize());
				this->stream.read_expect({ ")" }, "Expected )");
			}
			else {
				int cprev = this->stream.get_cur();
				this->stream.set_cur(this->stream.get_cur() + block.getSize());

				this->stream.peek_expect({ ")" }, "Expected )");

				this->stream.set_cur(cprev);
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
		vl = stream.read_expect({ "and ", "or ", "and", "or" }, "Expected and|or");
	}
	else {
		vl = stream.peek_expect({ "and ", "or ", "and", "or" }, "Expected and|or");
	}

	if (vl == 0 || vl == 2) {
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
		String block = this->stream.get_closing_block(this->stream.get_cur() + (read ? 0 : 1), '(', ')');
		block = block.substring(0, block.getSize() - 1);
		if (this->debug) {
			cout << "expr: " << block.toAnsiString() << endl;
		}
		try {
			int val = Parser1(block).parse();
			if (read) {
				this->stream.set_cur(this->stream.get_cur() + block.getSize());
			}
		}
		catch (ReadExpection& ex) {
			throw ReadExpection(this->stream.get_cur() + ex.position, ex.msg);
		}

		return Parser_val::Expr;
	}
}

vector<pair<Parser_val, int>> Parser1::next_opvals(bool read) {
	if (read) {
		stream.read_expect({ "(" }, "Expected (");
		int start = this->stream.get_cur();

		vector<pair<Parser_val, int>> vec;
		int tlength = 0;
		while (true) {
			int v = 0;
			Parser_val vl = this->next_opval(false);
			int dcur = this->stream.get_cur();
			String block;
			if (vl == Parser_val::False) {
				block = this->stream.str.substring(this->stream.get_cur(), 5);
			}
			else if (vl == Parser_val::True) {
				block = this->stream.str.substring(this->stream.get_cur(), 4);
			}
			else if (vl == Parser_val::Not) {
				int jk = stream.read_expect({ "not ", "not" }, "Expected not");
				tlength += (jk ? 3 : 4);
				continue;
			}
			else if (vl == Parser_val::Expr) {
				block = this->stream.get_closing_block(this->stream.get_cur(), '(', ')', 0);
			}
			if (this->debug) {
				cout << block.toAnsiString() << endl;
			}

			try {
				Parser_val vl2 = Parser1(block).next_opval(true);
				vec.push_back({ tlength ? Parser_val::Not : vl2, block.getSize() + tlength });
				tlength = 0;
			}
			catch (ReadExpection& ex) {
				throw ReadExpection(this->stream.get_cur() + ex.position, ex.msg);
			}

			try {
				this->stream.set_cur(dcur + block.getSize());
				int k = stream.read_expect({ ",", ")" }, "Expected , or )");
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
		stream.peek_expect({ "(" }, "Expected (");

		Stream stream2 = '(' + this->stream.get_closing_block(this->stream.get_cur() + 1, '(', ')');
		if (this->debug) {
			cout << stream2.str.toAnsiString() << endl;
		}
		try {
			return Parser1(stream2).next_opvals(true);
		}
		catch (ReadExpection& ex) {
			throw ReadExpection(this->stream.get_cur() + ex.position, ex.msg);
		}
	}
}

int Parser1::next_expr(int cur, bool read) {
	//this->debug = true;

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

		vector<pair<Parser_val, int>> vals = this->next_opvals(false);
		if (vals.size() < 2) {
			if (vals.size() < 1) {
				throw ReadExpection(this->stream.get_cur(), "Less than 2 operands");
			}
			else {
				throw ReadExpection(this->stream.get_cur() + vals[0].second, "Less than 2 operands");
			}
		}

		if (read) {
			stream.read_expect({ "(" }, "Expected (");
		}
		else {
			stream.peek_expect({ "(" }, "Expected (");
		}

		int ppos = this->stream.get_cur();
		vector<int> values(vals.size());
		for (int i = 0; i < vals.size(); ++i) {
			this->stream.set_cur(ppos);
			values[i] = this->val_to_value(vals[i].first, read);
			ppos += vals[i].second + 1;
		}

		if (!read) {
			this->stream.set_cur(ppos);
		}

		if (read) {
			stream.read_expect({ ")" }, "Expected )");
		}
		else {
			stream.peek_expect({ ")" }, "Expected )");
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

enum class Parser2_sign {
	Plus,
	Minus
};
class Parser2 {
	Stream stream;
	bool debug;

	int next_sum(bool read);
	int next_term(bool read);
	Parser2_sign next_sign(bool read);
	int next_num(bool read);
	int eval_sign(int left, int right, Parser2_sign sign);

public:
	Parser2(String str) {
		this->stream = Stream(str);
		this->debug = false;
	}
	Parser2(Stream str) {
		this->stream = str;
		this->debug = false;
	}

	void set_debug(bool s) {
		this->debug = s;
	}

	int parse() {
		int res = this->next_sum(true);

		if (this->stream.get_cur() < this->stream.get_size()) {
			String next_str = to_string(res) + this->stream.str.substring(this->stream.get_cur());
			int pos_shift = this->stream.get_size() - next_str.getSize();
			try {
				Parser2 ps = Parser2(next_str);
				ps.set_debug(this->debug);
				return ps.parse();
			}
			catch (ReadExpection& ex) {
				throw ReadExpection(this->stream.get_cur() - pos_shift + ex.position, ex.msg);
			}
		}
		return res;
	}
};

Parser2_sign Parser2::next_sign(bool read) {
	int vl;
	if (read) {
		vl = stream.read_expect({ "+", "-" }, "Expected +|-");
	}
	else {
		vl = stream.peek_expect({ "+", "-" }, "Expected +|-");
	}

	if (vl == 0) {
		return Parser2_sign::Plus;
	}
	else {
		return Parser2_sign::Minus;
	}
}

int Parser2::next_num(bool read) {
	return this->stream.get_num(this->stream.get_cur(), read);
}

int Parser2::next_term(bool read) {
	try {
		stream.peek_expect({ "(" }, "Expected (");
	}
	catch (ReadExpection& ex) {
		return this->next_num(read);
	}

	if (read) {
		stream.read_expect({ "(" }, "Expected (");
	}
	String block = this->stream.get_closing_block(this->stream.get_cur() + !read, '(', ')');
	if (block.getSize() < 1 || block[block.getSize() - 1] != ')') {
		throw ReadExpection(this->stream.get_cur() + block.getSize(), "Expected )");
	}
	block = block.substring(0, block.getSize() - 1);

	if (read) {
		this->stream.seek(block.getSize() + 1);
	}

	try {
		Parser2 ps = Parser2(block);
		ps.set_debug(this->debug);
		int right = ps.parse();
		return right;
	}
	catch (ReadExpection& ex) {
		throw ReadExpection(this->stream.get_cur() - block.getSize() - 1 + ex.position, ex.msg);
	}
}

int Parser2::eval_sign(int left, int right, Parser2_sign sign) {
	switch (sign) {
	case Parser2_sign::Plus:
		return left + right;
	case Parser2_sign::Minus:
		return left - right;
	}
	throw 0;
}

int Parser2::next_sum(bool read) {
	if (this->debug) {
		cout << "next_sum " << this->stream.str.toAnsiString() << endl;
	}
	bool neg = false;
	try {
		stream.read_expect({ "-" });
		neg = true;
	}
	catch (ReadExpection& ex) {

	}

	int term = this->next_term(read);
	if (neg) {
		term = -term;
	}

	try {
		this->next_sign(false); // probe read sign
	}
	catch (ReadExpection& ex) {
		if (!this->stream.is_end()) {
			throw ReadExpection(this->stream.get_cur(), "Unexpected symbol");
		}
		return term;
	}

	Parser2_sign sig = this->next_sign(true);
	int right = this->next_term(read);
	if (this->debug) {
		cout << "sign " << (int)sig << " left " << term << " right " << right << endl;
	}
	return this->eval_sign(term, right, sig);
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
		Vector2f field_pos = Vector2f(this->size.x / 2 - field_size.x / 2, this->size.y / 2 - field_size.y / 2);

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

		//cout << "Testing: " << current_text.toAnsiString() << endl;
		ReadExpection expection = ReadExpection(0, "");
		bool isErrored = false;
		if (!this->button_state) {
			Parser1 parser(this->current_text);
			try {
				int rs = parser.parse();
				text.setString("OK (" + to_string(rs) + ")");
				//cout << "ans: " << rs << endl;
			}
			catch (ReadExpection& ex) {
				//cout << "err: " << ex.what() << endl;
				text.setString(ex.what());
				expection = ex;
				expection.position = min(expection.position, (int)this->current_text.getSize());
				isErrored = true;
			}
			catch (...) {
				text.setString("Fail: Generic");
			}
		}
		else {
			Parser2 parser(this->current_text);
			try {
				int rs = parser.parse();
				text.setString("OK (" + to_string(rs) + ")");
			}
			catch (ReadExpection& ex) {
				text.setString(ex.what());
				expection = ex;
				expection.position = min(expection.position, (int)this->current_text.getSize());
				isErrored = true;
			}
			catch (...) {
				text.setString("Fail: Generic");
			}
		}
		this->wnd->draw(text);

		Vector2f offset = Vector2f(5, 0);
		for (int i = 0; i < this->current_text.getSize(); ++i) {
			Text text2;
			text2.setFont(g_Font);
			text2.setCharacterSize(16);
			text2.setFillColor(Color(200, 200, 200, 255));
			text2.setString(this->current_text.substring(i, 1));

			FloatRect bRect = text2.getGlobalBounds();
			if (offset.x + bRect.width >= field_size.x) {
				offset.y += 20;
				offset.x = 5;
			}

			if (isErrored && i == expection.position) {
				RectangleShape rect(Vector2f(5, 20));
				rect.setPosition(field_pos + offset);
				rect.setFillColor(Color(255, 0, 0, 255));
				this->wnd->draw(rect);
			}
			text2.setPosition(field_pos + offset);
			this->wnd->draw(text2);
			offset.x += bRect.width + 0.5f;
		}

		if (isErrored && this->current_text.getSize() == expection.position) {
			RectangleShape rect(Vector2f(5, 20));
			rect.setPosition(field_pos + offset);
			rect.setFillColor(Color(255, 0, 0, 255));
			this->wnd->draw(rect);
		}
	}


	void on_mousedown(Vector2f pos) {
		Vector2f button_pos = Vector2f(250, 150);
		Vector2f button_size = Vector2f(25, 25);
		if (Rect<float>(button_pos, button_size).contains(pos)) {
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
		String("and((or(false,not true,(false),(or(false,true)))),(or(false,not true,(false),(and(not false,true)))))"), // 1
		String("((((false))))"), // 0
		String("not (or (false,(not (not (((true)))))))"), // 0
		String("and(true,(or(false,not true)),not not true)") // 1
	};
	int num = 1;
	for (String s : tests) {
		Parser1 parser(s);
		try {
			int rs = parser.parse();
			cout << "Test " << num << ": OK (" << rs << ")" << endl;
		}
		catch (ReadExpection& ex) {
			cout << "Test " << num << ": Fail (" << ex.what() << ")" << endl;
		}
		catch (...) {
			cout << "Test " << num << ": Fail (Generic)" << endl;
		}
		++num;
	}

	/*String tests[] = {
		String("555+(0-(0-6))+(((0-6666)))+6106"), // 1
		String("182621-2818-(12827-18288817-(817277+(2)))+((81-23-(18272+0))+917236)-20172093"), //1
		String("((0)+5)")
	};
	int num = 1;
	for (String s : tests) {
		Parser2 parser(s);
		//parser.set_debug(true);
		try {
			int rs = parser.parse();
			cout << "Test " << num << ": OK (" << rs << ")" << endl;
		}
		catch (ReadExpection& ex) {
			cout << "Test " << num << ": Fail (" << ex.what() << ")" << endl;
		}
		catch (...) {
			cout << "Test " << num << ": Fail (Generic)" << endl;
		}
		++num;
	}*/

	//cin.get();

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
