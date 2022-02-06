#pragma once
#include <map>
#include <iostream>

#include "SFML/Graphics.hpp"
#include "BaseEntity.h"
#include "Utils.h"
#include "SnakeEntity.h"

class BaseGame
{
	sf::RenderWindow renderWindow;
	bool wantsClose;

	std::map<uint32_t, BaseEntity*> clientEntities;
	SnakeEntity* localSnake;
	
	void OnCreated();
public:
	static BaseGame* g_Instance;

	void Start();
	void Update(float deltaTime);
	void UpdateView();
	bool IsRunning();
	void RegisterEntity(BaseEntity* entity);
};

