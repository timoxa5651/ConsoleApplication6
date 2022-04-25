#pragma once
#include "BaseTree.hpp"

template<typename T>
class SplayNode : public BaseNode<T, SplayNode<T>> {
	template<typename U> friend class SplayTree;

	static float GetNodeSize() {
		return 120.f;
	}

	void UpdateHeight() {
		int lh = this->left ? this->left->height : 0;
		int rh = this->right ? this->right->height : 0;
		this->height = 1 + std::max(lh, rh);
	}
	SplayNode() {
		this->left = this->right = this->parent = nullptr;
		this->height = 1;
	};
	SplayNode(const T& val) : SplayNode() {
		this->data = val;
	}
public:
	int height;
	SplayNode<T>* parent;
};

template<typename T>
class SplayTree : public BaseTree<T>
{
	typedef SplayNode<T> Node, * PNode;

	void RotateLeft(PNode x) {
		PNode y = x->right;
		x->right = y->left;
		if (y->left != nullptr) {
			y->left->parent = x;
		}
		y->parent = x->parent;
		if (x->parent == nullptr) {
			this->root = y;
		}
		else if (x == x->parent->left) {
			x->parent->left = y;
		}
		else {
			x->parent->right = y;
		}
		y->left = x;
		x->parent = y;
		x->UpdateHeight();
		y->UpdateHeight();
	}
	void RotateRight(PNode x) {
		PNode y = x->left;
		x->left = y->right;
		if (y->right != nullptr) {
			y->right->parent = x;
		}
		y->parent = x->parent;
		if (x->parent == nullptr) {
			this->root = y;
		}
		else if (x == x->parent->right) {
			x->parent->right = y;
		}
		else {
			x->parent->left = y;
		}
		y->right = x;
		x->parent = y;
		x->UpdateHeight();
		y->UpdateHeight();
	}

	void Splay(PNode x) {
		while (x->parent) {
			if (!x->parent->parent) {
				if (x == x->parent->left) {
					// zig
					this->RotateRight(x->parent);
				}
				else {
					// zag
					this->RotateLeft(x->parent);
				}
			}
			else if (x == x->parent->left && x->parent == x->parent->parent->left) {
				// zigzig
				this->RotateRight(x->parent->parent);
				this->RotateRight(x->parent);
			}
			else if (x == x->parent->right && x->parent == x->parent->parent->right) {
				// zagzag
				this->RotateLeft(x->parent->parent);
				this->RotateLeft(x->parent);
			}
			else if (x == x->parent->right && x->parent == x->parent->parent->left) {
				// zigzag
				this->RotateLeft(x->parent);
				this->RotateRight(x->parent);
			}
			else {
				// zagzig
				this->RotateRight(x->parent);
				this->RotateLeft(x->parent);
			}
		}
	}

	void InsertInternal(const T& value, bool* result) {
		PNode node = new Node(value);
		PNode x = this->root;
		PNode y = nullptr;
		while (x) {
			y = x;
			if (node->data < x->data) {
				x = x->left;
			}
			else {
				x = x->right;
			}
		}

		if (y == nullptr) {
			this->root = node;
		}
		else if (node->data == y->data) {
			delete node;
			return;
		}
		else if (node->data < y->data) {
			y->left = node;
		}
		else {
			y->right = node;
		}
		node->parent = y;

		node->UpdateHeight();
		if(y)
			y->UpdateHeight();
		this->Splay(node);
		*result = true;
	}

	void DeleteInternal(PNode head, const T& x, bool* result) {
		
	}

	template<typename F>
	void InOrderInternal2(PNode node, int height, F call) {
		if (node == nullptr) return;
		this->InOrderInternal2(node->left, height + 1, call);
		call(node, height);
		this->InOrderInternal2(node->right, height + 1, call);
	}

	virtual void InOrderInternal(void* call) override {
		return this->InOrderInternal2(this->root, 0, reinterpret_cast<void(*)(PNode, int)>(call));
	}
	virtual void* GetRootInternal() override {
		return reinterpret_cast<void*>(this->root);
	};

public:
	PNode root;
	using Type = T;
	using NodeType = Node;

	virtual bool Insert(const T& value) final {
		bool result = false;
		this->InsertInternal(value, &result);
		return result;
	}

	virtual bool Delete(const T& value) {
		bool result = false;
		this->DeleteInternal(this->root, value, &result);
		return result;
	}

	virtual std::string TreeName() {
		return "Splay";
	}

	SplayTree() {
		this->root = nullptr;
	}
};

