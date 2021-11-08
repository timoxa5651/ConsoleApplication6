#define FMT_HEADER_ONLY
#include "fmt/format.h"
#include "SFML/Graphics.hpp"
#include <string>
#include <iostream>
#include <cassert>
#include <optional>

using namespace sf;
using namespace std;

struct string_internal;
struct editor {
	Vector2i _cursor;
	RenderWindow* window;
	Vector2f window_size;
	Vector2f panel_size;
	vector<string_internal*> data;

	editor(RenderWindow* window);
	Vector2i get_cursor();

	void set_cursor(Vector2i new_cursor, bool alloc_new = true);
	void frame();
	void insert_string(string_internal* new_string, int position);
	void remove_string(int position);
	void text_entered(Event evnt);
	void key_down(Event evnt);
};

static Font g_Font;
static editor* g_Editor;

struct string_internal {
	sf::Text text;
	int line;
	int cursor;
	string_internal* next_string;

	string_internal() {
		this->line = -1;
		this->text = sf::Text();
		this->text.setFont(g_Font);
		this->text.setCharacterSize(14);
		this->text.setFillColor(sf::Color::Black);
		this->text.setString(sf::String(""));
		this->cursor = 0;
		this->next_string = NULL;
	}
	void set_cursor(int new_cursor) {
		new_cursor = max(min(this->length(), new_cursor), 0);
		this->cursor = new_cursor;
	}

	void backspace() {
		if (this->length() && this->cursor > 0) {
			this->cursor -= 1;
			auto bString = this->text.getString();
			if (this->cursor + 1 < this->length()) {
				this->text.setString(bString.substring(0, this->cursor) + bString.substring(this->cursor + 1));
			}
			else {
				this->text.setString(bString.substring(0, this->cursor));
			}
		}
	}

	void backspace_back() {
		if (this->length() - this->cursor > 0) {
			auto bString = this->text.getString();
			this->text.setString(bString.substring(0, this->cursor) + bString.substring(this->cursor + 1));
		}
	}

	void add(String chr) {
		auto bString = this->text.getString();
		this->text.setString(bString.substring(0, this->cursor) + chr + bString.substring(this->cursor));
		this->cursor += chr.getSize();

		if (this->get_size().x > g_Editor->window_size.x) {
			bool is_last_char = this->cursor == this->length();
			int prev_cursor = this->cursor;

			this->set_cursor(this->length());
			Uint32 removed_char = this->text.getString()[this->length() - 1];
			this->backspace();
			this->set_cursor(prev_cursor);

			if (is_last_char) {
				g_Editor->set_cursor(Vector2i(g_Editor->_cursor.x + 1, 0));
			}

			if (!next_string) {
				next_string = new string_internal();
				g_Editor->insert_string(next_string, g_Editor->data.size());
			}
			prev_cursor = next_string->cursor;
			next_string->set_cursor(0);
			next_string->add(removed_char);
			next_string->set_cursor(prev_cursor);
		}
	}

	void set(String chr) {
		this->text.setString(chr);
		this->cursor = chr.getSize();
	}

	void add(Uint32 chr) {
		return add(String(chr));
	}

	int length() {
		return (int)text.getString().getSize();
	}

	Vector2f get_size() {
		sf::FloatRect rect = text.getLocalBounds();
		return Vector2f(rect.width, rect.height);
	}

	void draw(RenderWindow* window, Vector2f position) {
		this->text.setPosition(position + Vector2f(0, (text.getCharacterSize() + 5) * this->line));
		window->draw(this->text);
	}
};

editor::editor(RenderWindow* window) {
	this->window = window;
	this->window_size.x = window->getSize().x;
	this->window_size.y = window->getSize().y;
	this->data.reserve(100);
	this->data.push_back(new string_internal());
	this->_cursor = Vector2i(0, 0);
}

Vector2i editor::get_cursor() {
	return this->_cursor;
}
void editor::set_cursor(Vector2i new_cursor, bool alloc_new) {
	if (new_cursor.x >= this->data.size() && !alloc_new)
		return;
	new_cursor.x = max(new_cursor.x, 0);
	new_cursor.y = max(new_cursor.y, 0);

	while (new_cursor.x >= this->data.size()) {
		this->insert_string(new string_internal(), this->data.size());
	}
	new_cursor.y = min(new_cursor.y, this->data[new_cursor.x]->length());
	this->_cursor = new_cursor;
}

