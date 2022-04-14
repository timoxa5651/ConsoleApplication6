﻿#include <iostream>
#include <thread>
#include <exception>
#include <algorithm>
#include <functional>
#include "SFML/Graphics.hpp"

static sf::Font g_Font;
static int g_FrameCount = 0;
using std::cout;
using std::cin;
using std::endl;
using std::remove_pointer_t;
using std::vector;
using std::string;
using std::map;
using std::function;
using std::exception;

using sf::RenderWindow;
using sf::VideoMode;
using sf::Event;
using sf::Vector2f;
using sf::Color;
using sf::String;

#include "AVLTree.hpp"
#include "SplayTree.hpp"

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
		this->fmsg = this->msg + " at pos " + std::to_string(position + 1);
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
				rd = std::max(1, rd * 10);
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

enum class TreeOpType {
	Insert,
	Delete
};
template <typename T>
struct TreeOp {
	TreeOpType type;
	T param;

	TreeOp(TreeOpType t, const T& p) {
		this->type = t;
		this->param = p;
	}
};

template<class T, class V>
class TreeView {
	typedef typename V::NodeType Node;
	typedef Node* PNode;

	void ProcessOperation(const TreeOp<T>& opr) {
		switch (opr.type) {
		case TreeOpType::Insert: {
			this->tree->Insert(opr.param);
			break;
		}
		case TreeOpType::Delete: {
			this->tree->Delete(opr.param);
			break;
		}
		}
	}

	V* tree;
	int historyIndex;
	bool hasChanges;
	vector<map<int, PNode>> grid;
	vector<map<int, float>> xes;

public:
	TreeView() {
		this->tree = new V();
		this->historyIndex = 0;
		this->hasChanges = false;
	}

	~TreeView() {
		if (this->tree) {
			delete this->tree;
			this->tree = 0;
		}
	}

	string GetName() {
		return this->tree->TreeName();
	}

	void UpdateOperations(const vector<TreeOp<T>>& latest) {
		for (int i = historyIndex; i < latest.size(); ++i) {
			this->ProcessOperation(latest[i]);
		}
		if (this->historyIndex != latest.size())
			this->hasChanges = true;
		this->historyIndex = latest.size();
	}

	void RebuildGrid() {
		this->grid.clear();
		this->xes.clear();

		static map<int, vector<PNode>> heights;
		this->tree->InOrder([](PNode node, int height) {
			heights[height].push_back(node);
		});
		int max_height = heights.size();
		if (!max_height)
			return;
		this->grid.resize(max_height, map<int, PNode>());
		this->xes.resize(max_height, map<int, float>());
		PNode root = heights[0][0];

		vector<int> indices(max_height, 0);
		function<void(PNode, int)> process;
		process = [&](PNode node, int level) {
			if (!node) {
				int dy = 1;
				for (int i = level; i < max_height; ++i) {
					indices[i] += dy;
					dy *= 2;
				}
				return;
			}

			this->grid[level][indices[level]] = node;
			indices[level] += 1;

			process(node->left, level + 1);
			process(node->right, level + 1);
		};
		process(root, 0);

		heights.clear();

		float spacing = Node::GetNodeSize() + Node::GetNodeSpacing();
		float maxWidth = spacing;
		for (int i = 1; i < this->grid.size(); ++i) {
			//maxWidth += ((this->grid[i].size() / (float)pow(2, i)) * spacing);
		}
		//maxWidth /= (float)this->grid.size();
		cout << maxWidth << endl;

		auto& last = this->grid[this->grid.size() - 1];
		auto iter = last.begin();
		int cur = 0;
		for (int i = 0; i < pow(2, this->grid.size() - 1); ++i) {
			this->xes[this->grid.size() - 1][i] = i * maxWidth;
		}

		for (int level = this->grid.size() - 2; level >= 0; --level) {
			auto& definition = this->grid[level];
			for (int i = 0; i < pow(2, level); ++i) {
				float x1 = this->xes[level + 1][i * 2];
				float x2 = this->xes[level + 1][i * 2 + 1];

				float trg = (x1 + x2) / 2.f;
				if (i > 0) {
					//float prev = this->xes[level][i - 1];
					//if(prev != trg)
						//trg += std::max(0.f, Node::GetNodeSize() + Node::GetNodeSpacing() - trg + prev);
				}
				this->xes[level][i] = trg;
			}
		}
	}

