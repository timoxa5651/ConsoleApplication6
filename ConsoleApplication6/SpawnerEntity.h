#pragma once
#include <algorithm>
#include "BaseEntity.h"
#include "Utils.h"

class SpawnZone {
public:
	class SpawnerEntity* ownerEntity;
	Vec2f position;
	std::map<decltype(BaseEntity::uid), BaseEntity*> entities;
	bool hadEntities;

	bool Contains(Vec2f point);
	void EntityExit(BaseEntity* entity);
	void EntityJoin(BaseEntity* entity);
};
using OnSpawnCallback = BaseEntity*(*)(Vec2f position);

class SpawnerEntity : public BaseEntity
{
	float updateInterval;
	float updateRandom;
	OnSpawnCallback onSpawn;
	float population;
	float nextUpdateTime;
	float spawnPerTick;
	std::map<int, std::map<int, SpawnZone*>> zones; // map x, y

	SpawnZone* CreateZone(Vec2<int> position);
public:
	float zoneSize;

	SpawnZone* QueryZone(Vec2f position);
	SpawnerEntity(OnSpawnCallback onSpawn, float population, float spawnPerTick, float zoneSize);
	void SpawnAll();

	virtual void OnSpawned();
	virtual void Update(double deltaTime);
	virtual bool Intersects(Line<> line, float radius, Vec2f* point);
	virtual void OnCollisionWith(BaseEntity* entity);
};


