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

Font font;

class WndClass {
	RenderWindow* wnd;
	Vector2f size;
public:
	WndClass(RenderWindow* wnd, Vector2f size) {
		this->wnd = wnd;
		this->size = size;
	}

	void frame() {

	}

	void on_mousedown(Vector2f pos) {

	}

	void on_mouseup(Vector2f pos) {

	}
};

int main()
{
	RenderWindow window(VideoMode(800, 800), "123");
	WndClass* wnd = new WndClass(&window, Vector2f(800, 800));

	font.loadFromFile("arial.ttf");

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
		}
		window.clear(Color(20, 20, 20, 255));

		wnd->frame();
		
		window.display();
	}

	return 0;
}
