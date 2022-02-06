#include "SnakeEntity.h"

SnakeEntity::SnakeEntity() {
	BaseEntity::BaseEntity();
	this->isLocal = false;
}

void SnakeEntity::OnSpawned() {

}

void SnakeEntity::MakeLocal() {
	this->isLocal = true;
}