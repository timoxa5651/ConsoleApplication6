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
	this->uid = 1;
	this->isLocal = true;
	this->desiredMoveSpeed = this->moveSpeed = 150.f;
	this->thickness = 7.5f;
	this->desiredVelocity = Vec2f(0, 0);
	this->AddScore(200.f);
}

void SnakeEntity::OnKeyPressed(sf::Keyboard::Key key) {
	if (this->isLocal) {

	}
}

void SnakeEntity::AddScore(float scoreDelta) {

	this->score += scoreDelta;
	if (scoreDelta > 0.f) {
		constexpr float dstep = 0.01f;
		Vec2f direction = Vec2f(-1, 0);
		if(this->positionHistory.size() >= 2)
			direction = (*(this->positionHistory.begin())).position - (*(this->positionHistory.begin() + 1)).position;

		double screenTime = this->score / 100.f;
		while (this->positionHistory.empty() || Utils::Time() - this->positionHistory.front().time < screenTime) {
			PosEntry prevEntry = this->positionHistory.empty() ? PosEntry{ this->position, Utils::Time() } : this->positionHistory.front();
			Vec2f prevPosition = prevEntry.position;
			PosEntry entry;
			entry.position = prevPosition + direction.Normalized() * dstep * this->moveSpeed;
			entry.time = prevEntry.time - dstep;
			this->positionHistory.push_front(entry);
		}
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

	if (this->desiredVelocity.Length())
		this->velocity = this->velocity.LerpTo(this->desiredVelocity.Normalized() * this->moveSpeed, 1.f);

	double screenTime = this->score / 100;
	if (!this->positionHistory.empty()) {
		for (auto it = this->positionHistory.end() - 1; it != this->positionHistory.begin(); --it) {
			PosEntry& entry = *it;
			if (Utils::Time() - entry.time > screenTime) {
				if (this->isSprinting && Utils::Time() >= this->nextFoodSpawnTime) {
					Vec2f dir = (it != this->positionHistory.begin()) ? ((*(it - 1)).position - entry.position) : Vec2f();

					BaseFood::SpawnAt(entry.position + dir.Normalized() * (5.f + 10 * (rand() % 100) / 100.f), 2.f);
					this->nextFoodSpawnTime = Utils::Time() + 0.1f + (rand() % 100) / 200.f;
					this->AddScore(-2.5f);
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
	sf::CircleShape sh;
	sh.setRadius(this->thickness);
	sh.setFillColor(sf::Color(255, 255, 255));
	for (auto it = this->positionHistory.begin(); it != this->positionHistory.end(); ++it) {
		if (it == this->positionHistory.end() - 1)
			sh.setFillColor(sf::Color(0, 0, 255));
		sh.setPosition((*it).position - Vec2f(this->thickness / 2, this->thickness / 2));

		wnd.draw(sh);
	}
}

bool SnakeEntity::Intersects(Line<> line, float radius) {
	for (auto it = this->positionHistory.begin(); it != this->positionHistory.end(); ++it) {
		Vec2f pos = (*it).position;
		if (Vec2f(line.ClosestPoint(pos) - pos).Length() <= this->thickness + radius) {
			return true;
		}
	}
	return false;
}

void SnakeEntity::OnCollision(BaseEntity* entity) {
	BaseFood* food = dynamic_cast<BaseFood*>(entity);
	if (food)
	{
		this->AddScore(food->GetAmount());
		food->OnKilled();
		return;
	}
}

float SnakeEntity::GetRadius() {
	return this->thickness;
}
