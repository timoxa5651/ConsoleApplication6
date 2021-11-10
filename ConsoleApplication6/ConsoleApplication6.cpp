#include <any>
#define FMT_HEADER_ONLY
#include "fmt/format.h"
#include "SFML/Graphics.hpp"
#include <string>
#include <iostream>
#include <cassert>

using namespace sf;
using namespace std;

struct string_internal;
struct button_collection;
static Font g_Font;
static struct editor* g_Editor;

struct editor {
	Vector2i _cursor;
	RenderWindow* window;
	Vector2f window_size;
	Vector2f panel_pos;
	Vector2f panel_size;
	button_collection* actions;
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

enum class input_type {
	text_field,
	int_field,
	char_field
};
struct input {
	string name;
	input_type type;
	String value;

	input(string nm, input_type tp) {
		this->name = nm;
		this->type = tp;
		this->value = String();
	}

	bool can_enter(Uint32 chr) {
		if (type == input_type::int_field && (chr < L'0' || chr > L'9')) {
			return false;
		}
		if (type == input_type::char_field && this->value.getSize() > 0) {
			return false;
		}
		return true;
	}
};
struct button {
	string name;
	void(*call_fn)(editor*, vector<any>);
	vector<input> inputs;
	int input_active;

	button() {
		this->input_active = 0;
		this->call_fn = NULL;
	}

	button(string nme, void(*_call_fn)(editor*, vector<any>), vector<input> inp) : button() {
		this->name = nme;
		this->call_fn = _call_fn;
		this->inputs = inp;
	}

	void draw(editor* ed, Vector2f pos, Vector2f size) {
		Text text = Text();
		text.setFont(g_Font);
		text.setCharacterSize(20);
		text.setFillColor(Color::Black);
		text.setString(String(name));
		FloatRect bRect = text.getLocalBounds();
		text.setPosition(pos.x + size.x / 2 - (int)bRect.width / 2, pos.y + size.y / 2 - (int)bRect.height / 2);
		ed->window->draw(text);
	}

	void draw_active(editor* ed, Vector2f pos, Vector2f size) {
		RectangleShape rectangle(Vector2f(size.x, size.y));
		rectangle.move(pos.x, pos.y);
		rectangle.setOutlineColor(Color(0, 0, 0, 255));
		rectangle.setOutlineThickness(1.2f);
		rectangle.setFillColor(Color(255 * 0.8, 255 * 0.8, 255 * 0.8, 255 * 0.8));
		ed->window->draw(rectangle);

		pos.y += 30;
		for (int idx = 0; idx < this->inputs.size(); ++idx) {
			input& inp = this->inputs[idx];

			sf::Text text;
			text.setFont(g_Font);
			text.setCharacterSize(14);
			text.setFillColor(Color::Black);
			text.setString(inp.name);
			FloatRect bRect = text.getLocalBounds();
			text.setPosition(pos.x + size.x / 2 - (int)bRect.width / 2, pos.y + idx * 45 - 20);
			ed->window->draw(text);

			RectangleShape rect(Vector2f(size.x - 20, 20));
			rect.setPosition(pos.x + 10, pos.y + idx * 45);
			rect.setOutlineColor(Color(0, 0, 0, 255));
			rect.setOutlineThickness(1.f);
			ed->window->draw(rect);

			text.setString(inp.value);
			bRect = text.getLocalBounds();
			text.setPosition(pos.x + 10 + (size.x - 20) / 2 - (int)bRect.width / 2, pos.y + idx * 45);
			ed->window->draw(text);

			if (idx == this->input_active) {
				float bhei = text.getCharacterSize() + 3;
				float bx = text.findCharacterPos(text.getString().getSize()).x - text.findCharacterPos(0).x + 2;

				Vertex vertices[2];
				vertices[0] = Vertex(pos + Vector2f(10 + (size.x - 20) / 2 - (int)bRect.width / 2 + bx, idx * 45 + 2));
				vertices[1] = Vertex(pos + Vector2f(10 + (size.x - 20) / 2 - (int)bRect.width / 2 + bx, idx * 45 + bhei));
				vertices[1].color = vertices[0].color = Color(0, 0, 0, 255);
				ed->window->draw(vertices, 2, Lines);
			}
		}
	}

