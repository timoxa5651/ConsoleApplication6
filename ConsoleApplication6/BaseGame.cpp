#include "BaseGame.h"
#include "SpawnerEntity.h"
#include "BaseFood.h"

BaseGame* BaseGame::g_Instance = nullptr;

void BaseGame::Start() {
	this->renderWindow.create(sf::VideoMode(800, 800), "123");
	this->renderWindow.setVerticalSyncEnabled(false);
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
		if (entity2->Intersects(line, entity->GetRadius())) {
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

	for (auto it = this->clientEntities.begin(); it != this->clientEntities.end(); ++it) {
		BaseEntity* entity = (*it).second;
		Vec2f prevPosition = entity->position;
		entity->Update(deltaTime);
		if (prevPosition != entity->position) {
			this->ProcessCollision(entity, prevPosition);
		}
		entity->Draw(this->renderWindow);
	}

	for (auto it = this->clientEntities.begin(); it != this->clientEntities.end(); ++it) {
		BaseEntity* entity = (*it).second;
		if (entity->isKilled) {
			this->clientEntities.erase(it++);
			delete entity;
			if (it == this->clientEntities.end())
				break;
		}
	}

	this->UpdateView();
	std::cout << "FPS " << (1.0 / deltaTime) << std::endl;
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
	}, 5.f, 1.f, 200.f);
	foodSpawner->SpawnAll();
	this->RegisterEntity(foodSpawner);


	SnakeEntity* entity = new SnakeEntity();
	entity->MakeLocal();
	entity->position = sf::Vector2f(0.f, 0.f);
	this->RegisterEntity(entity);
	this->localSnake = entity;
}

void BaseGame::RegisterEntity(BaseEntity* entity) {
	this->clientEntities[entity->uid] = entity;
	entity->OnSpawned();
}