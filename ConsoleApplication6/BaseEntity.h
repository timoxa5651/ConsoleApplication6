#pragma once
#include <cstdint>
#include "SFML/Graphics.hpp"
#include "Utils.h"

class BaseEntity
{
public:
	uint32_t uid;
	Vec2f position;
	Vec2f velocity;
	bool isKilled;
	struct SpawnZone* spawnZone;

	BaseEntity();
	virtual void Update(double deltaTime);
	virtual void Draw(sf::RenderWindow& wnd);
	virtual void OnSpawned();
	virtual void OnKeyPressed(sf::Keyboard::Key key);

	virtual void OnCollision(BaseEntity* entity);
	virtual void OnCollisionWith(BaseEntity* entity);
	virtual bool Intersects(Line<> line, float radius);
	virtual float GetRadius();
	virtual void OnKilled();
};

