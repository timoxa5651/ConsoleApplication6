#include "Utils.h"

using namespace std::chrono;

__int64 Utils::startTime = Utils::TimeInternal();

__int64 Utils::TimeInternal() {
	return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

float Utils::Time() {
	return (Utils::TimeInternal() - Utils::startTime) / 1000.f;
}