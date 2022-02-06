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
public:
	Node<T>* next;
	T data;

	Node(T& dt, Node<T>* nxt = nullptr) {
		this->next = nxt;
		this->data = dt;
	}

	Node<T>* walk() {
		return this->next;
	}
};

template<typename T = int>
class List {
	using iterator = Node<T>*;
	iterator head;

	template<typename _Pred>
	iterator MakePartition(iterator head, iterator end, _Pred pred, iterator* newHead, iterator* newMid, iterator* newEnd) {
		iterator pivot = end;
		iterator prev = nullptr, cur = head, tail = pivot;
		*newMid = tail;
		while (cur != end) {
			if (cur->data == pivot->data) {
				if (prev)
					prev->next = cur->next;
				iterator tmp = cur->next;
				cur->next = (*newMid)->next;
				(*newMid)->next = cur;
				if (tail == *newMid)
					tail = cur;
				*newMid = cur;
				cur = tmp;
			}
			else if (pred(cur->data, pivot->data)) {
				if (prev)
					prev->next = cur->next;
				iterator tmp = cur->next;
				cur->next = nullptr;
				tail->next = cur;
				tail = cur;
				cur = tmp;
			}
			else
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
	}

	template<typename _Pred>
	iterator Sort(iterator start, iterator end, _Pred pred) {
		if (!start || !end || start == end)
			return start;
		iterator newHead = nullptr, newEnd = nullptr, newMid = nullptr;
		iterator pivot = this->MakePartition(start, end, pred, &newHead, &newMid, &newEnd);

		/*iterator t1 = newHead;
		while (t1 != pivot) {
			cout << t1->data << " ";
			t1 = t1->next;
		}
		cout << "ddd ";
		while (t1 != newMid) {
			cout << t1->data << " ";
			t1 = t1->next;
		}
		cout << "eee ";
		while (t1 != newEnd) {
			cout << t1->data << " ";
			t1 = t1->next;
		}
		cout << "\n";*/

		if (newHead != pivot) { // head -> pivot
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
		// (pivot, mid] - same elements, skip
		// mid -> end
		if(newMid->next) //not end
			newMid->next = this->Sort(newMid->next, newEnd, pred);

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
	//list->InsertBefore(list->InsertFirst(7), 6);
	//list->InsertAfter(list->InsertAfter(list->InsertFirst(7), 8), 8);
	//list->InsertBefore(list->InsertFirst(6), 7);
	//list->InsertBefore(list->InsertFirst(9), 5);
	
	for (int i = 1; i <= 10; ++i)
		list->InsertFirst(i);

	for (auto it = list->begin(); it != list->end(); it = it->walk()) {
		cout << it->data << " ";
	}
	cout << endl;

	list->Sort(std::greater<int>());

	for (auto it = list->begin(); it != list->end(); it = it->walk()) {
		cout << it->data << " ";
	}
	cout << endl;

	return 0;
}
