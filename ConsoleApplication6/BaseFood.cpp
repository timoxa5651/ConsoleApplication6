#include "BaseFood.h"
#include "BaseGame.h"

BaseFood::BaseFood() {
    this->amount = 0.f;

    BaseEntity::BaseEntity();
}

void BaseFood::OnSpawned() {
    BaseGame::g_Instance->ProcessCollision(this, this->position);

}
void BaseFood::Update(double deltaTime) {

}

void BaseFood::SpawnAt(Vec2f position, float amount) {
    BaseFood* food = new BaseFood();
    food->SetAmount(amount);
    food->originPosition = food->position = position;
    BaseGame::g_Instance->RegisterEntity(food);
}

void BaseFood::SetAmount(float newAmount) {
    this->amount = newAmount;
    int s = std::min(this->amount * 2.5f, 12.f);
    int d = s / 3;
    this->drawTexture = sf::Texture();
    this->drawTexture.create(2 * s + 1, 2 * s + 1);

    uint8_t* pixels = new uint8_t[(2 * s + 1) * (2 * s + 1) * 4];
    int ik = 0;
    for (int x = -s; x <= s; ++x) {
        int my = (abs(x) <= d) ? s : d;
        for (int y = -s; y <= s; ++y) {
            sf::Color clr = sf::Color(0, 255, 0, 0);
            if (abs(y) <= my) {
                clr.a = 255;
            }
            pixels[ik++] = clr.r;
            pixels[ik++] = clr.g;
            pixels[ik++] = clr.b;
            pixels[ik++] = clr.a;
        }
    }
    this->drawTexture.update(pixels);
    delete[] pixels;
}
float BaseFood::GetAmount() {
    return this->amount;
}

void BaseFood::Draw(sf::RenderWindow& wnd) {
    sf::Sprite sprite;
    sprite.setTexture(this->drawTexture);
    sprite.setPosition(this->position);
    wnd.draw(sprite);
}

bool BaseFood::Intersects(Line<> line, float radius) {
    return Vec2f(line.ClosestPoint(this->position) - this->position).Length() <= radius + this->GetRadius();
}

float BaseFood::GetRadius() {
    return std::min(this->amount * 3.f, 12.f);
}