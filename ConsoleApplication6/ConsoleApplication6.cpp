#define FMT_HEADER_ONLY
#include "fmt/format.h"
#include "SFML/Graphics.hpp"
#include <string>
#include <iostream>
#include <cassert>

using namespace sf;
using namespace std;

Font font;

struct shape {
public:
	bool inside;

	virtual pair<double, double> calc(double x) { // :(
		return { FLT_MAX, FLT_MAX };
	}
	virtual bool good(double x, double y) {
		return false;
	}
};

struct parabola : public shape {
public:
	double a, b, c; //y = ax^2 + bx + c

	parabola(double ka, double kb, double kc, bool in) {
		this->a = ka;
		this->b = kb;
		this->c = kc;
		this->inside = in;
		if (this->a < 0) {
			this->inside = !this->inside;
		}
	}

	virtual pair<double, double> calc(double x) final {
		return { this->a * x * x + this->b * x + this->c, FLT_MAX };
	}

	virtual bool good(double x, double y) final {
		if (inside) {
			return this->calc(x).first <= y;
		}
		return this->calc(x).first >= y;
	}
};

struct parabola_horizontal : public shape {
public:
	double a, c, yd; //a * x + c = y^2 + yd

	parabola_horizontal(double ka, double kc, double ky, bool in) {
		this->a = ka;
		this->c = kc;
		this->yd = ky;
		this->inside = in;
	}

	virtual pair<double, double> calc(double x) final {
		return { -sqrt(this->a * x + c) + this->yd, sqrt(this->a * x + c) + this->yd };
	}

	virtual bool good(double x, double y) final {
		auto sol = this->calc(x);
		bool flag = sol.first <= y && y <= sol.second;
		if (inside)
			return flag;
		else
			return !flag;
	}
};

struct line : public shape {
public:
	double k, b; //y = kx + b

	line(double k, double kb, bool side) {
		this->k = k;
		this->b = kb;
		this->inside = side;
	}

	virtual pair<double, double> calc(double x) final {
		return { this->k * x + this->b, FLT_MAX };
	}

	virtual bool good(double x, double y) final {
		if (inside) {
			return this->calc(x).first <= y;
		}
		return this->calc(x).first >= y;
	}
};

struct circle : public shape {
public:
	double cx, cy, r; // x^2 + y^2 = r^2

	circle(double kx, double ky, double kr, bool in) {
		this->cx = kx;
		this->cy = ky;
		this->r = kr;
		this->inside = in;
	}

	virtual pair<double, double> calc(double x) final {
		double rs = sqrtl(this->r * this->r - (x - this->cx) * (x - this->cx));
		return { -rs + this->cy, rs + this->cy };
	}

	virtual bool good(double x, double y) final {
		auto sol = this->calc(x);
		bool flag = sol.first <= y && y <= sol.second;
		if (inside)
			return flag;
		else
			return !flag;
	}
};

struct rectangle : public shape {
public:
	double x1, y1, x2, y2; // ...

	rectangle(double px1, double py1, double px2, double py2, bool in) {
		this->x1 = px1;
		this->x2 = px2;
		this->y1 = py1;
		this->y2 = py2;
		this->inside = in;
	}

	virtual pair<double, double> calc(double x) final {
		if (x >= this->x1 && x <= this->x2) {
			return { min(this->y1, this->y2), max(this->y1, this->y2) };
		}
		return { FLT_MAX, FLT_MAX };
	}

	virtual bool good(double x, double y) final {
		auto sol = this->calc(x);
		bool flag = sol.first <= y && y <= sol.second;
		if (inside)
			return flag;
		else
			return !flag;
	}
};

struct plot {
public:
	RenderWindow* window;
	float size;
	float current_scale;
	Vector2f offset;
	vector<vector<bool>> frame_filled;

