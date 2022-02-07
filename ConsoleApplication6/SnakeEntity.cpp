#include "SnakeEntity.h"
#include "Utils.h"
#include "BaseGame.h"
#include "BaseFood.h"

SnakeEntity::SnakeEntity() {
	BaseEntity::BaseEntity();
	this->isLocal = false;
}

void SnakeEntity::OnSpawned() {

}

void SnakeEntity::MakeLocal() {
	this->isLocal = true;
	this->desiredMoveSpeed = this->moveSpeed = 150.f;
	this->score = 100.f;
	this->thickness = 7.5f;
	this->desiredVelocity = Vec2f(0, 0);
}

void SnakeEntity::OnKeyPressed(sf::Keyboard::Key key) {
	if (this->isLocal) {

	}
}

void SnakeEntity::Update(double deltaTime) {
	sf::RenderWindow& wnd = BaseGame::g_Instance->renderWindow;
	//std::cout << deltaTime << std::endl;
	if (sf::Mouse::isButtonPressed(sf::Mouse::Left) && this->score > 50) {
		this->desiredMoveSpeed = 300.f;
		this->isSprinting = true;
	}
	else {
		this->desiredMoveSpeed = 150.f;
		this->isSprinting = false;
	}
	this->moveSpeed += (this->desiredMoveSpeed - this->moveSpeed) * 0.25f;

	Vec2<int> center = Vec2f(wnd.getSize()) * 0.5f;
	Vec2f wndCenter = wnd.mapPixelToCoords(center);

	this->desiredVelocity = Vec2f(wnd.mapPixelToCoords(sf::Mouse::getPosition(wnd))) - this->position;

	if(this->desiredVelocity.Length())
		this->velocity = this->velocity.LerpTo(this->desiredVelocity.Normalized() * this->moveSpeed, 1.f);

	double screenTime = this->score / 100;
	if (!this->positionHistory.empty()) {
		for (auto it = this->positionHistory.end() - 1; it != this->positionHistory.begin(); --it) {
			PosEntry& entry = *it;
			if (Utils::Time() - entry.time > screenTime) {
				if (this->isSprinting && Utils::Time() >= this->nextFoodSpawnTime) {
					Vec2f dir = (it == this->positionHistory.begin()) ? ((*(it - 1)).position - entry.position) : Vec2f();

					BaseFood::SpawnAt(entry.position + dir.Normalized() * 10.f, 1.75f);
					this->nextFoodSpawnTime = Utils::Time() + 0.1f + (rand() % 100) / 200.f;
					this->score -= 2.5f;
				}
				this->positionHistory.erase(this->positionHistory.begin(), it);
				break;
			}
		}
	}

	this->positionHistory.push_back({ this->position, Utils::Time() });

	BaseEntity::Update(deltaTime);
}

void SnakeEntity::Draw(sf::RenderWindow& wnd) {
	for (auto it = this->positionHistory.begin(); it != this->positionHistory.end(); ++it) {
		sf::CircleShape sh;
		sh.setFillColor(sf::Color(255, 255, 255));
		if(it == this->positionHistory.end() - 1)
			sh.setFillColor(sf::Color(0, 255, 0));
		sh.setPosition((*it).position);
		sh.setRadius(this->thickness);

		wnd.draw(sh);
	}
}