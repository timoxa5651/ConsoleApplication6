#pragma once
#include "BaseEntity.h"

class SnakeEntity : public BaseEntity
{
	bool isLocal;
public:
	SnakeEntity();
	void MakeLocal();

	virtual void OnSpawned();
};

