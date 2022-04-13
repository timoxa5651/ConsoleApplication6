#pragma once
#include <algorithm>

template <typename T, class V>
class BaseNode {
	/*template<typename> friend class AVLTree;
	friend V;

	V* left;
	V* right;
	T data;*/

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

template <typename T>
class BaseTree
{
public:
	BaseTree() = default;
	virtual bool Insert(const T& value) = 0;
};

