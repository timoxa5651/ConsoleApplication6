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

class sfc64 {
public:
	sfc64(uint64_t seed = 0) : m_a(seed), m_b(seed), m_c(seed), m_counter(1) {
		for (int i = 0; i < 12; ++i) {
			this->next();
		}
	}

	uint64_t next() {
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
public:
	sfc64 rnd;

	BaseAlgo(uint64_t seed = 0) {
		this->rnd = sfc64(seed);
	}

	__forceinline virtual void Sort(T* pBuffer, long long cbSize) = 0;

	__forceinline virtual void FillRandom(uint8_t* pBytes, long long cbSize) {
		assert(cbSize % 8 == 0);

		uint64_t* ptr = (uint64_t*)pBytes;
		for (long long idx = 0; idx < cbSize / 8; ++idx) {
			*ptr++ = this->rnd.next();
		}

		T* dPtr = (T*)pBytes;
		for (long long idx = 0; idx < cbSize / sizeof(T); ++idx) {
			//&= ~(1 << (sizeof(T) * 8 - 1));
			if (dPtr[idx] < 0)
				dPtr[idx] = -dPtr[idx];
		}
	}

	virtual double Calculate(long long cbInput) {
		T* input = new T[cbInput];
		this->FillRandom((uint8_t*)input, sizeof(T) * cbInput);

		auto start = chrono::steady_clock::now();
		this->Sort(input, cbInput);
		auto end = chrono::steady_clock::now();

		/*for (int i = 0; i < cbInput; ++i) {
			cout << input[i] << endl;
		}*/
#ifdef _DEBUG
		for (int i = 1; i < cbInput; ++i) {
			if (!isnan(input[i])) {
				assert(input[i - 1] <= input[i]);
			}
		}
#endif

		delete[] input;
		return chrono::duration_cast<chrono::milliseconds>(end - start).count() + chrono::duration_cast<chrono::microseconds>(end - start).count() * 0.001;
	}
};

template<typename T>
class STLSort : public BaseAlgo<T> {
public:
	STLSort(uint64_t seed = 0) : BaseAlgo<T>(seed) {};

	__forceinline virtual void Sort(T* pBuffer, long long cbSize) {
		std::sort(pBuffer, pBuffer + cbSize);
	}
};

template<typename T>
class QSTLSort : public BaseAlgo<T> {
public:
	QSTLSort(uint64_t seed = 0) : BaseAlgo<T>(seed) {};

	__forceinline virtual void Sort(T* pBuffer, long long cbSize) {
		std::qsort(pBuffer, cbSize, sizeof(T),
			[](const void* x, const void* y) {
				if (*(T*)x < *(T*)y)
					return -1;
				if (*(T*)x > *(T*)y)
					return 1;
				return 0;
			});
	}
};

template<typename T>
class RadixSort : public BaseAlgo<T> {
public:
	T* outBuffer;
	RadixSort(uint64_t seed = 0) : BaseAlgo<T>(seed) {};

	virtual double Calculate(long long cbInput) {
		outBuffer = new T[cbInput];
		double result = BaseAlgo<T>::Calculate(cbInput);
		delete[] outBuffer;
		return result;
	}

	void RSort_step(T* source, T* dest, unsigned int n, unsigned int* offset, unsigned char sortable_bit)
	{
		unsigned char* b = (unsigned char*)&source[n] + sortable_bit;
		T* v = &source[n];
		while (v >= source)
		{
			dest[--offset[*b]] = *v--;
			b -= sizeof(T);
		}
	}

	__forceinline virtual void Sort(T* pBuffer, long long cbSize) {
		unsigned int s[sizeof(T) * 256] = { 0 };
		unsigned char* b = (unsigned char*)&pBuffer[cbSize - 1];
		while (b >= (unsigned char*)&pBuffer[0])
		{
			for (unsigned int digit = 0; digit < sizeof(T); digit++)
			{
				s[*(b + digit) + 256 * digit]++;
			}
			b -= sizeof(T);
		}
		for (unsigned int i = 1; i < 256; i++)
		{
			for (unsigned int digit = 0; digit < sizeof(T); digit++)
			{
				s[i + 256 * digit] += s[i - 1 + 256 * digit];
			}
		}

		for (unsigned int digit = 0; digit < sizeof(T); digit++)
		{
			RSort_step(pBuffer, outBuffer, cbSize - 1, &s[256 * digit], digit);
			T* temp = pBuffer;
			pBuffer = outBuffer;
			outBuffer = temp;
		}
	}
};

template<typename T>
class QuickSort : public BaseAlgo<T> {
	__forceinline void _Sort(T* pBuffer, long long left, long long right) {
		const long long kLeft = left;
		const long long kRight = right;
		T pivot = pBuffer[(left + right) / 2];

		while (left <= right)
		{
			while (pBuffer[left] < pivot)
				left++;
			while (pBuffer[right] > pivot)
				right--;
			if (left <= right)
			{
				swap(pBuffer[left], pBuffer[right]);
				++left, --right;
			}
		}
		if (right > kLeft) {
			_Sort(pBuffer, kLeft, right);
		}	
		if (left < kRight) {
			_Sort(pBuffer, left, kRight);
		}	
	}

public:
	QuickSort(uint64_t seed = 0) : BaseAlgo<T>(seed) {};

	__forceinline virtual void Sort(T* pBuffer, long long cbSize) {
		this->_Sort(pBuffer, 0, cbSize - 1);
	}
};

template<typename T>
class InsertionSort : public BaseAlgo<T> {
public:
	InsertionSort(uint64_t seed = 0) : BaseAlgo<T>(seed) {};

	__forceinline virtual void Sort(T* pBuffer, long long cbSize) {
		for (T* i = pBuffer + 1; i < pBuffer + cbSize; i++) {
			T* j = i;
			while (j > pBuffer && *(j - 1) > *j) {
				swap(*(j - 1), *j);
				j--;
			}
		}
	}
};

int main()
{
	srand(time(0));
	uint64_t seed = time(0);

	long long size = 3 * 1e4;
	using test_type = double;
	cout << "sorting size " << size << endl;
	//QSTLSort<test_type>* qsort = new QSTLSort<test_type>(seed);

	for (int i = 0; i < 7; ++i) {
		InsertionSort<test_type>* sort = new InsertionSort<test_type>(seed);
		cout << "sort " << sort->Calculate(size) << "ms\n";
	}
	//RadixSort<test_type>* radix = new RadixSort<test_type>((uint64_t)time(0) * i);

	//cout << "sort " << sort->Calculate(size) << "ms\n";
	//cout << "qsort " << qsort->Calculate(size) << "ms\n";
	cin.get();
	cin.get();

	return 0;
}
