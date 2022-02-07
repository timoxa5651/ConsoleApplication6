#include <iostream>
#include <thread>

#include "BaseGame.h"
#include "Utils.h"
#include "windows.h"

using std::cout;
using std::cin;
using std::endl;

int main()
{
	srand(time(0));

	BaseGame::g_Instance = new BaseGame();

	BaseGame::g_Instance->Start();
	double lastUpdateTime = Utils::Time();
	while (BaseGame::g_Instance->IsRunning()) {
		BaseGame::g_Instance->Update(Utils::Time() - lastUpdateTime);
		lastUpdateTime = Utils::Time();
		std::this_thread::sleep_for(std::chrono::microseconds(100));
	}
	return 0;
}