	void Draw(RenderWindow* window) {
		if (this->hasChanges) {
			this->RebuildGrid();
			this->hasChanges = false;
		}

		auto draw_node = [&](Vector2f coords, PNode node, Vector2f parentPos) {
			node->Draw(window, coords, parentPos);
		};

		for (int level = this->grid.size() - 1; level >= 0; --level) {
			auto& definition = this->grid[level];
			auto GridToCoords = [&](int x, int y) {
				if (y < 0)
					return Vector2f(FLT_MAX, FLT_MAX);
				Vector2f coords = Vector2f(this->xes[y][x], y * (Node::GetNodeSize() * 2.8f));
				return coords;
			};

			for (auto& [xpos, node] : definition) {
				draw_node(GridToCoords(xpos, level), node, GridToCoords(xpos / 2, level - 1));
			}
		}
	}

	PNode FindByPos(Vector2f world) {
		for (int level = this->grid.size() - 1; level >= 0; --level) {
			auto& definition = this->grid[level];
			auto GridToCoords = [&](int x, int y) {
				if (y < 0)
					return Vector2f(FLT_MAX, FLT_MAX);
				Vector2f coords = Vector2f(this->xes[y][x], y * (Node::GetNodeSize() * 2.8f));
				return coords;
			};

			for (auto& [xpos, node] : definition) {
				Vector2f ppos = GridToCoords(xpos, level);
				if (sf::FloatRect(ppos, Vector2f(Node::GetNodeSize(), Node::GetNodeSize()) * 2.f).contains(world)) {
					return node;
				}
			}
		}
		return 0;
	}
};

template<typename T>
class TreeDrawer {
	vector<TreeOp<T>> opHistory;
	vector<TreeView<T, BaseTree<T>>*> treeViews;
	int activeViewIndex;
	RenderWindow* window;
	String insFieldText;

public:
	float windowZoomInternal;

	TreeDrawer(RenderWindow* window) {
		this->windowZoomInternal = 1.f;
		this->window = window;

		this->treeViews.push_back(reinterpret_cast<decltype(treeViews)::value_type>(new TreeView<T, AVLTree<T>>()));
		this->treeViews.push_back(reinterpret_cast<decltype(treeViews)::value_type>(new TreeView<T, SplayTree<T>>()));
		this->activeViewIndex = 0;
	}

	void InsertAll(const T& val) {
		this->opHistory.push_back(TreeOp(TreeOpType::Insert, val));
		this->treeViews[this->activeViewIndex]->UpdateOperations(this->opHistory);
	}
	void DeleteAll(const T& val) {
		this->opHistory.push_back(TreeOp(TreeOpType::Delete, val));
		this->treeViews[this->activeViewIndex]->UpdateOperations(this->opHistory);
	}

	void SwitchView(int desiredView) {
		auto prevView = this->treeViews[this->activeViewIndex];
		this->activeViewIndex = desiredView;
		auto curView = this->treeViews[this->activeViewIndex];
		curView->UpdateOperations(this->opHistory);
		sf::View view = this->window->getView();
		sf::Vector2f newPos = Vector2f(0, 0);
		view.setCenter(newPos);
		window->setView(view);
	}

	void Frame() {
		this->treeViews[this->activeViewIndex]->Draw(this->window);

		Vector2f dtv2 = Vector2f(this->window->getSize().x - 150, 10);
		Vector2f asd = this->window->mapPixelToCoords(sf::Vector2i(dtv2.x - 70, dtv2.y));
		Vector2f dtv = this->window->mapPixelToCoords(sf::Vector2i(dtv2.x, dtv2.y));
		Vector2f end = this->window->mapPixelToCoords(sf::Vector2i(dtv2.x + 150, dtv2.y + 30));

		sf::RectangleShape rect(end - dtv);
		rect.setPosition(dtv);
		rect.setFillColor(Color(255, 255, 255, 255));
		this->window->draw(rect);

		sf::Text text;
		text.setFont(g_Font);
		text.setScale(Vector2f(this->windowZoomInternal, this->windowZoomInternal));
		text.setCharacterSize(16);
		text.setFillColor(Color(0, 0, 0, 255));
		text.setString(this->insFieldText);
		text.setPosition(dtv);
		this->window->draw(text);

		sf::Text text2;
		text2.setFont(g_Font);
		text2.setScale(Vector2f(this->windowZoomInternal, this->windowZoomInternal));
		text2.setCharacterSize(16);
		text2.setString(String("Insert N"));
		text2.setPosition(asd);
		this->window->draw(text2);

		for (int i = 0; i < this->treeViews.size(); ++i) {
			constexpr int wid = 80;

			Vector2f dtv2 = Vector2f(i * wid, this->window->getSize().y - 60);
			Vector2f start = this->window->mapPixelToCoords(sf::Vector2i(dtv2.x, dtv2.y));
			Vector2f end = this->window->mapPixelToCoords(sf::Vector2i(dtv2.x + wid, dtv2.y + 60));

			sf::RectangleShape rect(end - start);
			rect.setPosition(start);
			rect.setFillColor(Color(255, 255, 255, 255));
			this->window->draw(rect);

			text2.setString(this->treeViews[i]->GetName());
			text2.setPosition(start);
			text2.setFillColor(Color(0, 0, 0, 255));
			this->window->draw(text2);

			sf::VertexArray lines(sf::Lines, 2);
			lines[0] = sf::Vertex(sf::Vector2f(end.x, start.y));
			lines[1] = sf::Vertex(sf::Vector2f(end.x, end.y));
			lines[0].color = lines[1].color = sf::Color(0, 0, 0, 255);
			window->draw(lines);
		}
	}

