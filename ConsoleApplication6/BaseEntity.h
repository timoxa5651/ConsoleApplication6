#pragma once
#include <cstdint>
#include <SFML/System/Vector2.hpp>

class BaseEntity
{
public:
	uint32_t uid;
	sf::Vector2f position;

	BaseEntity();
	virtual void Update(float deltaTime);
	virtual void OnSpawned();
};

