#include "Utils.h"

using namespace std::chrono;

__int64 Utils::startTime = Utils::TimeInternal();

__int64 Utils::TimeInternal() {
	return duration_cast<milliseconds>(high_resolution_clock::now().time_since_epoch()).count();
}

double Utils::Time() {
	__int64 val = Utils::TimeInternal() - Utils::startTime;
	return val / 1000.0;
}