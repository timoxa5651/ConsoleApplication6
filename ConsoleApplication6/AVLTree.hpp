#pragma once
#include "BaseTree.hpp"


//class AVLTree;

template<typename T>
class AVLNode : public BaseNode<T, AVLNode<T>> {
	template<typename U> friend class AVLTree;

	int height;

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
	};
	AVLNode(const T& val) : AVLNode() {
		this->data = val;
		this->height = 0;
	}
};

template<typename T = int>
class AVLTree : public BaseTree<T>
{
	typedef AVLNode<T> Node, * PNode;
	PNode root;

	PNode rightRotation(PNode node) {
		PNode newhead = node->left;
		node->left = newhead->right;
		newhead->right = node;
		node->UpdateHeight();
		newhead->UpdateHeight();
		return newhead;
	}

	PNode leftRotation(PNode node) {
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
				return rightRotation(node);
			}
			else {
				node->left = leftRotation(node->left);
				return rightRotation(node);
			}
		}
		else if (bal < -1) {
			if (value > node->right->data) {
				return leftRotation(node);
			}
			else {
				node->right = rightRotation(node->right);
				return leftRotation(node);
			}
		}
		return node;
	}

	template<typename F>
	void InOrderInternal(PNode node, F call) {
		if (node == nullptr) return;
		this->InOrderInternal(node->left, call);
		call(node);
		this->InOrderInternal(node->right, call);
	}
public:
	using Type = T;
	using NodeType = Node;

	virtual bool Insert(const T& value) final {
		bool result = false;
		this->root = this->InsertInternal(this->root, value, &result);
		return result;
	}

	template<typename F>
	void InOrder(F call) {
		return this->InOrderInternal(this->root, call);
	}

	AVLTree() {
		this->root = nullptr;
	}
};

