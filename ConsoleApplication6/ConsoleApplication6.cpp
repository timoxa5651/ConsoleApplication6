#include <iostream>
#include <thread>
#include "SFML/Graphics.hpp"


using std::cout;
using std::cin;
using std::endl;

using sf::RenderWindow;
using sf::VideoMode;
using sf::Event;
using sf::Vector2f;
using sf::Color;


static sf::Font g_Font;

int main()
{
	srand(time(0));

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
