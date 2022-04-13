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
