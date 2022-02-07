#pragma once
#include <chrono>
#include <iostream>
#include "SFML/Graphics.hpp"

static class Utils
{
	static __int64 startTime;
	static __int64 TimeInternal();
public:
	static double Time();
};

template <typename T, typename P = float>
class Vec2 : public sf::Vector2<T> {
public:
	Vec2(T x, T y) {
		this->x = x;
		this->y = y;
	}
	Vec2() : Vec2(0, 0) {};

	template<typename S>
	Vec2(sf::Vector2<S> vec) : Vec2<T>(vec.x, vec.y) {};

	P Length() {
		return sqrtl(this->x * this->x + this->y * this->y);
	}

	Vec2 Normalized() {
		P len = this->Length();
		if (len == 0)
			return Vec2(0, 0);
		return Vec2(this->x / len, this->y / len);
	}

	Vec2 LerpTo(Vec2 b, float t) {
		return Vec2(this->x + (b.x - this->x) * t, this->y + (b.y - this->y) * t);
	}
};

using Vec2f = Vec2<float>;