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

	std::deque<PosEntry> positionHistory;
public:
	double score;
	bool isLocal;

	SnakeEntity();
	void MakeLocal();
	void AddScore(float scoreDelta);

	virtual void Draw(sf::RenderWindow& wnd);
	virtual void OnSpawned();
	virtual void OnKeyPressed(sf::Keyboard::Key key);
	virtual void Update(double deltaTime);
	virtual float GetRadius();
	virtual void OnCollision(BaseEntity* entity);
	virtual bool Intersects(Line<> line, float radius);
};

