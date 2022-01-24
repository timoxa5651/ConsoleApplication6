#define FMT_HEADER_ONLY
#include "fmt/format.h"
#include "SFML/Graphics.hpp"
#include <string>
#include <iostream>
#include <cassert>
#include <random>
#include <chrono>
#include <any>
#include <list>

using namespace std;

template<typename T = int>
struct Node {
	using iterator = Node<T>*;

	iterator next;
	T data;

	Node(T& dt, iterator nxt = nullptr) {
		this->next = nxt;
		this->data = dt;
	}

	iterator walk() {
		return this->next;
	}
};

template<typename T = int>
class List {
	using iterator = Node<T>::iterator;
	iterator head;

	using Predicate = bool(*)(T&, T&);
	void _Sort(iterator start, int len, Predicate pred) {
		// order [_First, _First + _Size), return _First + _Size
		if (len == 0) {
			return start;
		}
		else if (len == 1) {
			return start->next;
		}

		auto _Mid = _Sort(_First, _Size / 2, pred);
		const auto _Last = _Sort(_Mid, _Size - _Size / 2, pred);
		_First = _Merge_same(_First, _Mid, _Last, pred);
		return _Last;
	}
public:
	size_t size;

	List() {
		this->head = nullptr;
		this->size = 0;
	};

	iterator begin() {
		return this->head;
	}
	iterator end() {
		return nullptr;
	}
	
	iterator InsertFirst(T data) {
		if (!element)
			return nullptr;
		this->head = new Node(data, this->head);
		return this->head;
	}

	iterator InsertAfter(iterator element, T data) {
		if (!element)
			return nullptr;
		element->next = new Node(data, element->next);
		return element->next;
	}

	iterator InsertBefore(iterator element, T data) {
		if (!element)
			return nullptr;
		iterator anext = new Node(element->data, element->next);
		element->data = data;
		element->next = anext;
		return element;
	}

	void Sort(Predicate pred = greater<T>()) {
		return this->_Sort(this->begin(), this->size, pred);
	}
};

int main()
{
	srand(time(0));

	auto list = new List();
	list->InsertBefore(list->InsertFirst(7), 6);
	list->InsertBefore(list->InsertFirst(5), 4);
	list->InsertAfter(list->InsertAfter(list->InsertFirst(1), 2), 3);

	for (auto it = list->begin(); it != list->end(); it = it->walk()) {
		cout << it->data << " ";
	}

	std::list<int> lst;
	lst.sort();

	cout << endl;
	return 0;
}