	plot(RenderWindow* window) {
		this->window = window;
		this->size = window->getSize().x;
		this->current_scale = size / 6.f;
		this->current_scale /= 2.f;
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

	void draw_grid() {

		// x axis
		float xStart = wnd_to_plot(Vector2f(0, 0)).x;
		float xEnd = wnd_to_plot(Vector2f(this->size, 0)).x;
		if (xEnd < xStart)
			swap(xStart, xEnd);
		xStart = round(xStart);
		xEnd = round(xEnd);
		for (float cur = xStart; cur <= xEnd; cur += 1.f) {
			Vector2f wPos = plt_to_wnd(Vector2f(cur, 0));

			Vertex vertices[2];
			vertices[0] = Vertex(Vector2f(wPos.x, 0));
			vertices[1] = Vertex(Vector2f(wPos.x, this->size));
			vertices[0].color = Color(255, 255, 255, 30);
			vertices[1].color = Color(255, 255, 255, 30);
			window->draw(vertices, 2, Lines);
		}

		//y axis
		xStart = wnd_to_plot(Vector2f(0, 0)).y;
		xEnd = wnd_to_plot(Vector2f(0, this->size)).y;
		if (xEnd < xStart)
			swap(xStart, xEnd);
		xStart = round(xStart);
		xEnd = round(xEnd);
		for (float cur = xStart; cur <= xEnd; cur += 1.f) {
			Vector2f wPos = plt_to_wnd(Vector2f(0, cur));

			Vertex vertices[2];
			vertices[0] = Vertex(Vector2f(0, wPos.y));
			vertices[1] = Vertex(Vector2f(this->size, wPos.y));
			vertices[0].color = Color(255, 255, 255, 30);
			vertices[1].color = Color(255, 255, 255, 30);
			window->draw(vertices, 2, Lines);
		}

		//0, 0
		Vector2f pPos = plt_to_wnd(Vector2f(0, 0));
		if (pPos.x >= 0 && pPos.x < this->size && pPos.y >= 0 && pPos.y < this->size) {
			Text text;
			text.setFont(font);
			text.setString("(0,0)");
			text.setCharacterSize(10);
			text.setPosition(pPos);
			window->draw(text);
		}
		//radius
		
	}

	void begin_frame() {
		this->frame_filled.clear();
		this->frame_filled.resize(this->size, vector<bool>(this->size, false));
	}

	void end_frame() {
		const float fill_color[4] = { 1.f, 1.f, 1.f, 0.2f };

		sf::Uint8* pixels = new sf::Uint8[this->size * this->size * 4];
		memset(pixels, 0, this->size * this->size * 4);

		sf::Texture texture;
		texture.create(this->size, this->size);

		for (int x = 0; x < this->size; ++x) {
			for (int y = 0; y < this->size; ++y) {
				int p = (x * this->size + y) * 4;
				if (this->frame_filled[x][y]) {
					pixels[p] = (sf::Uint8)(fill_color[0] * 255.f);
					pixels[p + 1] = (sf::Uint8)(fill_color[1] * 255.f);
					pixels[p + 2] = (sf::Uint8)(fill_color[2] * 255.f);
					pixels[p + 3] = (sf::Uint8)(fill_color[3] * 255.f);
				}
			}
		}

		texture.update(pixels);
		this->window->draw(sf::Sprite(texture));
		delete[] pixels;
	}
};

struct area {
public:
	vector<shape*> fragments;

	bool good(double x, double y) {
		for (shape* sh : this->fragments) {
			if (!sh->good(x, y)) {
				return false;
			}
		}
		return true;
	}

	void add(shape* sh) {
		this->fragments.push_back(sh);
	}
	void add(shape** sh) {
		while (*sh)
			this->add(*sh++);
	}

