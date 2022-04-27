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
#include <stack>

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

	int get_num(int st, bool read, bool negat = false, bool db = false) {
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


enum class OpType {
	ConstNumber,
	DynNumber,
	Plus
};
class Opdef {
public:
	bool isFunction;
	OpType type;
	string repr;
	double(*eval)(struct OpNode*);
	int priority;

	template<typename F>
	Opdef(OpType type, string repr, F&& call, bool isf, int pr = 0) {
		this->repr = repr;
		this->eval = call;
		this->type = type;
		this->isFunction = isf;
	}
};
static map<OpType, Opdef*> g_opcodes;

#define REG_OPCODE(name, repr, pr, fun) g_opcodes[OpType::name] = new Opdef(OpType::name, repr, fun, pr, false);
#define REG_FUNC(name, repr, fun) g_opcodes[OpType::name] = new Opdef(OpType::name, repr, fun, true);

typedef struct OpNode {
	OpNode* left, *right;
	OpType type;
	double arg;

	OpNode() {
		this->left = this->right = nullptr;
	}
} Node, *PNode;

Opdef* FindOpdef(string str) {
	for (auto& [type, def] : g_opcodes) {
		string& dd = def->repr;
		if (str.starts_with(dd))
			return def;
	}
	return nullptr;
}

PNode ExprParse(Stream stream) {
	vector<PNode> nodes;

	std::stack<pair<string, Opdef*>> funcs;
	while (stream.get_cur() < stream.get_size()) {
		stream.read_while(' ');

		try {
			double num = stream.get_num(stream.get_cur(), true, true, true);
			PNode node = new Node();
			node->type = OpType::ConstNumber;
			node->arg = num;
			nodes.emplace_back(node);
			continue;
		}
		catch (ReadExpection& ex) {}

		if (stream.peek_char() == '(') {
			funcs.push(make_pair("(", nullptr));
			continue;
		}
		else if (stream.peek_char() == ')') {
			vector<PNode> args;
			while (funcs.size()) {
				auto [r, fun] = funcs.top();
				if (r == '(') {
					funcs.pop();
					break;
				}
				args.emplace_back(fun);
				funcs.pop();
			}

			if (funcs.size() && funcs.top().second && funcs.top().second->isFunction) {
				auto func = funcs.top().second;
				PNode node = new Node();
				node->type = func->type;
				node->left = args[0];
				nodes.push_back(node);

				funcs.pop();
			}
			else {

			}

			continue;
		}

		string cstream = stream.str.substring(stream.get_cur());
		Opdef* def = FindOpdef(cstream);
		if (!def) {
			throw ReadExpection(stream.get_cur(), "No operand");
		}

		if (def->isFunction) {
			funcs.push(make_pair(def->repr, def));
		}
		else {
			while (funcs.size()) {
				auto [r, fun] = funcs.top();
				if (!fun || fun->priority <= def->priority)
					break;
				nodes.emplace_back(fun);
				funcs.pop();
			}
			funcs.push(make_pair(def->repr, def));
		}
	}
	

	return nullptr;
}

double EvalNode(PNode pnode) {
	return g_opcodes[pnode->type]->eval(pnode);
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
			
			text.setString("Fail: Generic");
			
		}
		else {

			text.setString("Fail: Generic");
			
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
	REG_OPCODE(ConstNumber, "", 0, [](PNode node) -> double {
		return node->arg;
	})
	REG_OPCODE(DynNumber, "x", 0, [](PNode node) -> double {
		return node->arg;
	})
	REG_OPCODE(Plus, "+", 0, [](PNode node) -> double {
		return EvalNode(node->left) + EvalNode(node->right);
	})

	String tests[] = {
		String("x + 1"), // 1
		
	};
	/*int num = 1;
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
	}*/

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
