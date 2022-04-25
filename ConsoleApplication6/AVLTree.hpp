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

	virtual bool Insert(const T& value) override {
		bool result = false;
		this->root = this->InsertInternal(this->root, value, &result);
		return result;
	}

	virtual bool Delete(const T& value) {
		bool result = false;
		this->root = this->DeleteInternal(this->root, value, &result);
		return result;
	}

	virtual std::string TreeName() {
		return "AVL";
	}

	AVLTree() {
		this->root = nullptr;
	}
};