	void render(plot* pl) {
		const float outline_color[4] = { 1.f, 1.f, 1.f, 1.f };

		for (int x = 0; x < pl->size; ++x) {
			for (int y = 0; y < pl->size; ++y) {
				if (!pl->frame_filled[x][y]) {
					int p = (x * pl->size + y) * 4;
					Vector2f coords = pl->wnd_to_plot(Vector2f(y, x));
					if (this->good(coords.x, coords.y)) {
						pl->frame_filled[x][y] = true;
					}
				}
			}
		}

		//outline

		vector<Vertex> vt_buffer[2];
		vt_buffer[0].reserve(pl->size);
		vt_buffer[1].reserve(pl->size);
		for (shape* sh : this->fragments) {
			for (int x = 0; x < pl->size; ++x) {
				float px = pl->wnd_to_plot(Vector2f(x, 0)).x;
				pair<double, double> solutions = sh->calc(px);
				if(solutions.first < FLT_MAX) {
					Vector2f spos = pl->plt_to_wnd(Vector2f(px, solutions.first));

					//if (spos.x >= 0 && spos.x < pl->size && spos.y >= 0 && spos.y < pl->size) {
						sf::Vertex vertex;
						vertex.position = Vector2f(spos.x, spos.y);
						vertex.color = Color(outline_color[0] * 255.f, outline_color[1] * 255.f, outline_color[2] * 255.f, outline_color[3] * 255.f);
						vt_buffer[0].push_back(vertex);
					//}
				}

				if (solutions.second < FLT_MAX) {
					Vector2f spos = pl->plt_to_wnd(Vector2f(px, solutions.second));

					//if (spos.x >= 0 && spos.x < pl->size && spos.y >= 0 && spos.y < pl->size) {
						sf::Vertex vertex;
						vertex.position = Vector2f(spos.x, spos.y);
						vertex.color = Color(outline_color[0] * 255.f, outline_color[1] * 255.f, outline_color[2] * 255.f, outline_color[3] * 255.f);
						vt_buffer[1].push_back(vertex);
					//}
				}
			}

			if(vt_buffer[0].size())
				pl->window->draw(vt_buffer[0].data(), vt_buffer[0].size(), PrimitiveType::LinesStrip);
			if(vt_buffer[1].size())
				pl->window->draw(vt_buffer[1].data(), vt_buffer[1].size(), PrimitiveType::LinesStrip);
			if (vt_buffer[0].size() && vt_buffer[1].size()) {
				Vertex vt[2] = { vt_buffer[0][vt_buffer[0].size() - 1], vt_buffer[1][vt_buffer[1].size() - 1] };
				pl->window->draw(vt, 2, PrimitiveType::LinesStrip);

				Vertex vt2[2] = { vt_buffer[0][0], vt_buffer[1][0] };
				pl->window->draw(vt2, 2, PrimitiveType::LinesStrip);
			}
			vt_buffer[0].clear();
			vt_buffer[1].clear();
		}
	}
};

int main()
{
	vector<area> areas;
	RenderWindow window(VideoMode(600, 600), "123");

	font.loadFromFile("arial.ttf");

	plot* pl = new plot(&window);
	area area1 = area();
	area1.add(new parabola(0.5f, 0, 0, true));
	area1.add(new line(1.5f, 0, true));
	area1.add(new parabola_horizontal(-1.f, 4, 0.f, false));
	area1.add(new circle(2.f, 2.f, 1.f, false));
	area1.add(new rectangle(-1.f, 2.f, 0.f, 3.f, false));

	for(int i = 0; i < 10; ++i)
		areas.push_back(area1);
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
			else if (event.type == sf::Event::MouseWheelScrolled)
			{
				if (event.mouseWheelScroll.delta > 0) {
					pl->current_scale *= 1.07f;
				}
				else {
					pl->current_scale /= 1.05f;
				}
			}
			else if (event.type == sf::Event::MouseButtonPressed) {
				if (event.mouseButton.button == 0) {
					_moving = true;
					_start = sf::Vector2f(sf::Mouse::getPosition(window));
				}
			}
			else if (event.type == sf::Event::MouseButtonReleased) {
				if (event.mouseButton.button == 0) {
					if (_moving) {
						pl->offset -= (sf::Vector2f(sf::Mouse::getPosition(window)) - _start);
					}
					_moving = false;
				}
			}
		}
		window.clear(Color(20, 20, 20, 255));

		Vector2f prev_offset = pl->offset;
		if (_moving) {
			pl->offset -= (sf::Vector2f(sf::Mouse::getPosition(window)) - _start);
		}

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