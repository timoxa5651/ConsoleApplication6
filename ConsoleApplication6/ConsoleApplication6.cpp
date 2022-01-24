#define FMT_HEADER_ONLY
#include "fmt/format.h"
#include "SFML/Graphics.hpp"
#include <string>
#include <iostream>
#include <cassert>
#include <random>
#include <chrono>
#include <any>

using namespace std;

template<typename T = int>
class List {
	struct Node {
		Node* next;
		T data;

		Node(T& dt) {
			this->data = dt;
		}
	};
	using iterator = Node*;

	iterator Head = nullptr;
public:

	List() {};

	void Push() {

	}
};

int main()
{
	srand(time(0));

	auto list = new List();



	return 0;
}
