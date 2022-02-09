#include "BaseGame.h"
#include "SpawnerEntity.h"
#include "BaseFood.h"

BaseGame* BaseGame::g_Instance = nullptr;

void BaseGame::Start() {
	this->renderWindow.create(sf::VideoMode(800, 800), "123");
	//this->renderWindow.setVerticalSyncEnabled(false);
	this->renderWindow.setPosition(sf::Vector2i(0, 0));
	this->renderWindow.setActive(true);
	this->renderWindow.setTitle("");
	this->renderWindow.setVisible(true);
	this->wantsClose = false;

	this->OnCreated();
}

void BaseGame::UpdateView() {
	sf::View view;
	view.setCenter(this->localSnake->position);
	this->renderWindow.setView(view);
}

void BaseGame::ProcessCollision(BaseEntity* entity, Vec2f oldPosition) {
	Line<> line = Line<float, Vec2f>(oldPosition, entity->position);
	for (auto it2 = this->clientEntities.begin(); it2 != this->clientEntities.end(); ++it2) {
		BaseEntity* entity2 = (*it2).second;
		if (entity2 == entity || entity2->isKilled)
			continue;
		if (entity2->Intersects(line, entity->GetRadius(), nullptr)) {
			entity->OnCollision(entity2);
			entity2->OnCollisionWith(entity);
		}
	}
}

void BaseGame::Update(double deltaTime) {
	sf::Event evnt;
	while (this->renderWindow.pollEvent(evnt)) {
		if (evnt.type == sf::Event::EventType::Closed) {
			this->wantsClose = true;
			return;
		}
		else if (evnt.type == sf::Event::KeyPressed) {
			for (auto it = this->clientEntities.begin(); it != this->clientEntities.end(); ++it) {
				(*it).second->OnKeyPressed(evnt.key.code);
			}
		}
	}
	this->renderWindow.clear(sf::Color(0, 0, 0, 255));

	std::vector<decltype(this->clientEntities)::iterator> deletedEntities;
	deletedEntities.reserve(10);

	Vec2f windowSize = this->renderWindow.getSize();
	for (auto it = this->clientEntities.begin(); it != this->clientEntities.end(); ++it) {
		BaseEntity* entity = (*it).second;
		Vec2f prevPosition = entity->position;
		entity->Update(deltaTime);
		if (prevPosition != entity->position) {
			this->ProcessCollision(entity, prevPosition);
		}
		bool isSnake = dynamic_cast<SnakeEntity*>(entity) == entity;
		sf::Vector2i screenPoint = this->renderWindow.mapCoordsToPixel(entity->position);
		float forgiviness = entity->GetRadius();
		if (isSnake || (screenPoint.x >= -forgiviness && screenPoint.x <= windowSize.x + forgiviness && screenPoint.y >= -forgiviness && screenPoint.y <= windowSize.y + forgiviness)) {
			entity->Draw(this->renderWindow);
		}
		if (entity->isKilled) {
			deletedEntities.push_back(it);
		}
	}

	for (auto& it : deletedEntities) {
		BaseEntity* entity = (*it).second;
		if (entity->isKilled) {
			this->clientEntities.erase(it);
			delete entity;
		}
	}

	this->UpdateView();

	while (fpsHistory.size() >= 10)
		fpsHistory.pop_front();
	fpsHistory.push_back(1.0 / deltaTime);

	double sum = 0.0;
	for (auto& it : this->fpsHistory)
		sum += it;
	std::string str = "FPS: " + std::to_string((long long)(sum / this->fpsHistory.size())) + " ENT: " + std::to_string(this->clientEntities.size());
	this->renderWindow.setTitle(str);

	this->renderWindow.display();
}

bool BaseGame::IsRunning() {
	return !this->wantsClose;
}

void BaseGame::OnCreated() {
	SpawnerEntity* foodSpawner = new SpawnerEntity([](Vec2f position) -> BaseEntity* {
		BaseFood* food = new BaseFood();
		food->position = position;
		food->SetAmount(2.f);
		BaseGame::g_Instance->RegisterEntity(food);
		return food;
	}, 100.f, 3.f, 500.f);
	foodSpawner->SpawnAll();
	this->RegisterEntity(foodSpawner);


	SnakeEntity* entity = new SnakeEntity();
	entity->MakeLocal();
	entity->position = sf::Vector2f(0.f, 0.f);
	this->RegisterEntity(entity);
	this->localSnake = entity;

	SnakeEntity* enemySnake = new SnakeEntity();
	enemySnake->position = sf::Vector2f(rand() / (double)RAND_MAX, rand() / (double)RAND_MAX) * 150.f;
	enemySnake->MakeBot();
	this->RegisterEntity(enemySnake);
}

void BaseGame::RegisterEntity(BaseEntity* entity) {
	this->clientEntities[entity->uid] = entity;
	entity->OnSpawned();
}