	bool text_entered(editor* ed, Event evnt) {
		if (input_active < 0) {
			input_active = 0;
		}

		if (input_active >= 0) {
			input& inp = this->inputs[input_active];

			if (evnt.text.unicode == 13) { // enter
				if (this->input_active + 1 == this->inputs.size()) {
					vector<any> args;
					for (input& in : this->inputs) {
						auto ps = in.value.toUtf16();
						wstring wstr = wstring(ps.begin(), ps.end());

						if (in.type == input_type::int_field) {
							args.push_back(wstr.size() ? stoi(wstr) : 0);
						}
						else {
							args.push_back(wstr);
						}
						in.value = "";
					}
					this->call_fn(ed, args);
					this->input_active = 0;
					return true;
				}
				else {
					this->input_active += 1;
				}
			}
			else if (evnt.text.unicode == 8) { // backspace
				if (inp.value.getSize()) {
					inp.value = inp.value.substring(0, inp.value.getSize() - 1);
				}
			}
			else if (inp.can_enter(evnt.text.unicode)) {
				inp.value += evnt.text.unicode;
			}
		}
		return false;
	}
};
struct button_collection {
	vector<button> buttons;
	int active_button_popup;

	void init();
	void draw(editor* ed, Vector2f pos, Vector2f size);
	void on_click(editor* ed, Event event);
	void text_entered(editor* ed, Event evnt);
	bool blocks_input();
};


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
		else if (this->next_string) {
			this->text.setString(this->text.getString() + this->next_string->text.getString());
			g_Editor->remove_string(this->next_string->line);
		}
	}

	void add(String chr) {
		auto bString = this->text.getString();
		this->text.setString(bString.substring(0, this->cursor) + chr + bString.substring(this->cursor));
		this->cursor += chr.getSize();

		while (this->get_size().x > g_Editor->window_size.x) {
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

	void replace(String prev, String next) {
		if (!prev.getSize())
			return;
		String cur = this->text.getString();
		do {
			size_t idx = cur.find(prev);
			if (idx == string::npos) {
				break;
			}
			cur.erase(idx, prev.getSize());
			cur.insert(idx, next);
		} while (cur.getSize());
		this->text.setString(cur);
		this->set_cursor(this->cursor);
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
	this->panel_size = Vector2f(150, this->window_size.y);
	this->data.reserve(128);
	this->data.push_back(new string_internal());
	this->_cursor = Vector2i(0, 0);
	this->actions = new button_collection();
	this->actions->init();
	this->panel_pos = Vector2f(this->window_size.x - this->panel_size.x, 0);
	this->window_size.x -= this->panel_size.x;
}

Vector2i editor::get_cursor() {
	return this->_cursor;
}
void editor::set_cursor(Vector2i new_cursor, bool alloc_new) {
	new_cursor.x = max(new_cursor.x, 0);
	new_cursor.y = max(new_cursor.y, 0);

	while (alloc_new && new_cursor.x >= this->data.size()) {
		this->insert_string(new string_internal(), this->data.size());
	}
	new_cursor.x = min(new_cursor.x, (int)this->data.size() - 1);
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
			this->window->draw(vertices, 2, Lines);
		}
	}

	Vertex vertices[2];
	vertices[0] = Vertex(Vector2f(this->panel_pos.x, 0));
	vertices[1] = Vertex(Vector2f(this->panel_pos.x, this->panel_size.y));
	vertices[1].color = vertices[0].color = Color(0, 0, 0, 255);
	this->window->draw(vertices, 2, Lines);
	this->actions->draw(this, this->panel_pos, this->panel_size);
}

void editor::insert_string(string_internal* new_string, int position) {
	if (position < 0 || position > this->data.size()) {
		return;
	}

	this->data.insert(this->data.begin() + position, new_string);
	for (size_t idx = max(0, position - 1); idx < this->data.size(); ++idx) {
		this->data[idx]->line = idx;
		if (idx + 1 < this->data.size()) {
			this->data[idx]->next_string = this->data[idx + 1];
		}
		else {
			this->data[idx]->next_string = nullptr;
		}
	}
	this->set_cursor(this->get_cursor());
}

