#pragma once
#include "BaseEntity.h"
#include "Utils.h"

using sf::Vector2f;

class BaseFood : public BaseEntity
{
	Vec2f originPosition;
	float amount;
public:
	BaseFood();

	virtual void Draw(sf::RenderWindow& wnd);
	virtual void OnSpawned();
	virtual void Update(double deltaTime);

	static void SpawnAt(Vec2f position, float amount);
};


