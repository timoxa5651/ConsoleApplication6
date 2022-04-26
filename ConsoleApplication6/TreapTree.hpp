#pragma once
#include "BaseTree.hpp"
#include <tuple>
using std::tuple;

template<typename T>
class TreapNode : public BaseNode<T, TreapNode<T>> {
	template<typename> friend class TreapTree;

	static float GetNodeSize() {
		return 120.f;
	}

	TreapNode() {
		this->left = this->right = nullptr;
		this->priority = rand() * rand();
	};
	TreapNode(const T& val) : TreapNode() {
		this->data = val;
	}
public:
	__int64 priority;
};

template<typename T>
class TreapTree : public BaseTree<T>
{
	typedef TreapNode<T> Node, * PNode;

	PNode Merge(PNode t1, PNode t2) {
		if (!t1) return t2;
		if (!t2) return t1;

		if (t1->priority > t2->priority) {
			t1->right = this->Merge(t1->right, t2);
			return t1;
		}
		else {
			t2->left = this->Merge(t1, t2->left);
			return t2;
		}
	}

	tuple<PNode, PNode, PNode> Split(PNode node, const T& value) {
		if (!node) 
			return { nullptr, nullptr, nullptr };

		if (node->data < value) {
			auto [low, mid, high] = this->Split(node->right, value);
			node->right = nullptr;
			return { this->Merge(node, low), mid, high };
		}
		if (node->data > value) {
			auto [low, mid, high] = this->Split(node->left, value);
			node->left = nullptr;
			return { low, mid, this->Merge(high, node) };
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
		auto [low, mid, high] = this->Split(this->root, value);
		if (mid) {
			*result = false;
			return;
		}
		this->root = this->Merge(low, this->Merge(new Node(value), high));
		*result = true;
	}

	void DeleteInternal(PNode head, const T& value, bool* result) {
		if (!this->root)
			return;
		auto [low, mid, high] = this->Split(this->root, value);
		if (!mid)
			return;
		mid->left = mid->right = nullptr;
		delete mid;
		this->root = this->Merge(low, high);
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

	virtual bool Insert(const T& value) {
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
		return "Treap";
	}
	virtual bool UsesAutoShrink() {
		return true;
	}

	TreapTree() {
		this->root = nullptr;
	}
};

