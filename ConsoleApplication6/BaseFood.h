#pragma once
#include <algorithm>
#include "BaseEntity.h"
#include "Utils.h"

using sf::Vector2f;

class BaseFood : public BaseEntity
{
	Vec2f originPosition;
	float amount;
	sf::Texture drawTexture;
public:

	BaseFood();
	void SetAmount(float newAmount);
	float GetAmount();
	static void SpawnAt(Vec2f position, float amount);

	virtual void Draw(sf::RenderWindow& wnd);
	virtual void OnSpawned();
	virtual void Update(double deltaTime);
	virtual bool Intersects(Line<> line, float radius, Vec2f* point);
	virtual float GetRadius();	
	virtual void OnKilled();
};


