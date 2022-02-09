#pragma once
#include <chrono>
#include <iostream>
#include "SFML/Graphics.hpp"

#define M_PI_F 3.14159265358979323846f
#define RAD2DEG(x) ((float)(x) * (float)(180.f / M_PI_F))
#define DEG2RAD(x) ((float)(x) * (float)(M_PI_F / 180.f))

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
	P Dot(Vec2 other) {
		return this->x * other.x + this->y * other.y;
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

	Vec2 Rotate(P angle) {
		Vec2 rotated_point;
		rotated_point.x = this->x * cos(angle) - this->y * sin(angle);
		rotated_point.y = this->x * sin(angle) + this->y * cos(angle);
		return rotated_point;
	}
};

using Vec2f = Vec2<float, float>;

template<typename P = float, class T = Vec2<P, P>>
class Line {
public:
	T start;
	T end;
	
	Line(T start, T end) : start(start), end(end) {};
	Line() : Line(T(), T()) {};

	T ClosestPoint(T pos)
	{
		static auto Clamp = [](P value, P min, P max)
		{
			if (value < min)
			{
				value = min;
			}
			else if (value > max)
			{
				value = max;
			}
			return value;
		};

		T a = end - start;
		P magnitude = a.Length();
		if (magnitude == 0) 
			return start;
		T vector = a / magnitude;
		return start + vector * Clamp(T(pos - start).Dot(vector), 0, magnitude);
	}
};