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

	double get_num(int st, bool read, bool negat = false, bool db = false) {
		bool ng = this->str[st] == '-';
		if (!negat && ng)
			throw ReadExpection(this->off, "Only positive numbers are allowed");
		if (ng) {
			st += 1;
			if (read)
				this->off += 1;
		}

		double rs = 0;
		int rd = 0;
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
			st += 1;
		}

		if (db && this->peek_char() == L'.') {
			if (read)
				this->seek(1);
			double pst = this->get_num(st + 1, read, false, false);
			while (pst > 1.0) pst /= 10;
			rs += pst;
		}
		return rs * (ng ? -1 : 1);
	}
};


enum class OpType {
	ConstNumber,
	DynNumber,

	Plus,
	Minus,
	Multiply,
	Divide,
	Pow,

	Cos,
	Sin,
	Log,
	Exp,
	Sqrt
};
class Opdef {
public:
	bool isFunction;
	OpType type;
	string repr;
	double(*eval)(struct OpNode*);
	int priority;

	template<typename F>
	Opdef(OpType type, string repr, const F& call, bool isf, int pr = 0) {
		this->repr = repr;
		this->eval = call;
		this->type = type;
		this->isFunction = isf;
		this->priority = pr;
	}

	int GetArgCount() {
		if (this->isFunction)
			return 1;
		switch (this->type) {
		case OpType::ConstNumber:
		case OpType::DynNumber:
			return 0;
		}
		return 2;
	}
};
static map<OpType, Opdef*> g_opcodes;

#define REG_OPCODE(name, repr, pr, fun) g_opcodes[OpType::name] = new Opdef(OpType::name, repr, fun, false, pr)
#define REG_FUNC(name, repr, fun) g_opcodes[OpType::name] = new Opdef(OpType::name, repr, fun, true)

typedef struct OpNode {
	OpNode* left, * right;
	Opdef* def;
	double arg;

	OpNode(OpType type) {
		this->left = this->right = nullptr;
		this->arg = 0;
		this->def = g_opcodes[type];
	}
	~OpNode() {
		if (this->left) {
			delete this->left;
			this->left = 0;
		}
		if (this->right) {
			delete this->right;
			this->right = 0;
		}
	}
} Node, * PNode;

Opdef* FindOpdef(string str) {
	for (auto& [type, def] : g_opcodes) {
		string& dd = def->repr;
		if (dd.length() && str.starts_with(dd))
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
			double num = stream.get_num(stream.get_cur(), true, false, true);
			PNode node = new Node(OpType::ConstNumber);
			node->arg = num;
			nodes.push_back(node);
			continue;
		}
		catch (ReadExpection& ex) {}

		if (stream.peek_char() == '(') {
			funcs.push(make_pair("(", nullptr));
			stream.seek(1);
			continue;
		}
		else if (stream.peek_char() == ')') {
			stream.seek(1);
			while (funcs.size()) {
				auto [r, fun] = funcs.top();
				if (r == '(') {
					funcs.pop();
					break;
				}
				PNode node = new Node(fun->type);
				nodes.push_back(node);
				funcs.pop();
			}

			if (funcs.size() && funcs.top().second && funcs.top().second->isFunction) {
				auto func = funcs.top().second;
				PNode node = new Node(func->type);
				nodes.push_back(node);

				funcs.pop();
			}

			continue;
		}

		string cstream = stream.str.substring(stream.get_cur());
		Opdef* def = FindOpdef(cstream);
		if (!def) {
			throw ReadExpection(stream.get_cur(), "No operand");
		}
		stream.seek(def->repr.length());

		if (def->type == OpType::DynNumber) {
			PNode node = new Node(OpType::DynNumber);
			nodes.push_back(node);
		}
		else if (def->isFunction) {
			funcs.push(make_pair(def->repr, def));
		}
		else {
			while (funcs.size()) {
				auto [r, fun] = funcs.top();
				if (!fun || fun->priority < def->priority)
					break;
				PNode node = new Node(fun->type);
				nodes.push_back(node);

				funcs.pop();
			}
			funcs.push(make_pair(def->repr, def));
		}
	}

	while (funcs.size()) {
		auto [r, fun] = funcs.top();
		if (!fun)
			throw ReadExpection(stream.get_cur(), "strike");
		PNode node = new Node(fun->type);
		nodes.push_back(node);

		funcs.pop();
	}

	static PNode(*BuildNode)(vector<PNode>&) = [](vector<PNode>& nodes) -> PNode {
		if (!nodes.size())
			return nullptr; // throw
		PNode curNode = nodes[nodes.size() - 1];
		nodes.pop_back();
		int argc = curNode->def->GetArgCount();
		if (argc > 0) {
			curNode->right = BuildNode(nodes);
		}
		if (argc > 1) {
			curNode->left = BuildNode(nodes);
		}
		return curNode;
	};

	return BuildNode(nodes);
}

