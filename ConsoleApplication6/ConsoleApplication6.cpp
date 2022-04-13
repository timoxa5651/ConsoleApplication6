#include <iostream>
#include <thread>
#include "SFML/Graphics.hpp"

#include "AVLTree.hpp"

using std::cout;
using std::cin;
using std::endl;
using std::remove_pointer_t;
using std::vector;
using std::string;
using std::map;
using std::function;

using sf::RenderWindow;
using sf::VideoMode;
using sf::Event;
using sf::Vector2f;
using sf::Color;

static sf::Font g_Font;

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


		constexpr float circleSize = 30.f;
		constexpr float circleSpacing = 40.f;

		for (int i = 0; i < pow(2, this->grid.size() - 1); ++i) {
			this->xes[this->grid.size() - 1][i] = i * (circleSize + circleSpacing);
		}

		for (int level = this->grid.size() - 2; level >= 0; --level) {
			auto& definition = this->grid[level];
			for (int i = 0; i < pow(2, level); ++i) {
				float x1 = this->xes[level + 1][i * 2];
				float x2 = this->xes[level + 1][i * 2 + 1];

				this->xes[level][i] = (x1 + x2) / 2.f;
			}
		}
	}

	void Draw(RenderWindow* window) {
		if (this->hasChanges) {
			this->RebuildGrid();
			this->hasChanges = false;
		}
		
		constexpr float circleSize = 30.f;
		constexpr float circleSpacing = 40.f;
		auto draw_node = [&](Vector2f coords, PNode node, Vector2f parentPos) {
			sf::CircleShape shape(circleSize);
			shape.setFillColor(sf::Color(0, 0, 255));
			shape.setOutlineThickness(0.f);
			shape.setPosition(coords);
			window->draw(shape);

			if (parentPos.x != FLT_MAX) {
				sf::VertexArray lines(sf::Lines, 2);
				lines[0] = sf::Vertex(coords);
				lines[1] = sf::Vertex(parentPos);
				window->draw(lines);
			}
		};

		float wsize = window->getSize().x;
		int maxLvl = this->grid.size();
		float curSpacing = circleSpacing;
		float startSpacing = 0.f;
		float rowLen = pow(2, this->grid.size() - 1) * (circleSize + curSpacing) + circleSize;
		for (int level = this->grid.size() - 1; level >= 0; --level) {
			auto& definition = this->grid[level];
			auto GridToCoords = [&](int x, int y) {
				if (y < 0)
					return Vector2f(FLT_MAX, FLT_MAX);
				Vector2f coords = Vector2f(this->xes[y][x], y * 125);
				return coords;
			};

			for (auto& [xpos, node] : definition) {
				draw_node(GridToCoords(xpos, level), node, GridToCoords(xpos / 2, level - 1));
			}
		}
	}
};

template<typename T>
class TreeDrawer {
	vector<TreeOp<T>> opHistory;
	vector<TreeView<T, BaseTree<T>>*> treeViews;
	int activeViewIndex;
	RenderWindow* window;

public:
	TreeDrawer(RenderWindow* window) {
		this->window = window;

		this->treeViews.push_back(reinterpret_cast<decltype(treeViews)::value_type>(new TreeView<T, AVLTree<T>>()));
		this->activeViewIndex = 0;
	}

	void InsertAll(const T& val) {
		this->opHistory.push_back(TreeOp(TreeOpType::Insert, val));
		this->treeViews[this->activeViewIndex]->UpdateOperations(this->opHistory);
	}

	void Frame() {
		this->treeViews[this->activeViewIndex]->Draw(this->window);
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
	float zoom = 5.f;
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
					zoom = std::min(30.f, zoom + 0.2f);
				else if (event.mouseWheelScroll.delta >= 1)
					zoom = std::max(0.5f, zoom - 0.2f);
				view.setSize(window.getDefaultView().getSize());
				view.zoom(zoom);
				window.setView(view);
			}
		}
		window.clear(Color(20, 20, 20, 255));

		if(rand() % 500 == 0)
			drawer->InsertAll(rand());
		drawer->Frame();

		window.display();
	}
	return 0;
}
