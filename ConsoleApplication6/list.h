#pragma once
#include <type_traits>
template<typename T = int>
struct Node {
public:
	Node<T>* next;
	Node<T>* prev;
	T data;

	Node(const T& dt, Node<T>* nxt = nullptr, Node<T>* prv = nullptr) {
		this->next = nxt;
		this->prev = prv;
		this->data = dt;
	}

	~Node() {
		if (is_pointer<T>::value && this->data) {
			delete this->data;
		}
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
	iterator Sort(iterator start, _Pred pred) {
		if (!start || !start->next)
			return start;
		iterator right = start;
		iterator temp = start;
		while (temp && temp->next)
		{
			right = right->next;
			temp = temp->next->next;
		}

		if (right->prev)
			right->prev->next = nullptr; // cut in half

		iterator list = this->Sort(start, pred);
		right = this->Sort(right, pred);

		iterator next = nullptr;
		iterator result = nullptr;
		iterator tail = nullptr;
		while (list || right)
		{
			if (!right) {
				next = list;
				list = list->next;
			}
			else if (!list) {
				next = right;
				right = right->next;
			}
			else if (!pred(list->data, right->data)) {
				next = list;
				list = list->next;
			}
			else {
				next = right;
				right = right->next;
			}
			if (!result) {
				result = next;
			}
			else {
				tail->next = next;
			}
			next->prev = tail;
			tail = next;
		}
		return result;
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

	iterator& begin() {
		return this->head;
	}
	iterator end() {
		return nullptr;
	}

	iterator InsertFirst(T data) {
		this->head = new Node<T>(data, this->head, nullptr);
		if (this->head->next)
			this->head->next->prev = this->head;
		this->size += 1;
		return this->head;
	}

	iterator InsertLast(T data) {
		if (!this->head)
			return this->InsertFirst(data);
		Node<T>* tmp = this->head;
		while (tmp->next) {
			tmp = tmp->next;
		}
		return this->InsertAfter(tmp, data);
	}

	iterator InsertAfter(iterator element, T data) {
		if (!element)
			return nullptr;

		element->next = new Node(data, element->next, element);
		if (element->next->next)
			element->next->next->prev = element->next;
		this->size += 1;
		return element->next;
	}

	iterator InsertBefore(iterator element, T data) {
		if (!element)
			return nullptr;
		if (element->prev)
			return this->InsertAfter(element->prev, data);
		return this->InsertFirst(data);
	}

	void Delete(iterator iter) {
		if (!iter)
			return;
		if (iter->prev)
			iter->prev->next = iter->next;
		else
			this->head = iter->next;

		if (iter->next)
			iter->next->prev = iter->prev;
		delete iter;
		this->size -= 1;
	}

	iterator operator[](int idx) {
		iterator it = this->begin();
		while (--idx >= 0)
			it = it->next;
		return it;
	}

	template<typename _Pred>
	void Sort(_Pred pred) {
		this->head = this->Sort(this->head, pred);
	}
};