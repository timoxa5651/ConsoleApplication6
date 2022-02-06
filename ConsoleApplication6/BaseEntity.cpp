#include "BaseEntity.h"
#include <stdlib.h>

BaseEntity::BaseEntity() {
	this->uid = 1 + rand();
}
void BaseEntity::Update(float deltaTime) {

}
void BaseEntity::OnSpawned() {

}