static double g_GetDynVariable; // :(

double EvalNode(PNode pnode) {
	if (!pnode)
		return 0.0;
	return pnode->def->eval(pnode);
}

class Input {
	Vector2i pos;
	Vector2i size;
	String value;
	bool isActive;
public:
	PNode rootNode;
	void set_active(bool f);
	Input(Vector2f ps);
	void render(class WndClass* wndcl);
	void rebuild();
	void text_entered(Event evnt);
	void on_mousedown(Vector2f pos);
};

class WndClass {
	RenderWindow* wnd;
	Vector2f sizeV;

	vector<Input*> inputs;

	friend class Input;
public:
	float current_scale;
	float target_scale;
	Vector2f offset;
	float size;

	void create_inputs() {
		inputs.push_back(new Input(Vector2f(20, 20)));
		inputs.push_back(new Input(Vector2f(20, 50)));
		inputs.push_back(new Input(Vector2f(20, 80)));

	}

	WndClass(RenderWindow* wnd, Vector2f size) {
		this->wnd = wnd;
		this->sizeV = size;
		this->size = wnd->getSize().x;
		this->current_scale = this->size / 6.0;
		this->current_scale /= 2;
		this->target_scale = this->current_scale;
		this->create_inputs();
	}

	void do_buttons() {
		for (Input* in : this->inputs)
			in->render(this);
	}

	void draw_grid() {

		// x axis
		double xStart = wnd_to_plot(Vector2f(0, 0)).x;
		double xEnd = wnd_to_plot(Vector2f(this->size, 0)).x;
		if (xEnd < xStart)
			swap(xStart, xEnd);
		xStart = round(xStart);
		xEnd = round(xEnd);
		for (double cur = xStart; cur <= xEnd; cur += 1.f) {
			Vector2f wPos = plt_to_wnd(Vector2f(cur, 0));

			Vertex vertices[2];
			vertices[0] = Vertex(Vector2f(wPos.x, 0));
			vertices[1] = Vertex(Vector2f(wPos.x, this->size));
			vertices[0].color = Color(255, 255, 255, 30);
			vertices[1].color = Color(255, 255, 255, 30);
			wnd->draw(vertices, 2, Lines);
		}

		Vertex vertices[2];
		vertices[0] = Vertex(plt_to_wnd(Vector2f(-1e6, 0)));
		vertices[1] = Vertex(plt_to_wnd(Vector2f(1e6, 0)));
		vertices[0].color = vertices[1].color = Color(255, 255, 255, 255);
		wnd->draw(vertices, 2, Lines);
		vertices[0] = Vertex(plt_to_wnd(Vector2f(0, -1e6)));
		vertices[1] = Vertex(plt_to_wnd(Vector2f(0, 1e6)));
		wnd->draw(vertices, 2, Lines);

		//y axis
		xStart = wnd_to_plot(Vector2f(0, 0)).y;
		xEnd = wnd_to_plot(Vector2f(0, this->size)).y;
		if (xEnd < xStart)
			swap(xStart, xEnd);
		xStart = round(xStart);
		xEnd = round(xEnd);
		for (double cur = xStart; cur <= xEnd; cur += 1.f) {
			Vector2f wPos = plt_to_wnd(Vector2f(0, cur));

			Vertex vertices[2];
			vertices[0] = Vertex(Vector2f(0, wPos.y));
			vertices[1] = Vertex(Vector2f(this->size, wPos.y));
			vertices[0].color = Color(255, 255, 255, 30);
			vertices[1].color = Color(255, 255, 255, 30);
			wnd->draw(vertices, 2, Lines);
		}

		//0, 0
		Vector2f pPos = plt_to_wnd(Vector2f(0, 0));
		if (pPos.x >= 0 && pPos.x < this->size && pPos.y >= 0 && pPos.y < this->size) {
			Text text;
			text.setFont(g_Font);
			text.setString("(0,0)");
			text.setCharacterSize(10);
			text.setPosition(pPos);
			wnd->draw(text);
		}

	}

	Vector2f wnd_to_plot(Vector2f wnd) {
		wnd += offset;
		Vector2f rs = (wnd - Vector2f(size, size) / 2.f) / this->current_scale;
		rs.y = -rs.y;
		return rs;
	}
	Vector2f plt_to_wnd(Vector2f plt) {
		plt.y = -plt.y;
		Vector2f rs = plt * this->current_scale + Vector2f(size, size) / 2.f;
		rs -= Vector2f(offset.x, offset.y);
		return rs;
	}

