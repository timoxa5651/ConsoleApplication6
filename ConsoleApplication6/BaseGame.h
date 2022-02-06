#pragma once
#include "SFML/Graphics.hpp"

class BaseGame
{
	__int64 startTime;
public:
	static BaseGame* g_Instance;
	void Start();
	void Update(float deltaTime);
};

