#include <iostream>
#include <thread>
#include "SFML/Graphics.hpp"

#include "AVLTree.hpp"


using std::cout;
using std::cin;
using std::endl;
using std::remove_pointer_t;
using std::vector;


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
};

template<class T>
class TreeHolder {
public:
	T* tree;

	void ProcessOperations() {

	}
};

class Window {
	
};

int main()
{
	srand(time(0));

	AVLTree<>* tree = new AVLTree<>();
	tree->Insert(22);
	tree->Insert(21);
	tree->Insert(23);
	tree->Insert(11);
	tree->Insert(99);
	tree->InOrder([](remove_pointer_t<decltype(tree)>::NodeType* node) {
		cout << node->data << " ";
	});

	return 0;
	RenderWindow window(VideoMode(800, 800), "123");

	g_Font.loadFromFile("arial.ttf");

	while (window.isOpen())
	{
		Event event;
		while (window.pollEvent(event))
		{
			if (event.type == Event::Closed) {
				window.close();
			}
			else if (event.type == Event::MouseWheelScrolled)
			{
				
			}
			else if (event.type == Event::MouseButtonPressed) {
				if (event.mouseButton.button == 0) {
							
				}
			}
		}
		window.clear(Color(20, 20, 20, 255));

		
		window.display();
	}
	return 0;
}