void editor::remove_string(int position) {
	if (position < 0 || position >= this->data.size()) {
		return;
	}
	if (this->data.size() == 1) {
		this->data[0]->set("");
	}
	else {
		this->data.erase(this->data.begin() + position);
	}
	
	for (size_t idx = max(0, position - 1); idx < this->data.size(); ++idx) {
		this->data[idx]->line = idx;

		if (idx + 1 < this->data.size()) {
			this->data[idx]->next_string = this->data[idx + 1];
		}
		else {
			this->data[idx]->next_string = nullptr;
		}
	}
	this->set_cursor(this->get_cursor(), false);
}

void editor::text_entered(Event evnt) {
	if (this->actions->blocks_input()) {
		this->actions->text_entered(this, evnt);
		return;
	}
	Vector2i cursor = this->get_cursor();
	string_internal* current_string = this->data[cursor.x];
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
				int tCursor = this->data[cursor.x - 1]->length();
				if (current_string->length()) {
					this->data[cursor.x - 1]->add(current_string->text.getString());
				}
				this->set_cursor(Vector2i(cursor.x - 1, tCursor));
				this->remove_string(cursor.x);
				this->data[this->get_cursor().x]->cursor = this->get_cursor().y;
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
	if (this->actions->blocks_input())
		return;
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


void button_collection::init() {
	this->buttons.push_back(button(
		"Paste: 1 line",
		[](editor* instance, vector<any> params) {
			string_internal* str = new string_internal();
			str->set(any_cast<wstring>(params[0]));
			int index = min(max(0, any_cast<int>(params[1])), (int)instance->data.size());
			instance->insert_string(str, index);
			instance->set_cursor(Vector2i(index, str->cursor));
		},
		{ {"Text", input_type::text_field}, {"After Nth line", input_type::int_field} }
		));

	this->buttons.push_back(button(
		"Paste: N lines",
		[](editor* instance, vector<any> params) {
			int index = min(max(0, any_cast<int>(params[1])), (int)instance->data.size());
			int cnt = any_cast<int>(params[2]);
			if (cnt < 1) {
				return;
			}
			wstring wstr = any_cast<wstring>(params[0]);
			for (int i = 0; i < cnt; ++i) {
				string_internal* str = new string_internal();
				str->set(wstr);
				instance->insert_string(str, index);
			}
			instance->set_cursor(Vector2i(index + cnt - 1, wstr.size()));
		},
		{ {"Text", input_type::text_field}, {"After Nth line", input_type::int_field}, {"Amount", input_type::int_field} }
		));

	this->buttons.push_back(button(
		"Delete: 1 line",
		[](editor* instance, vector<any> params) {
			Vector2i cursor = instance->get_cursor();
			instance->remove_string(any_cast<int>(params[0]) - 1);
			instance->set_cursor(cursor, false);
		},
		{ {"Nth line", input_type::int_field} }
	));

	this->buttons.push_back(button(
		"Insert: 1 line",
		[](editor* instance, vector<any> params) {
			int index = min(max(1, any_cast<int>(params[1])), (int)instance->data.size()) - 1;
			int idx = any_cast<int>(params[2]);

			instance->data[index]->set_cursor(idx);
			instance->data[index]->add(any_cast<wstring>(params[0]));
			instance->set_cursor(Vector2i(index, instance->data[index]->cursor));
		},
		{ {"Text", input_type::text_field}, {"In Nth line", input_type::int_field}, {"After Mth position", input_type::int_field} }
	));

	this->buttons.push_back(button(
		"Replace: 1 char",
		[](editor* instance, vector<any> params) {
			int index = min(max(1, any_cast<int>(params[0])), (int)instance->data.size()) - 1;
			int idx = max(min(instance->data[index]->length(), any_cast<int>(params[1])), 1) - 1;

			instance->data[index]->set_cursor(idx);
			instance->data[index]->backspace_back();
			instance->data[index]->add(any_cast<wstring>(params[2]));
			instance->set_cursor(Vector2i(index, instance->data[index]->cursor));
		},
		{ {"In Nth line", input_type::int_field}, {"In Mth position", input_type::int_field}, {"New character", input_type::char_field} }
	));

	this->buttons.push_back(button(
		"Replace: string",
		[](editor* instance, vector<any> params) {
			int p1 = any_cast<int>(params[2]);
			int p2 = any_cast<int>(params[3]);
			wstring p3 = any_cast<wstring>(params[0]);
			wstring p4 = any_cast<wstring>(params[1]);

			if (p1 <= 0 || p2 <= 0 || p1 > instance->data.size() || p2 > instance->data.size()) {
				p1 = 1;
				p2 = instance->data.size();
			}
			int index = min(max(1, p1), (int)instance->data.size()) - 1;
			int index2 = min(max(1, p2), (int)instance->data.size()) - 1;

			for (int idx = index; idx <= index2; ++idx) {
				instance->data[idx]->replace(p3, p4);
			}
		},
		{ {"Old string", input_type::text_field}, {"New string", input_type::text_field}, {"From Nth line", input_type::int_field}, {"To Mth line", input_type::int_field} }
	));

	this->active_button_popup = -1;
}

void button_collection::draw(editor* ed, Vector2f pos, Vector2f size) {
	float dMove = size.y / this->buttons.size();
	Vector2i mouse = Mouse::getPosition(*ed->window);

	for (int i = 0; i < this->buttons.size(); ++i) {
		Vertex vertices[2];
		vertices[0] = Vertex(pos + Vector2f(0, (i + 1) * dMove));
		vertices[1] = Vertex(pos + Vector2f(size.x, (i + 1) * dMove));
		vertices[1].color = vertices[0].color = Color(0, 0, 0, 255);
		ed->window->draw(vertices, 2, Lines);

		if (this->active_button_popup < 0 && mouse.x > pos.x && mouse.x < pos.x + size.x && mouse.y > pos.y + i * dMove && mouse.y < pos.y + (i + 1) * dMove) { // #NoRect
			Vertex vertices[4];
			vertices[0] = Vertex(pos + Vector2f(0, i * dMove));
			vertices[1] = Vertex(pos + Vector2f(size.x, i * dMove));
			vertices[2] = Vertex(pos + Vector2f(size.x, (i + 1) * dMove));
			vertices[3] = Vertex(pos + Vector2f(0, (i + 1) * dMove));
			vertices[3].color = vertices[2].color = vertices[1].color = vertices[0].color = Color(0, 0, 0, 255 * 0.2);
			ed->window->draw(vertices, 4, Quads);
		}

		this->buttons[i].draw(ed, pos + Vector2f(0, i * dMove), Vector2f(size.x, dMove));
	}

	if (this->active_button_popup >= 0) {
		Vector2f fCenter = Vector2f(ed->window->getSize().x / 2, ed->window->getSize().y / 2);
		const Vector2f window_size = Vector2f(300, 300);
		this->buttons[this->active_button_popup].draw_active(ed, fCenter - Vector2f(window_size.x / 2, window_size.y / 2), window_size);
	}
}

void button_collection::on_click(editor* ed, Event event) {
	float dMove = ed->panel_size.y / this->buttons.size();
	if (this->active_button_popup >= 0) {
		return;
	}
	Vector2f pos = ed->panel_pos;
	Vector2f size = ed->panel_size;
	Vector2i mouse = Mouse::getPosition(*ed->window);

	for (int i = 0; i < this->buttons.size(); ++i) {
		if (mouse.x >= pos.x && mouse.x <= pos.x + size.x && mouse.y >= pos.y + i * dMove && mouse.y <= pos.y + (i + 1) * dMove) { // #NoRect
			this->active_button_popup = i;
			break;
		}
	}
}

void button_collection::text_entered(editor* ed, Event evnt) {
	if (this->active_button_popup >= 0) {
		if (this->buttons[this->active_button_popup].text_entered(ed, evnt)) {
			this->active_button_popup = -1;
		}
	}
}

bool button_collection::blocks_input() {
	return this->active_button_popup >= 0;
}

int main()
{
	g_Font.loadFromFile("arial.ttf");
	RenderWindow window(VideoMode(800, 900), "123");

	g_Editor = new editor(&window);
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
					g_Editor->actions->on_click(g_Editor, event);
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
