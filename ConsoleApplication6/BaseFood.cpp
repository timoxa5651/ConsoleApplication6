#include "BaseFood.h"
#include "BaseGame.h"

BaseFood::BaseFood() {

}

void BaseFood::OnSpawned() {

}
void BaseFood::Update(double deltaTime) {

}

void BaseFood::SpawnAt(Vec2f position, float amount) {
    BaseFood* food = new BaseFood();
    food->amount = amount;
    food->originPosition = food->position = position;
    BaseGame::g_Instance->RegisterEntity(food);
}

void BaseFood::Draw(sf::RenderWindow& wnd) {
    /*sf::Texture tex;
    sf::Sprite sprite;

    tex.loadFromFile("food.png");
    sprite.setTexture(tex);
    sprite.setPosition(this->position);
    wnd.draw(sprite);*/
    sf::RectangleShape shp;
    shp.setPosition(this->position);
    shp.setFillColor(sf::Color(255, 0, 0));
    shp.setSize(sf::Vector2f(4, 4));
    wnd.draw(shp);
}