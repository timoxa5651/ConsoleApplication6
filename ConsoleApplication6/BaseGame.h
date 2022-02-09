#pragma once
#include <map>
#include <iostream>

#include "SFML/Graphics.hpp"
#include "BaseEntity.h"
#include "Utils.h"
#include "SnakeEntity.h"

class BaseGame
{
	bool wantsClose;

	std::deque<double> fpsHistory;

	void OnCreated();
public:
	static BaseGame* g_Instance;

	sf::RenderWindow renderWindow;
	SnakeEntity* localSnake;
	std::map<decltype(BaseEntity::uid), BaseEntity*> clientEntities;

	void Start();
	void Update(double deltaTime);
	void UpdateView();
	bool IsRunning();
	void RegisterEntity(BaseEntity* entity);
	void ProcessCollision(BaseEntity* entity, Vec2f oldPosition);
};

