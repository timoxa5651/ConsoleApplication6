#pragma once
#include "BaseTree.hpp"

template<typename T>
class AVLNode : public BaseNode<T, AVLNode<T>> {
	template<typename U> friend class AVLTree;

	void UpdateHeight() {
		int lh = this->left ? this->left->height : 0;
		int rh = this->right ? this->right->height : 0;
		this->height = 1 + std::max(lh, rh);
	}
	int GetBalance() {
		int lh = this->left ? this->left->height : 0;
		int rh = this->right ? this->right->height : 0;
		return lh - rh;
	}

	static float GetNodeSize() {
		return 120.f;
	}


	void Draw(sf::RenderWindow* window, sf::Vector2f position, sf::Vector2f parentPos) {
		float freq = 0.0001f;
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

	AVLNode() {
		this->left = this->right = nullptr;
		this->height = 1;
	};
	AVLNode(const T& val) : AVLNode() {
		this->data = val;
	}
public:	
	int height;
};

template<typename T>
class AVLTree : public BaseTree<T>
{
	typedef AVLNode<T> Node, * PNode;

	PNode RotateRight(PNode node) {
		PNode newhead = node->left;
		node->left = newhead->right;
		newhead->right = node;
		node->UpdateHeight();
		newhead->UpdateHeight();
		return newhead;
	}

	PNode RotateLeft(PNode node) {
		PNode newhead = node->right;
		node->right = newhead->left;
		newhead->left = node;
		node->UpdateHeight();
		newhead->UpdateHeight();
		return newhead;
	}

	PNode InsertInternal(PNode node, const T& value, bool* result) {
		if (node == nullptr) {
			node = new Node(value);
			return node;
		}
		if (value < node->data)
			node->left = this->InsertInternal(node->left, value, result);
		else if (value > node->data)
			node->right = this->InsertInternal(node->right, value, result);
		else if (result)
			*result = false;
		
		node->UpdateHeight();
		int bal = node->GetBalance();
		if (bal > 1) {
			if (value < node->left->data) {
				return this->RotateRight(node);
			}
			else {
				node->left = this->RotateLeft(node->left);
				return this->RotateRight(node);
			}
		}
		else if (bal < -1) {
			if (value > node->right->data) {
				return this->RotateLeft(node);
			}
			else {
				node->right = this->RotateRight(node->right);
				return this->RotateLeft(node);
			}
		}
		return node;
	}

	PNode DeleteInternal(PNode head, const T& x, bool* result) {
		if (head == NULL) 
			return NULL;
		if (x < head->data) {
			head->left = this->DeleteInternal(head->left, x, result);
		}
		else if (x > head->data) {
			head->right = this->DeleteInternal(head->right, x, result);
		}
		else {
			if (result)
				*result = true;
			PNode r = head->right;
			if (!head->right) {
				PNode l = head->left;
				head->left = head->right = 0;
				delete head;
				head = l;
			}
			else if (head->left == NULL) {
				head->left = head->right = 0;
				delete head;
				head = r;
			}
			else {
				while (r->left != NULL) r = r->left;
				head->data = r->data;
				head->right = this->DeleteInternal(head->right, r->data, result);
			}
		}
		if (!head) 
			return head;
		head->UpdateHeight();
		int bal = head->GetBalance();
		int hl = head->left ? head->left->height : 0;
		int hr = head->right ? head->right->height : 0;
		if (bal > 1) {
			if (hl >= hr) {
				return this->RotateRight(head);
			}
			else {
				head->left = this->RotateLeft(head->left);
				return this->RotateRight(head);
			}
		}
		else if (bal < -1) {
			if (hr >= hl) {
				return this->RotateLeft(head);
			}
			else {
				head->right = this->RotateRight(head->right);
				return this->RotateLeft(head);
			}
		}
		return head;
	}

	template<typename F>
	void InOrderInternal2(PNode node, int height, F call) {
		if (node == nullptr) return;
		this->InOrderInternal2(node->left, height + 1, call);
		call(node, height);
		this->InOrderInternal2(node->right, height + 1, call);
	}
	template<typename F>
	bool InOrderInternal3(PNode node, int height, F call) {
		if (node == nullptr) return true;
		if (this->InOrderInternal3(node->left, height + 1, call))
			return true;
		if (call(node, height))
			return true;
		if (this->InOrderInternal3(node->right, height + 1, call))
			return true;
	}

	virtual void InOrderInternal(void* call, bool ret) override {
		if(!ret)
			return this->InOrderInternal2(this->root, 0, reinterpret_cast<void(*)(PNode, int)>(call));
		this->InOrderInternal3(this->root, 0, reinterpret_cast<bool(*)(PNode, int)>(call));
	}

public:
	PNode root;
	using Type = T;
	using NodeType = Node;

	virtual bool Insert(const T& value) final {
		bool result = false;
		this->root = this->InsertInternal(this->root, value, &result);
		return result;
	}

	virtual bool Delete(const T& value) {
		bool result = false;
		this->root = this->DeleteInternal(this->root, value, &result);
		return result;
	}

	std::string TreeName() {
		return "AVL";
	}

	AVLTree() {
		this->root = nullptr;
	}
};

