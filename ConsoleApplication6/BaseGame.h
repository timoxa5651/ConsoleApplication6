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

	std::map<uint32_t, BaseEntity*> clientEntities;
	SnakeEntity* localSnake;
	
	void OnCreated();
public:
	static BaseGame* g_Instance;

	sf::RenderWindow renderWindow;

	void Start();
	void Update(double deltaTime);
	void UpdateView();
	bool IsRunning();
	void RegisterEntity(BaseEntity* entity);
};

