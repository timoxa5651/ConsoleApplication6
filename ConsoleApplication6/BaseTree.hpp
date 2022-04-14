#pragma once
#include <algorithm>
#include <functional>

template <typename T, class V>
class BaseNode {
	/*template<typename> friend class AVLTree;
	friend V;

	V* left;
	V* right;
	T data; :( */

public:
	V* left;
	V* right;
	T data;

	BaseNode() : left(nullptr), right(nullptr), data(T{}) {};
	BaseNode(const T& val) : BaseNode() {
		this->data = val;
	}

	virtual ~BaseNode() {
		if (this->left) {
			delete this->left;
			this->left = 0;
		}
		if (this->right) {
			delete this->right;
			this->right = 0;
		}
	}

	static float GetNodeSize() {
		return 60.f;
	}
	static float GetNodeSpacing() {
		return 50.f;
	}

	virtual void Draw(sf::RenderWindow* window, sf::Vector2f position, sf::Vector2f parentPos) {
		float freq = 0.0002f;
		sf::CircleShape shape(this->GetNodeSize() * 0.5f);
		sf::Color clr = sf::Color(std::sin(freq * g_FrameCount) * 127 + 128, std::sin(freq * g_FrameCount + 2) * 127 + 128, std::sin(freq * g_FrameCount + 4) * 127 + 128);
		shape.setFillColor(clr);
		shape.setOutlineThickness(0.f);
		shape.setPosition(position);
		window->draw(shape);
		Vector2f size = Vector2f(this->GetNodeSize(), this->GetNodeSize());
		Vector2f end = position;

		sf::Text text;
		text.setFont(g_Font);
		text.setCharacterSize(32);
		text.setFillColor(sf::Color(255 - clr.r, 255 - clr.g, 255 - clr.b, clr.a));
		text.setString(sf::String(std::to_string(this->data)));
		sf::FloatRect bdns = text.getLocalBounds();
		Vector2f pos = position + sf::Vector2f(size.x - bdns.width, size.y - bdns.height) * 0.5f;
		text.setPosition(pos);
		window->draw(text);

		if (parentPos.x != FLT_MAX) {
			sf::VertexArray lines(sf::Lines, 2);
			lines[0] = sf::Vertex(position + Vector2f(this->GetNodeSize() / 2.f, 0.f));
			lines[1] = sf::Vertex(parentPos + Vector2f(this->GetNodeSize() / 2.f, this->GetNodeSize()));
			window->draw(lines);
		}
	}
};

template<typename T>
class DummyNode : public BaseNode<T, DummyNode<T>> {

};


template <typename T>
class BaseTree
{
public:
	using Type = T;
	using NodeType = DummyNode<T>;

	BaseTree() = default;
	virtual bool Insert(const T& value) = 0;	
	virtual bool Delete(const T& value) = 0;
	virtual std::string TreeName() = 0;

	virtual void InOrderInternal(void* call, bool ret) {}

	void InOrder(void(*call)(NodeType*, int)) {
		return this->InOrderInternal(reinterpret_cast<void*>(call), false);
	}

	void InOrder(bool(*call)(NodeType*, int)) {
		return this->InOrderInternal(reinterpret_cast<void*>(call), true);
	}
};
