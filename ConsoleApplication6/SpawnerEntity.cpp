#include "SpawnerEntity.h"
#include "BaseGame.h"


SpawnerEntity::SpawnerEntity(OnSpawnCallback onSpawn, float population, float spawnPerTick, float zoneSize) {
	this->onSpawn = onSpawn;
	this->population = population;
	this->nextUpdateTime = 0.f;
	this->spawnPerTick = spawnPerTick;
	this->updateRandom = 0.2f;
	this->updateInterval = 3.f;
	this->zoneSize = zoneSize;
	BaseEntity::BaseEntity();
}

void SpawnerEntity::OnSpawned() {
	
}

void SpawnerEntity::SpawnAll() {
	Vec2f size = BaseGame::g_Instance->renderWindow.getSize();
	for (float x = -size.x; x < size.x; x += this->zoneSize) {
		for (float y = -size.y; y < size.y; y += this->zoneSize) {
			this->QueryZone(Vec2f(x, y));
		}
	}

	for (auto& [x, xmap] : this->zones) {
		for (auto& [y, zone] : xmap) {
			while (zone->entities.size() < this->population) {
				Vec2f entityPos = Vec2f(x, y) * SpawnerEntity::zoneSize;
				entityPos += Vec2f(rand() / (double)RAND_MAX, rand() / (double)RAND_MAX) * SpawnerEntity::zoneSize;
				BaseEntity* spawned = this->onSpawn(entityPos);
				spawned->spawnZone = zone;
				zone->EntityJoin(spawned);
			}
		}
	}
}

void SpawnerEntity::Update(double deltaTime) {
	SnakeEntity* snake = BaseGame::g_Instance->localSnake;
	if (snake) {
		sf::Vector2u size = BaseGame::g_Instance->renderWindow.getSize();
		for (float x = snake->position.x - size.x; x < snake->position.x + size.x; x += this->zoneSize) {
			for (float y = snake->position.y - size.y; y < snake->position.y + size.y; y += this->zoneSize) {
				SpawnZone* zone = this->QueryZone(Vec2f(x, y));
				if (!zone->hadEntities) {
					while (zone->entities.size() < this->population) {
						Vec2f entityPos = zone->position * SpawnerEntity::zoneSize;
						entityPos += Vec2f(rand() / (double)RAND_MAX, rand() / (double)RAND_MAX) * SpawnerEntity::zoneSize;
						BaseEntity* spawned = this->onSpawn(entityPos);
						spawned->spawnZone = zone;
						zone->EntityJoin(spawned);
					}
				}
			}
		}
	}

	if (this->onSpawn && Utils::Time() >= this->nextUpdateTime) {
		for (auto& [x, xmap] : this->zones) {
			for (auto& [y, zone] : xmap) {
				int spawned = 0;
				while (zone->entities.size() < this->population && spawned < this->spawnPerTick) {
					Vec2f entityPos = Vec2f(x, y) * SpawnerEntity::zoneSize;
					entityPos += Vec2f(rand() / (double)RAND_MAX, rand() / (double)RAND_MAX) * SpawnerEntity::zoneSize;
					BaseEntity* spawnedEnt = this->onSpawn(entityPos);
					spawnedEnt->spawnZone = zone;
					zone->EntityJoin(spawnedEnt);
					spawned += 1;
				}
			}
		}

		this->nextUpdateTime = Utils::Time() + this->updateInterval + 2.f * this->updateRandom * ((-500 + rand() % 1000) / 1000.f);
	}
}

bool SpawnerEntity::Intersects(Line<> line, float radius, Vec2f* hitPoint) {
	return true; // force trigger collision 
}

void SpawnerEntity::OnCollisionWith(BaseEntity* entity) {
	if (entity && entity->spawnZone && !entity->spawnZone->Contains(entity->position)) {
		if(entity->spawnZone)
			entity->spawnZone->EntityExit(entity);
		entity->spawnZone = this->QueryZone(entity->position);
		entity->spawnZone->EntityJoin(entity);
	}
}

SpawnZone* SpawnerEntity::CreateZone(Vec2<int> position) {
	SpawnZone* zone = new SpawnZone();
	zone->position = position;
	zone->entities = std::map<decltype(BaseEntity::uid), BaseEntity*>();
	zone->ownerEntity = this;
	zone->hadEntities = false;

	auto& mp = this->zones[position.x];
	mp[position.y] = zone;
	return zone;
}

SpawnZone* SpawnerEntity::QueryZone(Vec2f position) {
	Vec2<int> cord = Vec2<int>(floor(position.x / this->zoneSize), floor(position.y / this->zoneSize));
	auto it = this->zones.find(cord.x);
	if (it != this->zones.end()) {
		auto& mp = (*it).second;
		auto it2 = mp.find(cord.y);
		if (it2 != mp.end()) {
			return (*it2).second;
		}
	}
	return this->CreateZone(cord);
}


bool SpawnZone::Contains(Vec2f point) {
	return sf::Rect(this->position.x, this->position.y, this->ownerEntity->zoneSize, this->ownerEntity->zoneSize).contains(point);
}

void SpawnZone::EntityJoin(BaseEntity* entity) {
	this->entities[entity->uid] = entity;
	this->hadEntities = true;
}


void SpawnZone::EntityExit(BaseEntity* entity) {
	auto it = this->entities.find(entity->uid);
	if (it != this->entities.end())
		this->entities.erase(it);
}