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
	float lastUpdateTime = Utils::Time();
	while (BaseGame::g_Instance->IsRunning()) {
		BaseGame::g_Instance->Update(Utils::Time() - lastUpdateTime);
		lastUpdateTime = Utils::Time();
		Sleep(1);
	}
	return 0;
}