void editor::frame() {
	Vector2i cursor = this->get_cursor();
	for (int idx = 0; idx < this->data.size(); ++idx) {
		string_internal* sdata = this->data[idx];
		sdata->line = idx;
		sdata->draw(this->window, Vector2f(0, 0));

		if (cursor.x == idx) {
			float bhei = (sdata->text.getCharacterSize() + 5);
			float bx = sdata->text.findCharacterPos(sdata->cursor).x - sdata->text.findCharacterPos(0).x + 2;
			float by = bhei * sdata->line;
			Vertex vertices[2];
			vertices[0] = Vertex(Vector2f(bx, by));
			vertices[1] = Vertex(Vector2f(bx, by + bhei));
			vertices[1].color = vertices[0].color = Color(0, 0, 0, 255);
			window->draw(vertices, 2, Lines);
		}
	}
}

void editor::insert_string(string_internal* new_string, int position) {
	this->data.insert(this->data.begin() + position, new_string);
	for (int idx = max(0, position - 1); idx < this->data.size(); ++idx) {
		this->data[idx]->line = idx;
		if (idx + 1 < this->data.size()) {
			this->data[idx]->next_string = this->data[idx + 1];
		}
		else {
			this->data[idx]->next_string = nullptr;
		}	
	}
}
void editor::remove_string(int position) {
	this->data.erase(this->data.begin() + position);
	for (int idx = max(0, position - 1); idx < this->data.size(); ++idx) {
		this->data[idx]->line = idx;

		if (idx + 1 < this->data.size()) {
			this->data[idx]->next_string = this->data[idx + 1];
		}
		else {
			this->data[idx]->next_string = nullptr;
		}
	}
}

void editor::text_entered(Event evnt) {
	Vector2i cursor = this->get_cursor();
	string_internal* current_string = this->data[cursor.x];
	cursor.y = min(cursor.y, current_string->length());
	current_string->set_cursor(cursor.y);

	unsigned int key = evnt.text.unicode;
	if (key == 13) { //enter
		String left = current_string->text.getString().substring(0, current_string->cursor);
		String right = current_string->text.getString().substring(current_string->cursor);
		current_string->set(left);

		string_internal* new_string = new string_internal();
		new_string->set(right);
		new_string->set_cursor(0);
		this->insert_string(new_string, cursor.x + 1);
		this->set_cursor(Vector2i(cursor.x + 1, new_string->cursor));
	}
	else if (key == 8) { //backspace
		if (this->get_cursor().y) {
			current_string->backspace();
			this->set_cursor(Vector2i(this->get_cursor().x, current_string->cursor));
		}
		else {
			if (cursor.x > 0) {
				int tCursor = cursor.y;
				if (current_string->length()) {
					tCursor = this->data[cursor.x - 1]->length();
					this->data[cursor.x - 1]->add(current_string->text.getString());
					this->data[cursor.x - 1]->set_cursor(tCursor);
				}
				this->remove_string(cursor.x);
				this->set_cursor(Vector2i(cursor.x - 1, tCursor));
			}
		}
	}
	else {
		current_string->add(evnt.text.unicode);

		this->set_cursor(Vector2i(this->get_cursor().x, current_string->cursor));
	}
	//cout << evnt.text.unicode << " ";
}

void editor::key_down(Event evnt) {
	Vector2i cursor = this->get_cursor();
	Vector2i next_cursor = cursor;
	string_internal* current_string = this->data[cursor.x];

	switch (evnt.key.code) {
	case Keyboard::Key::Up:
		next_cursor += Vector2i(-1, 0);
		this->set_cursor(next_cursor, false);
		cursor = this->get_cursor();
		this->data[cursor.x]->set_cursor(next_cursor.y);
		break;
	case Keyboard::Key::Down:
		next_cursor += Vector2i(1, 0);
		this->set_cursor(next_cursor, false);
		cursor = this->get_cursor();
		this->data[cursor.x]->set_cursor(next_cursor.y);
		break;
	case Keyboard::Key::Right:
		this->set_cursor(this->get_cursor() + Vector2i(0, 1));
		break;
	case Keyboard::Key::Left:
		this->set_cursor(this->get_cursor() + Vector2i(0, -1));
		break;
	case Keyboard::Key::Delete:
		current_string->backspace_back();
		break;
	}

	current_string->set_cursor(this->get_cursor().y);
}



int main()
{
	RenderWindow window(VideoMode(800, 600), "123");

	g_Font.loadFromFile("arial.ttf");

	g_Editor = new editor(&window);

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
			else if (event.type == Event::MouseButtonReleased) {
				if (event.mouseButton.button == 0) {

				}
			}
			else if (event.type == sf::Event::TextEntered) {
				g_Editor->text_entered(event);
			}
			else if (event.type == sf::Event::KeyPressed) {
				g_Editor->key_down(event);
			}
		}
		window.clear(Color(255, 255, 255, 255));

		g_Editor->frame();

		window.display();
	}

	return 0;
}