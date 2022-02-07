#include "BaseEntity.h"
#include <stdlib.h>

BaseEntity::BaseEntity() {
	this->uid = 1 + rand();
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
