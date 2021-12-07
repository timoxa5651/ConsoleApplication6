#include <any>
#define FMT_HEADER_ONLY
#include "fmt/format.h"
#include "SFML/Graphics.hpp"
#include <string>
#include <iostream>
#include <cassert>
#include <random>
#include <chrono>

using namespace std;

class sfc64 {
public:
	using result_type = uint64_t;

	sfc64() : sfc64(std::random_device{}()) {}

	explicit sfc64(uint64_t seed) : m_a(seed), m_b(seed), m_c(seed), m_counter(1) {
		for (int i = 0; i < 12; ++i) {
			operator()();
		}
	}

	uint64_t operator()() noexcept {
		auto const tmp = m_a + m_b + m_counter++;
		m_a = m_b ^ (m_b >> right_shift);
		m_b = m_c + (m_c << left_shift);
		m_c = rotl(m_c, rotation) + tmp;
		return tmp;
	}

private:
	template <typename T> T rotl(T const x, int k) { return (x << k) | (x >> (8 * sizeof(T) - k)); }

	static constexpr int rotation = 24;
	static constexpr int right_shift = 11;
	static constexpr int left_shift = 3;
	uint64_t m_a;
	uint64_t m_b;
	uint64_t m_c;
	uint64_t m_counter;
};

template<typename T>
class BaseAlgo {
	__forceinline virtual void Sort(T* pBuffer, long long cbSize) = 0;

	__forceinline virtual void FillRandom(uint8_t* pBytes, long long cbSize) {
		assert(cbSize % 8 == 0);
		sfc64 rnd = sfc64(time(0));

		uint64_t* ptr = (uint64_t*)pBytes;
		for (long long idx = 0; idx < cbSize / 8; ++idx) {
			*ptr++ = rnd();
		}

		T* dPtr = (T*)pBytes;
		for (long long idx = 0; idx < cbSize / sizeof(T); ++idx) {
			dPtr[idx] &= ~(1 << (sizeof(T) * 8 - 1));
		}
	}

public:
	BaseAlgo() = default;

	virtual double Calculate(long long cbInput) {
		T* input = new T[cbInput];
		this->FillRandom((uint8_t*)input, sizeof(T) * cbInput);
		auto start = chrono::steady_clock::now();

		this->Sort(input, cbInput);

		auto end = chrono::steady_clock::now();
		delete[] input;
		return chrono::duration_cast<chrono::milliseconds>(end - start).count() + chrono::duration_cast<chrono::microseconds>(end - start).count()  * 0.001;
	}
};

template<typename T>
class STLSort : public BaseAlgo<T> {
	__forceinline virtual void Sort(T* pBuffer, long long cbSize) {
		std::sort(pBuffer, pBuffer + cbSize);
	}
};

int main()
{
	srand(time(0));

	STLSort<long long>* sort = new STLSort<long long>();
	cout << sort->Calculate(1e8 * 3) << "ms";
	return 0;
}