	void render() {
		sf::Color clrs[3] = { sf::Color::Red, sf::Color::Blue, sf::Color::Cyan };

		//outline
		vector<Vertex> vt_buffer;
		vt_buffer.reserve(this->wnd->getSize().x);
		int i = 0;
		for (Input* sh : this->inputs) {
			if (!sh->rootNode)
				continue;
			sf::Color outline_color = clrs[i++ % (sizeof(clrs) / sizeof(clrs[0]))];
			for (int x = 0; x <= this->wnd->getSize().x; ++x) {
				double px = wnd_to_plot(Vector2f(x, 0)).x;
				// for x 
				g_GetDynVariable = px;
				double solution = EvalNode(sh->rootNode);
				
				Vector2f spos = plt_to_wnd(Vector2f(px, solution));

				//Vector2f prevVertex = vt_buffer.size() ? vt_buffer[vt_buffer.size() - 1].position : Vector2f(0, 0);

				//if (!(prevVertex.y > this->size && spos.y < 0) && !(prevV5ertex.y < 0 && spos.y > this->size)) {
					Vertex vertex;
					vertex.position = Vector2f(spos.x, spos.y);
					vertex.color = Color(outline_color.r, outline_color.g, outline_color.b, outline_color.a);
					vt_buffer.push_back(vertex);
				//}

			}

			if (vt_buffer.size())
				this->wnd->draw(vt_buffer.data(), vt_buffer.size(), PrimitiveType::LinesStrip);

			vt_buffer.clear();
		}
	}

	void frame() {
		this->current_scale += (this->target_scale - this->current_scale) * min(0.75f, abs(this->current_scale - this->target_scale));
		this->draw_grid();
		this->render();

		this->do_buttons();
	}


	void on_mousedown(Vector2f pos) {
		for (Input* in : this->inputs)
			in->on_mousedown(pos);
	}

	void on_mouseup(Vector2f pos) {

	}

	void text_entered(Event evnt) {
		for (Input* in : this->inputs)
			in->text_entered(evnt);
	}
};

void Input::set_active(bool f) {
	this->isActive = f;
}

Input::Input(Vector2f ps) {
	this->pos = Vector2i(ps.x, ps.y);
	this->size = Vector2i(200, 20);
	this->set_active(0);
}

void Input::render(WndClass* wndcl) {
	RectangleShape rect(Vector2f(this->size.x, this->size.y));
	rect.setPosition(wndcl->wnd->mapPixelToCoords(pos));
	rect.setOutlineColor(Color(0, 0, 0, 255));
	rect.setOutlineThickness(1.f);
	rect.setFillColor(Color(255, 255, 255, 255));
	wndcl->wnd->draw(rect);

	Text text;
	text.setFont(g_Font);
	text.setCharacterSize(16);
	text.setFillColor(Color(0, 0, 0, 255));
	// text.setScale(Vector2f(wndcl->zoomInternal, wndcl->zoomInternal));
	if (this->value.getSize()) {
		int cnt = this->value.getSize() - 1;
		while (cnt >= 0 && text.getLocalBounds().width + 10 < size.x) {
			text.setString(this->value[cnt--] + text.getString());
		}
		//this->lastDrawValue = text.getString();
	}
	Vector2f dp = Vector2f(this->pos.x, this->pos.y) + Vector2f(this->size.x - text.getLocalBounds().width, 0);
	text.setPosition(wndcl->wnd->mapPixelToCoords(Vector2i(dp.x, dp.y)));
	wndcl->wnd->draw(text);
}

void Input::rebuild() {
	if (this->rootNode) {
		delete this->rootNode;
		this->rootNode = nullptr;
	}
	if (!this->value.getSize())
		return;

	try {
		this->rootNode = ExprParse(Stream(this->value));
	}
	catch (...) {

	}
}

void Input::text_entered(Event evnt) {
	if (!this->isActive)
		return;
	if (evnt.text.unicode == 8) { // backspace
		if (this->value.getSize())
			this->value = this->value.substring(0, this->value.getSize() - 1);
	}
	else if (evnt.text.unicode) {
		this->value += evnt.text.unicode;
	}

	this->rebuild();
}

void Input::on_mousedown(Vector2f pos) {
	this->isActive = Rect<float>(this->pos.x, this->pos.y, this->size.x, this->size.y).contains(pos);
}


double fun1(double x) {
	return 0.6 * x + 3;
}
double fun2(double x) {
	return pow(x - 2, 3) - 1;
}
double fun3(double x) {
	return 3 / x;
}

double fun1p(double x) {
	return 0.6;
}
double fun2p(double x) {
	return 3 * pow(x - 2, 2);
}
double fun3p(double x) {
	return -3 / pow(x, 2);
}

