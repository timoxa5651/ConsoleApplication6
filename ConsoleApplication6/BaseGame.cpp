#include "BaseGame.h"

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
		(*it).second->Update(deltaTime);
		(*it).second->Draw(this->renderWindow);
	}

	sf::View view;
	view.setCenter(this->localSnake->position);
	this->renderWindow.setView(view);
	
	this->renderWindow.display();
}

bool BaseGame::IsRunning() {
	return !this->wantsClose;
}

void BaseGame::OnCreated() {
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