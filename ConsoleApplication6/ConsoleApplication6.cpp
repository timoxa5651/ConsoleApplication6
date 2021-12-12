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

class window {
	RenderWindow* wnd;
	Vector2f size;
public:
	window(RenderWindow* wnd, Vector2f size) {
		this->wnd = wnd;
		this->size = size;
	}

	void frame() {

	}
};

int main()
{
	RenderWindow window(VideoMode(800, 800), "123");

	font.loadFromFile("arial.ttf");

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
			else if (event.type == Event::MouseButtonPressed) {
				if (event.mouseButton.button == 0) {
					_moving = true;
					_start = Vector2f(Mouse::getPosition(window));
				}
			}
			else if (event.type == Event::MouseButtonReleased) {
				if (event.mouseButton.button == 0) {
					if (_moving) {
						pl->offset -= (Vector2f(Mouse::getPosition(window)) - _start);
					}
					_moving = false;
				}
			}
		}
		window.clear(Color(20, 20, 20, 255));


		pl->begin_frame();
		pl->draw_grid();
		for (area ar : areas) {
			ar.render(pl);
		}
		pl->end_frame();

		pl->offset = prev_offset;
		window.display();
	}

	return 0;
}
