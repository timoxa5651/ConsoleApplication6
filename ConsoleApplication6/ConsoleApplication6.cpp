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
	bool ignore;

	shape() : ignore(false) {};
	virtual pair<double, double> calc(double x) { // :(
		return { FLT_MAX, FLT_MAX };
	}
	virtual bool good(double x, double y) {
		return false;
	}
	virtual bool connect() {
		return true;
	}
};

struct parabola : public shape {
public:
	double a, b, c; //y = ax^2 + bx + c
	double ox;

	parabola(double ka, double kb, double kc, double kx, bool in) {
		this->a = ka;
		this->b = kb;
		this->c = kc;
		this->ox = kx;
		this->inside = in;
		if (this->a < 0) {
			this->inside = !this->inside;
		}
	}

	virtual pair<double, double> calc(double x) final {
		x -= this->ox;
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

struct rhomb : public shape {
public:
	double cx0, cy0, ca, cb;

	rhomb(double kx, double ky, double ka, double kb, bool in) {
		this->cx0 = kx;
		this->cy0 = ky;
		this->ca = ka;
		this->cb = kb;
		this->inside = in;
	}

	virtual bool connect() {
		return false;
	}

	virtual pair<double, double> calc(double x) final {
		double rs = this->cb * (1 - abs(x - this->cx0) / this->ca);
		if (rs < 0.f) {
			return { FLT_MAX, FLT_MAX };
		}
		double r1 = this->cy0 - rs;
		double r2 = this->cy0 + rs;

		return { r1, r2 };
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
		
	}

	void begin_frame() {
		this->frame_filled.clear();
		this->frame_filled.resize(this->size, vector<bool>(this->size, false));
	}

	void end_frame() {
		const float fill_color[4] = { 1.f, 1.f, 1.f, 0.2f };

		Uint8* pixels = new Uint8[this->size * this->size * 4];
		memset(pixels, 0, this->size * this->size * 4);

		Texture texture;
		texture.create(this->size, this->size);

		for (int x = 0; x < this->size; ++x) {
			for (int y = 0; y < this->size; ++y) {
				int p = (x * this->size + y) * 4;
				if (this->frame_filled[x][y]) {
					pixels[p] = (Uint8)(fill_color[0] * 255.f);
					pixels[p + 1] = (Uint8)(fill_color[1] * 255.f);
					pixels[p + 2] = (Uint8)(fill_color[2] * 255.f);
					pixels[p + 3] = (Uint8)(fill_color[3] * 255.f);
				}
			}
		}

		texture.update(pixels);
		this->window->draw(Sprite(texture));
		delete[] pixels;

		Vertex vertices[2];
		Vector2f zero = this->plt_to_wnd(Vector2f(0, 0));
		RectangleShape rectangle(Vector2f(2.f, this->size));
		rectangle.move(Vector2f(zero.x, 0));
		rectangle.setFillColor(Color(175, 180, 240, 255));
		this->window->draw(rectangle);

		rectangle = RectangleShape(Vector2f(this->size, 2.f));
		rectangle.move(Vector2f(0, zero.y));
		rectangle.setFillColor(Color(175, 180, 240, 255));
		this->window->draw(rectangle);
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
			if (sh->ignore) {
				continue;
			}
			for (int x = 0; x <= pl->size; ++x) {
				float px = pl->wnd_to_plot(Vector2f(x, 0)).x;
				pair<double, double> solutions = sh->calc(px);
				if (solutions.first < FLT_MAX) {
					Vector2f spos = pl->plt_to_wnd(Vector2f(px, solutions.first));

					//if (spos.x >= 0 && spos.x < pl->size && spos.y >= 0 && spos.y < pl->size) {
					Vertex vertex;
					vertex.position = Vector2f(spos.x, spos.y);
					vertex.color = Color(outline_color[0] * 255.f, outline_color[1] * 255.f, outline_color[2] * 255.f, outline_color[3] * 255.f);
					vt_buffer[0].push_back(vertex);
					//}
				}

				if (solutions.second < FLT_MAX) {
					Vector2f spos = pl->plt_to_wnd(Vector2f(px, solutions.second));

					//if (spos.x >= 0 && spos.x < pl->size && spos.y >= 0 && spos.y < pl->size) {
					Vertex vertex;
					vertex.position = Vector2f(spos.x, spos.y);
					vertex.color = Color(outline_color[0] * 255.f, outline_color[1] * 255.f, outline_color[2] * 255.f, outline_color[3] * 255.f);
					vt_buffer[1].push_back(vertex);
					//}
				}
			}

			if (vt_buffer[0].size())
				pl->window->draw(vt_buffer[0].data(), vt_buffer[0].size(), PrimitiveType::LinesStrip);
			if (vt_buffer[1].size())
				pl->window->draw(vt_buffer[1].data(), vt_buffer[1].size(), PrimitiveType::LinesStrip);
			if (vt_buffer[0].size() && vt_buffer[1].size() && sh->connect()) {
				Vertex vt[2] = { vt_buffer[0][vt_buffer[0].size() - 1], vt_buffer[1][vt_buffer[1].size() - 1] };
				if(vt[0].position.x != pl->size && vt[1].position.x != pl->size)
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
	RenderWindow window(VideoMode(800, 800), "123");

	font.loadFromFile("arial.ttf");

	plot* pl = new plot(&window);
	area area1 = area();
	area1.add(new parabola(-0.5f, 0, -0.5f, 3.5f, true));
	area1.add(new circle(4.f, -1.f, 2.f, true));
	area1.add(new rhomb(1.5, 0.5f, 10.f / 2, 2.5f, false));
	area1.add(new parabola_horizontal(-1.f, 3.f, -1.f, false));
	areas.push_back(area1);

	area area2 = area();
	area2.add(new parabola_horizontal(-1.f, 3.f, -1.f, false));
	area2.add(new rhomb(1.5, 0.5f, 10.f / 2, 2.5f, false));
	area2.add(new line(0.25f, 2.5f, false));
	area2.add(new rectangle(0.5f, 1.f, 4.5f, 2.5f, false));
	area2.add(new parabola(-0.5f, 0, -0.5f, 3.5f, false));
	area2.add(new circle(4.f, -1.f, 2.f, false));
	area2.add(new line(-0.75f, -3.5f, true));
	auto high_iq = new rectangle(-2.f, -4.5f, 2.f, -2.f, false);
	auto high_iq2 = new rectangle(0.f, 2.f, 1.f, 3.f, false);
	high_iq2->ignore = high_iq->ignore = true;
	area2.add(high_iq);
	area2.add(high_iq2);
	areas.push_back(area2);

	area area3 = area();
	area3.add(new rhomb(1.5, 0.5f, 10.f / 2, 2.5f, true));
	area3.add(new parabola_horizontal(-1.f, 3.f, -1.f, true));
	area3.add(new parabola(-0.5f, 0, -0.5f, 3.5f, true));
	areas.push_back(area3);

	area area4 = area();
	area4.add(new rectangle(0.5f, 1.f, 4.5f, 2.5f, false));
	area4.add(new circle(4.f, -1.f, 2.f, false));
	area4.add(new parabola_horizontal(-1.f, 3.f, -1.f, false));
	area4.add(new rhomb(1.5, 0.5f, 10.f / 2, 2.5f, true));
	auto pline1 = new line(0.f, 2.5f, false);
	auto pline2 = new rectangle(-6.f, -6.f, 0.f, 6.f, false);
	auto pline3 = new rectangle(4.5f, 0.f, 10.f, 2.5f, false);
	pline1->ignore = pline2->ignore = pline3->ignore = true;
	area4.add(pline1);
	area4.add(pline2);
	area4.add(pline3);
	areas.push_back(area4);

	area area5 = area();
	area5.add(new line(-0.75f, -3.5f, false));
	area5.add(new parabola(-0.5f, 0, -0.5f, 3.5f, false));
	area5.add(new parabola_horizontal(-1.f, 3.f, -1.f, false));

	areas.push_back(area5);

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
			else if (event.type == Event::MouseWheelScrolled)
			{
				if (event.mouseWheelScroll.delta > 0) {
					pl->current_scale *= 1.07f;
				}
				else {
					pl->current_scale /= 1.05f;
				}
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

		Vector2f prev_offset = pl->offset;
		if (_moving) {
			pl->offset -= (Vector2f(Mouse::getPosition(window)) - _start);
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