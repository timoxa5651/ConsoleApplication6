#include "BaseEntity.h"
#include "SpawnerEntity.h"
#include <stdlib.h>

BaseEntity::BaseEntity() {
	this->uid = 2 + rand() * rand();
	this->isKilled = false;
	this->velocity = Vec2f();
	this->position = Vec2f();
	this->spawnZone = nullptr;
}
void BaseEntity::Update(double deltaTime) {
	this->position += this->velocity * (float)deltaTime;
}
void BaseEntity::Draw(sf::RenderWindow& wnd) {

}
void BaseEntity::OnSpawned() {

}

void BaseEntity::OnKeyPressed(sf::Keyboard::Key key) {

}

bool BaseEntity::Intersects(Line<> line, float radius) {
	return false;
}

void BaseEntity::OnCollision(BaseEntity* entity) {

}

void BaseEntity::OnCollisionWith(BaseEntity* entity) {

}

float BaseEntity::GetRadius() {
	return 0.f;
}

void BaseEntity::OnKilled() {
	if (!this->isKilled && this->spawnZone) {
		this->spawnZone->EntityExit(this);
		this->spawnZone = nullptr;
	}
	this->isKilled = true;
}