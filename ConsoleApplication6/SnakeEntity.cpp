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
	this->uid = UINT_MAX - 1;
	this->isLocal = true;
	this->desiredMoveSpeed = this->moveSpeed = 150.f;
	this->thickness = 7.5f;
	this->desiredVelocity = Vec2f(0, 0);
	this->AddScore(30.f);
}

void SnakeEntity::MakeBot() {
	this->uid = UINT_MAX - 2;
	this->isLocal = false;
	this->desiredMoveSpeed = this->moveSpeed = 100.f;
	this->thickness = 7.5f;
	this->desiredVelocity = Vec2f(1, 0);
	this->lastEnemyTime = 0.f;
	this->AddScore(50.f);
}

void SnakeEntity::OnKeyPressed(sf::Keyboard::Key key) {
	
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

void SnakeEntity::Bot_Update(float deltaTime) {
	SnakeEntity* localSnake = BaseGame::g_Instance->localSnake;
	bool velocityUpdated = false;
	if (localSnake) {
		Line<> raycast = Line<>(this->position, this->position + this->velocity.Normalized() * 30.f);
		Vec2f hitPoint;
		Vec2f rot = this->velocity.Normalized();
		int num = 0;
		constexpr int step = 1;
		while (num < 360 && localSnake->Intersects(raycast, this->GetRadius(), &hitPoint)) {
			num += step;
			rot = this->velocity.Normalized().Rotate(DEG2RAD(num));
			raycast = Line<>(this->position, this->position + rot * 30.f);
		}

		if (num > 0 && num < 360) {
			velocityUpdated = true;
			this->desiredVelocity = rot;
		}
		else if (num > 0) {
			velocityUpdated = true;
		}

		if (num > 0) {
			this->lastEnemyTime = Utils::Time();
		}
	}
	

	bool canSprint = this->score > 30;
	bool sprintUpdated = false;
	bool shouldSprint = canSprint && this->isSprinting;
	if (!velocityUpdated && localSnake && Utils::Time() - this->lastEnemyTime >= 0.5f) {
		float dist = Vec2f(this->position - localSnake->position).Length();
		float requiredDist = (this->score > 250.f) ? 300.f : 500.f;
		if (dist > requiredDist) {
			this->desiredVelocity = localSnake->position - this->position;
			if (canSprint) {
				sprintUpdated = true;
				shouldSprint = true;
			}
		}
		else if (this->score > 250.f) {
			// attack
			//float dist = Vec2f(this->position - localSnake->position).Length();
			//Line<> forward = Line<>(this->position, localSnake->position);
			this->desiredVelocity = localSnake->position + localSnake->velocity.Normalized() * 10.f - this->position;
			sprintUpdated = true;
			shouldSprint = dist > 40.f;
		}
		else {
			Vec2f closestFood = this->closestFoodPos;
			if (Utils::Time() - this->closestFoodTime > 1.f) {
				bool flag = false;
				for (auto it = BaseGame::g_Instance->clientEntities.begin(); it != BaseGame::g_Instance->clientEntities.end(); ++it) {
					BaseEntity* entity = (*it).second;
					if (entity->isKilled)
						continue;
					BaseFood* food = dynamic_cast<BaseFood*>(entity);
					if (food && (!flag || Vec2f(food->position - this->position).Length() < Vec2f(closestFood - this->position).Length())) {
						closestFood = food->position;
						flag = true;
					}
				}
				this->closestFoodPos = closestFood;
				this->closestFoodTime = Utils::Time();
			}
			shouldSprint = canSprint;
			sprintUpdated = true;

			this->desiredVelocity = closestFood - this->position;
		}
	}

	if(!sprintUpdated && canSprint)
		shouldSprint = Utils::Time() - this->lastEnemyTime < 2.f;
	if (shouldSprint) {
		this->desiredMoveSpeed = 650.f;
	}
	else {
		this->desiredMoveSpeed = 155.f;
	}
}

void SnakeEntity::Update(double deltaTime) {
	sf::RenderWindow& wnd = BaseGame::g_Instance->renderWindow;
	//std::cout << deltaTime << std::endl;
	if (this->isLocal) {
		if (sf::Mouse::isButtonPressed(sf::Mouse::Left) && this->score > 30) {
			this->desiredMoveSpeed = 300.f;
			this->isSprinting = true;
		}
		else {
			this->desiredMoveSpeed = 150.f;
			this->isSprinting = false;
		}
	}
	else {
		this->Bot_Update(deltaTime);
	}
	this->moveSpeed += (this->desiredMoveSpeed - this->moveSpeed) * 0.25f;

	Vec2<int> center = Vec2f(wnd.getSize()) * 0.5f;
	Vec2f wndCenter = wnd.mapPixelToCoords(center);

	if (this->isLocal)
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
	if(!this->isLocal)
		sh.setFillColor(sf::Color(255, 0, 0, 200));
	for (auto it = this->positionHistory.begin(); it != this->positionHistory.end(); ++it) {
		if (it == this->positionHistory.end() - 1)
			sh.setFillColor(sf::Color(0, 0, 255));
		sh.setPosition((*it).position - Vec2f(this->thickness / 2, this->thickness / 2));

		wnd.draw(sh);
	}
}

bool SnakeEntity::Intersects(Line<> line, float radius, Vec2f* hitPoint) {
	for (auto it = this->positionHistory.begin(); it != this->positionHistory.end(); ++it) {
		Vec2f pos = (*it).position;
		Vec2f clos = line.ClosestPoint(pos);
		if (Vec2f(clos - pos).Length() <= this->thickness + radius) {
			if(hitPoint)
				*hitPoint = clos;
			return true;
		}
	}
	return false;
}

void SnakeEntity::OnCollision(BaseEntity* entity) {
	BaseFood* food = dynamic_cast<BaseFood*>(entity);
	if (food) {
		this->AddScore(food->GetAmount());
		food->OnKilled();
		this->closestFoodTime = 0.f;
		return;
	}
	SnakeEntity* snake = dynamic_cast<SnakeEntity*>(entity);
	if (snake) {
		std::cout << this->isLocal << " collided with " << snake->isLocal << std::endl;
		this->OnKilled();
		return;
	}
}

void SnakeEntity::OnKilled() {
	double delta = this->score / this->positionHistory.size();
	double pending = 0;
	for (auto it = this->positionHistory.begin(); it != this->positionHistory.end(); ++it) {
		pending += delta;
		if (pending > 10.f) {
			BaseFood::SpawnAt((*it).position, 10.f);
			pending -= 10.f;
		}
	}
	BaseFood::SpawnAt((*this->positionHistory.rbegin()).position, pending);
	BaseEntity::OnKilled();
}

float SnakeEntity::GetRadius() {
	return this->thickness;
}
