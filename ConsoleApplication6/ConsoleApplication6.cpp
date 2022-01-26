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

using std::cout;
using std::cin;
using std::endl;

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

	template<typename _Pred>
	iterator MakePartition(iterator head, iterator end, _Pred pred, iterator* newHead, iterator* newEnd) {
		iterator pivot = end;
		iterator prev = nullptr, cur = head, tail = pivot;
		while (cur != pivot) {
			if (pred(cur->data, pivot->data)) {
				if (prev)
					prev->next = cur->next;
				iterator tmp = cur->next;
				cur->next = nullptr;
				tail->next = cur;
				tail = cur;
				cur = tmp;
			}
			else // cur > pivot
			{
				if ((*newHead) == nullptr)
					(*newHead) = cur;
				prev = cur;
				cur = cur->next;
			}
		}
		if ((*newHead) == nullptr) {
			(*newHead) = pivot;
		}
		(*newEnd) = tail;
		return pivot;

		/*iterator pivot = head;
		iterator i = head, j = end, tail = end;
		while (true) {

			while (pred(i->data, pivot->data)) {
				i = i->next;
			}
			while (!pred(j->data, pivot->data)) {
				// j -= 1;
			}

		}*/
	}

	template<typename _Pred>
	iterator Sort(iterator start, iterator end, _Pred pred) {
		if (!start || !end || start == end)
			return start;
		iterator newHead = nullptr, newEnd = nullptr;
		iterator pivot = this->MakePartition(start, end, pred, &newHead, &newEnd);
		if (newHead != pivot) {
			iterator tmp = newHead;
			while (tmp->next != pivot)
				tmp = tmp->next;
			tmp->next = nullptr;
			newHead = this->Sort(newHead, tmp, pred);
			tmp = newHead;
			while (tmp->next) {
				tmp = tmp->next;
			}
			tmp->next = pivot;
		}
		pivot->next = this->Sort(pivot->next, newEnd, pred);

		return newHead;
	}
public:
	size_t size;

	List() {
		this->head = nullptr;
		this->size = 0;
	};
	~List() {
		while (this->head) {
			iterator nxt = this->head->next;
			delete this->head;
			this->head = nxt;
		}
	}

	iterator begin() {
		return this->head;
	}
	iterator end() {
		return nullptr;
	}

	iterator InsertFirst(T data) {
		this->head = new Node(data, this->head);
		this->size += 1;
		return this->head;
	}

	iterator InsertAfter(iterator element, T data) {
		if (!element)
			return nullptr;
		element->next = new Node(data, element->next);
		this->size += 1;
		return element->next;
	}

	iterator InsertBefore(iterator element, T data) {
		if (!element)
			return nullptr;
		iterator anext = new Node(element->data, element->next);
		element->data = data;
		element->next = anext;
		this->size += 1;
		return element;
	}

	void Delete(iterator iter) {
		if (!iter || !iter->next)
			return;
		iterator q = iter->next;
		iter->data = iter->next->data;
		iter->next = iter->next->next;
		delete q;
	}

	template<typename _Pred>
	void Sort(_Pred pred) {
		if (!this->head)
			return;
		iterator tail = this->head;
		while (tail->next) {
			tail = tail->next;
		}
		this->head = this->Sort(this->head, tail, pred);
	}

	void Sort() {
		return this->Sort(std::greater<T>());
	}
};

int main()
{
	auto list = new List<int>();
	list->InsertBefore(list->InsertFirst(7), 6);
	list->InsertAfter(list->InsertAfter(list->InsertFirst(1), 2), 3);
	list->InsertBefore(list->InsertFirst(5), 4);

	for (auto it = list->begin(); it != list->end(); it = it->walk()) {
		cout << it->data << " ";
	}
	cout << endl;

	list->Sort(std::greater<int>());

	for (auto it = list->begin(); it != list->end(); it = it->walk()) {
		cout << it->data << " ";
	}
	cout << endl;

	list->InsertAfter(list->begin(), 8);
	list->Sort(std::less<int>());

	for (auto it = list->begin(); it != list->end(); it = it->walk()) {
		cout << it->data << " ";
	}
	cout << endl;

	return 0;
}
