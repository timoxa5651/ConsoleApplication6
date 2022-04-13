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

	std::string TreeName() {
		return "AVL";
	}

	AVLTree() {
		this->root = nullptr;
	}
};

