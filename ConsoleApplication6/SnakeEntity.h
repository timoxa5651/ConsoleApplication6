#pragma once
#include "BaseEntity.h"
#include <iostream>
#include <deque>

using sf::Vector2f;

struct PosEntry {
	Vec2f position;
	double time;
};
class SnakeEntity : public BaseEntity
{
	Vec2f desiredVelocity;
	float moveSpeed;
	float desiredMoveSpeed;
	float thickness;
	bool isSprinting;
	float nextFoodSpawnTime;

	float lastEnemyTime;
	Vec2f closestFoodPos;
	float closestFoodTime;

	std::deque<PosEntry> positionHistory;
public:
	double score;
	bool isLocal;

	SnakeEntity();
	void MakeLocal();
	void MakeBot();
	void AddScore(float scoreDelta);

	void Bot_Update(float deltaTime);

	virtual void Draw(sf::RenderWindow& wnd);
	virtual void OnSpawned();
	virtual void OnKeyPressed(sf::Keyboard::Key key);
	virtual void Update(double deltaTime);
	virtual float GetRadius();
	virtual void OnCollision(BaseEntity* entity);
	virtual bool Intersects(Line<> line, float radius, Vec2f* hitPoint);
	virtual void OnKilled();
};