double fun1pp(double x) {
	return 0;
}
double fun2pp(double x) {
	return 6 * (x - 2);
}
double fun3pp(double x) {
	return 6 / pow(x, 3);
}

using fnt = double(*)(double);
double get_roots(double a, double b, fnt afun, fnt bfun, fnt afunp, fnt bfunp, fnt afunpp, fnt bfunpp) {
	constexpr float EPS = 1e-6;
	while (abs(a - b) > EPS) {
		cout << a << " " << b << endl;
		if (afun(a) * afunpp(a) < 0) {
			a -= (afun(a) * (a - b)) / (afun(a) - bfun(b));
		}
		else if(afun(a) * afunpp(a) > 0){
			a -= afun(a) / afunp(a);
		}

		if (bfun(b) * bfunpp(b) < 0) {
			b = b - (bfun(b) * (b - a)) / (bfun(b) - afun(a));
		}
		else if (bfun(b) * bfunpp(b) > 0) {
			b = b - bfun(b) / bfunp(b);
		}
	}
	return (a + b) / 2;
}

void pr2() {
	double x1 = get_roots(0.1, 1, fun1, fun3, fun1p, fun3p, fun1pp, fun3pp);
	cout << x1 << endl;
}

int main()
{
	pr2();
	return 0;

	REG_OPCODE(ConstNumber, "", 0, [](PNode node) -> double {
		return node->arg;
		});
	REG_OPCODE(DynNumber, "x", 0, [](PNode node) -> double {
		return g_GetDynVariable;
		});
	REG_OPCODE(Plus, "+", 1, [](PNode node) -> double {
		return EvalNode(node->left) + EvalNode(node->right);
		});
	REG_OPCODE(Minus, "-", 1, [](PNode node) -> double {
		return EvalNode(node->left) - EvalNode(node->right);
		});
	REG_OPCODE(Multiply, "*", 2, [](PNode node) -> double {
		return EvalNode(node->left) * EvalNode(node->right);
		});
	REG_OPCODE(Divide, "/", 2, [](PNode node) -> double {
		if (abs(EvalNode(node->right)) <= 1e-6)
			return INFINITY;
		return EvalNode(node->left) / EvalNode(node->right);
		});
	REG_OPCODE(Pow, "^", 2, [](PNode node) -> double {
		return pow(EvalNode(node->left), EvalNode(node->right));
		});
	REG_FUNC(Cos, "cos", [](PNode node) -> double {
		return cos(EvalNode(node->right));
		});
	REG_FUNC(Sin, "sin", [](PNode node) -> double {
		return sin(EvalNode(node->right));
		});
	REG_FUNC(Log, "lg", [](PNode node) -> double {
		return log(EvalNode(node->right));
		});
	REG_FUNC(Exp, "exp", [](PNode node) -> double {
		return exp(EvalNode(node->right));
		});
	REG_FUNC(Sqrt, "sqrt", [](PNode node) -> double {
		return sqrt(EvalNode(node->right));
		});

	String tests[] = {
		String("cos(x)"), // 635825

	};
	int num = 1;
	for (String s : tests) {
		try {
			PNode rs = ExprParse(s);
			cout << "Test " << num << ": OK (" << EvalNode(rs) << ")" << endl;
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
		static bool _moving = false;
		static Vector2f _start = Vector2f();

		Event event;
		while (window.pollEvent(event))
		{
			if (event.type == Event::Closed) {
				window.close();
			}
			else if (event.type == sf::Event::MouseButtonPressed) {
				if (event.mouseButton.button == 0) {
					_moving = true;
					_start = Vector2f(Mouse::getPosition(window));

					wnd->on_mousedown(Vector2f(event.mouseButton.x, event.mouseButton.y));
				}
			}
			else if (event.type == sf::Event::MouseButtonReleased) {
				if (event.mouseButton.button == 0) {
					if (_moving) {
						wnd->offset -= (Vector2f(Mouse::getPosition(window)) - _start);
					}
					_moving = false;

					wnd->on_mouseup(Vector2f(event.mouseButton.x, event.mouseButton.y));
				}
			}
			else if (event.type == sf::Event::MouseWheelScrolled) {
				if (event.mouseWheelScroll.delta > 0) {
					wnd->target_scale *= 1.07f;
				}
				else {
					wnd->target_scale /= 1.05f;
				}
			}
			else if (event.type == Event::TextEntered) {
				wnd->text_entered(event);
			}
		}
		window.clear(Color(30, 30, 30, 255));

		Vector2f prev_offset = wnd->offset;
		if (_moving) {
			wnd->offset -= (Vector2f(Mouse::getPosition(window)) - _start);
		}

		wnd->frame();

		wnd->offset = prev_offset;
		window.display();
	}

	delete wnd;
	return 0;
}