	void text_entered(Event evnt) {
		if (evnt.text.unicode == 13) {
			try {
				Stream str = Stream(this->insFieldText);
				this->insFieldText = "";
				int num = str.get_num(0, true, true);
				this->InsertAll(num);
			}
			catch (...) {}
		}
		else if (evnt.text.unicode == 8) { //backspace
			if (this->insFieldText.getSize())
				this->insFieldText = this->insFieldText.substring(0, this->insFieldText.getSize() - 1);
		}
		else {
			this->insFieldText += evnt.text.unicode;
		}
	}

	void OnClicked(Vector2f world) {
		for (int i = 0; i < this->treeViews.size(); ++i) {
			constexpr int wid = 80;

			Vector2f dtv2 = Vector2f(i * wid, this->window->getSize().y - 60);
			Vector2f start = this->window->mapPixelToCoords(sf::Vector2i(dtv2.x, dtv2.y));
			Vector2f end = this->window->mapPixelToCoords(sf::Vector2i(dtv2.x + wid, dtv2.y + 60));

			if (sf::FloatRect(start, end - start).contains(world)) {
				this->SwitchView(i);
				return;
			}
		}

		auto node = this->treeViews[this->activeViewIndex]->FindByPos(world);
		if (node) {
			this->DeleteAll(node->data);
		}
	}
};

int main()
{
	srand(time(0));

	RenderWindow window(VideoMode(800, 800), "123");
	g_Font.loadFromFile("arial.ttf");

	TreeDrawer<int>* drawer = new TreeDrawer<int>(&window); // :(

	bool moving = false;
	sf::Vector2f oldPos;
	float zoom = 3.f;
	sf::View view = window.getDefaultView();
	view.setSize(window.getDefaultView().getSize());
	view.zoom(zoom);
	window.setView(view);

	int txt = 0;
	while (window.isOpen())
	{
		Event event;
		while (window.pollEvent(event))
		{
			if (event.type == Event::Closed) {
				window.close();
			}
			else if (event.type == sf::Event::MouseButtonPressed) {
				if (event.mouseButton.button == 0) {
					moving = true;
					oldPos = window.mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y));
					drawer->OnClicked(oldPos);
					// cout << oldPos.x << " " << oldPos.y << endl;
				}
			}
			else if (event.type == sf::Event::MouseButtonReleased) {
				if (event.mouseButton.button == 0) {
					moving = false;
				}
			}
			else if (event.type == sf::Event::MouseMoved) {
				if (!moving)
					break;
				sf::Vector2f newPos = window.mapPixelToCoords(sf::Vector2i(event.mouseMove.x, event.mouseMove.y));
				sf::Vector2f deltaPos = oldPos - newPos;
				view.setCenter(view.getCenter() + deltaPos);
				window.setView(view);
				oldPos = window.mapPixelToCoords(sf::Vector2i(event.mouseMove.x, event.mouseMove.y));
			}
			else if (event.type == sf::Event::MouseWheelScrolled) {
				if (moving)
					break;
				if (event.mouseWheelScroll.delta <= -1)
					zoom = zoom + 0.5f;
				else if (event.mouseWheelScroll.delta >= 1)
					zoom = std::max(0.5f, zoom - 0.5f);
				view.setSize(window.getDefaultView().getSize());
				view.zoom(zoom);
				window.setView(view);
			}
			else if (event.type == Event::TextEntered) {
				drawer->text_entered(event);
			}
		}
		window.clear(Color(50, 50, 50, 255));

		drawer->windowZoomInternal = zoom;
		drawer->Frame();

		window.display();
		++g_FrameCount;
	}
	return 0;
}
