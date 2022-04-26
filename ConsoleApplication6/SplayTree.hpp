#pragma once
#include "BaseTree.hpp"
#include <tuple>
using std::tuple;

template<typename T>
class SplayNode : public BaseNode<T, SplayNode<T>> {
	template<typename U> friend class SplayTree;

	static float GetNodeSize() {
		return 120.f;
	}

	SplayNode() {
		this->left = this->right = nullptr;
	};
	SplayNode(const T& val) : SplayNode() {
		this->data = val;
	}
public:
};

template<typename T>
class SplayTree : public BaseTree<T>
{
	typedef SplayNode<T> Node, * PNode;

	void Splay(PNode& node, const T& value, bool isRoot = true) {
		if (node == nullptr) {
			return;
		}

		if (value < node->data) {
			this->Splay(node->left, value, false);
			if (node->left->data == value) {
				if (isRoot) {
					PNode q = node;
					node = node->left;
					q->left = node->right;
					node->right = q;
				}
			}
			else {
				PNode p, q;
				if (value < node->left->data) {
					q = node->left;
					node->left = q->right;
					q->right = node;
					p = q->left;
					q->left = p->right;
					p->right = q;
					node = p;
				}
				else if (value > node->left->data) {
					q = node->left;
					p = q->right;
					q->right = p->left;
					p->left = q;
					node->left = p->right;
					p->right = node;
					node = p;
				}
			}
		}
		else if (value > node->data) {
			this->Splay(node->right, value, false);
			if (node->right->data == value) {
				if (isRoot) {
					PNode q = node;
					node = node->right;
					q->right = node->left;
					node->left = q;
				}
			}
			else {
				PNode q, p;
				if (value < node->right->data) {
					q = node->right;
					p = q->left;
					q->left = p->right;
					p->right = q;
					node->right = p->left;
					p->left = node;
					node = p;
				}
				else if (value > node->right->data) {
					q = node->right;
					node->right = q->left;
					q->left = node;
					p = q->right;
					q->right = p->left;
					p->left = q;
					node = p;
				}
			}
		}
	}

	PNode Merge(PNode t1, PNode t2) {
		if (!t1) return t2;
		if (!t2) return t1;
		PNode q = t1;
		while (q->right != nullptr) q = q->right;
		this->Splay(t1, q->data);
		t1->right = t2;
		return t1;
	}

	tuple<PNode, PNode, PNode> Split(PNode node, const T& value) {
		if (!node)
			return { nullptr, nullptr, nullptr };

		T minElem = T{};
		bool minElemInit = false;
		PNode q = node;
		while (q != nullptr) {
			if (q->data >= value) {
				if (!minElemInit) {
					minElem = q->data;
				}
				minElem = std::min(minElem, q->data);
				minElemInit = true;
			}
			if (value > q->data) {
				q = q->right;
			}
			else if (value < q->data) {
				q = q->left;
			}
		}
		if (!minElemInit) {
			return { node, nullptr, nullptr };
		}
		this->Splay(node, minElem);
		if (node->data != value) {
			PNode left = node->left;
			node->left = nullptr;
			return { left, nullptr, node };
		}
		PNode left = node->left;
		PNode right = node->right;
		node->left = node->right = nullptr;
		return { left, node, right };
	}

	bool Exists(const T& value) {
		PNode q = this->root;
		while (q != nullptr) {
			if (value > q->data) {
				q = q->right;
			}
			else if (value < q->data) {
				q = q->left;
			}
			else {
				return true;
			}
		}
		return false;
	}

	void InsertInternal(const T& value, bool* result) {
		if (this->Exists(value)) {
			*result = false;
			return;
		}
			
		auto [low, mid, high] = this->Split(this->root, value);

		PNode q = new Node(value);
		q->left = low;
		q->right = high;
		this->root = q;
		*result = true;
	}

	void DeleteInternal(PNode head, const T& x, bool* result) {
		if (!this->root)
			return;
		this->Splay(this->root, x);
		if (this->root->data != x) {
			return;
		}
		PNode left = this->root->left;
		PNode right = this->root->right;
		this->root->left = this->root->right = nullptr;
		delete this->root;
		this->root = this->Merge(left, right);
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
	virtual bool UsesAutoShrink() {
		return true;
	}

	SplayTree() {
		this->root = nullptr;
	}
};

