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

struct plot {
public:
	RenderWindow* window;
	float size;
	float current_scale;
	Vector2f offset;

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
		float _padd = abs(this->wnd_to_plot(Vector2f(this->size, this->size)).x - this->wnd_to_plot(Vector2f(0, 0)).x);
		for (float i = 0; i < this->size; i += _padd) {
			Vertex vertices[2];
			vertices[0] = Vertex(Vector2f(i, this->size));
			vertices[1] = Vertex(Vector2f(i, 0));
			window->draw(vertices, 2, Lines);
			/*if (i == 0 || i == lCnt / 2 || i == lCnt) {
				string str;
				if (i == lCnt / 2) {
					auto pnt = wnd_to_plot(Vector2f(i * (this->size / lCnt), this->size / 2));
					str = fmt::format("({:.1f} {:.1f})", pnt.x, pnt.y);
				}
				else {
					str = fmt::format("{:.1f}", wnd_to_plot(Vector2f(i * (this->size / lCnt), this->size / 2)).x);
				}

				Text text;
				text.setFont(font);
				text.setString(str.c_str());
				text.setCharacterSize(10);
				pPos.x = min(pPos.x, this->size - text.getLocalBounds().width - 2);
				text.setPosition(pPos);
				window->draw(text);
			}*/
		}


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
		const float fill_color[4] = {1.f, 1.f, 1.f, 0.2f};
		const float outline_color[4] = { 1.f, 1.f, 1.f, 1.f };

		sf::Uint8* pixels = new sf::Uint8[pl->size * pl->size * 4];
		memset(pixels, 0, pl->size * pl->size * 4);

		sf::Texture texture;
		texture.create(pl->size, pl->size);

		for (int x = 0; x < pl->size; ++x) {
			for (int y = 0; y < pl->size; ++y) {
				int p = (x * pl->size + y) * 4;
				Vector2f coords = pl->wnd_to_plot(Vector2f(y, x));
				if (this->good(coords.x, coords.y)) {
					pixels[p] =     (sf::Uint8)(fill_color[0] * 255.f);
					pixels[p + 1] = (sf::Uint8)(fill_color[1] * 255.f);
					pixels[p + 2] = (sf::Uint8)(fill_color[2] * 255.f);
					pixels[p + 3] = (sf::Uint8)(fill_color[3] * 255.f);
				}
			}
		}

		texture.update(pixels);
		pl->window->draw(sf::Sprite(texture));
		delete[] pixels;

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

					if (spos.x >= 0 && spos.x < pl->size && spos.y >= 0 && spos.y < pl->size) {
						sf::Vertex vertex;
						vertex.position = Vector2f(spos.x, spos.y);
						vertex.color = Color(outline_color[0] * 255.f, outline_color[1] * 255.f, outline_color[2] * 255.f, outline_color[3] * 255.f);
						vt_buffer[0].push_back(vertex);
					}
				}

				if (solutions.second < FLT_MAX) {
					Vector2f spos = pl->plt_to_wnd(Vector2f(px, solutions.second));

					if (spos.x >= 0 && spos.x < pl->size && spos.y >= 0 && spos.y < pl->size) {
						sf::Vertex vertex;
						vertex.position = Vector2f(spos.x, spos.y);
						vertex.color = Color(outline_color[0] * 255.f, outline_color[1] * 255.f, outline_color[2] * 255.f, outline_color[3] * 255.f);
						vt_buffer[1].push_back(vertex);
					}
				}
			}

			if(vt_buffer[0].size())
				pl->window->draw(vt_buffer[0].data(), vt_buffer[0].size(), PrimitiveType::LinesStrip);
			if(vt_buffer[1].size())
				pl->window->draw(vt_buffer[1].data(), vt_buffer[1].size(), PrimitiveType::LinesStrip);
			if (vt_buffer[0].size() && vt_buffer[1].size()) {
				Vertex vt[2] = { vt_buffer[0][vt_buffer[0].size() - 1], vt_buffer[1][vt_buffer[1].size() - 1] };
				pl->window->draw(vt, 2, PrimitiveType::LinesStrip);
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
	area1.add(new line(1.f, 0, true));
	area1.add(new parabola_horizontal(-1.f, 4, -1, false));

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
		for (area ar : areas) {
			ar.render(pl);
		}
		pl->draw_grid();

		pl->offset = prev_offset;
		window.display();
	}

	return 0;
}