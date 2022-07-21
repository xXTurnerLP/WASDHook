#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <atomic>
#include <mutex>
#include <Windows.h>

#pragma comment(lib, "Winmm.lib")

LRESULT CALLBACK LowLevelKeyboardProc(
	_In_ int    nCode,
	_In_ WPARAM wParam,
	_In_ LPARAM lParam
);
void InjectChangeDirection(char oldKey, char intermediateKey, char newKey, uint64_t ms_interval);

int main()
{
	fclose(stdin);
	HHOOK hLowLevelKBHook = SetWindowsHookExA(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(NULL), NULL);
	if (!hLowLevelKBHook)
	{
		MessageBoxA(0, "Error while installing ll hook", std::to_string(GetLastError()).c_str(), 0);
		return -1;
	}

	PlaySound("On.wav", NULL, SND_ASYNC | SND_FILENAME);
	puts("Turn on/off with INSERT");

	MSG msg{};
	while (GetMessage(&msg, NULL, NULL, NULL))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	
	UnhookWindowsHookEx(hLowLevelKBHook);
}

enum Directions : uint8_t
{
	UNDEFINED,
	UP,
	DOWN,
	LEFT,
	RIGHT
};

uint8_t g_LastUDDirection = UNDEFINED;
uint8_t g_LastLRDirection = UNDEFINED;
bool isPressed_W = false;
bool isPressed_A = false;
bool isPressed_S = false;
bool isPressed_D = false;

bool enabled = true;

LRESULT CALLBACK LowLevelKeyboardProc(
	_In_ int    nCode,
	_In_ WPARAM wParam,
	_In_ LPARAM lParam
)
{
	if (nCode >= 0)
	{
		KBDLLHOOKSTRUCT* kb = (KBDLLHOOKSTRUCT*)lParam;

		if (kb->vkCode == VK_INSERT && wParam == WM_KEYDOWN)
		{
			enabled = !enabled;
			if (enabled)
				PlaySound("On.wav", NULL, SND_ASYNC | SND_FILENAME);
			else
				PlaySound("Off.wav", NULL, SND_ASYNC | SND_FILENAME);

			goto ret;
		}

		if (!enabled)
			goto ret;

		bool shouldSuppressKey = (kb->vkCode == 'W' || kb->vkCode == 'A' || kb->vkCode == 'S' || kb->vkCode == 'D') &&
			!(kb->flags & LLKHF_INJECTED);

		kb->flags &= ~LLKHF_INJECTED; // Clear injected flag. Remove trace for anti cheat (if anticheat gets the input after the hook)

		if (shouldSuppressKey)
		{
			switch (kb->vkCode)
			{
			case 'W':
				isPressed_W = wParam == WM_KEYDOWN;
				break;
			case 'A':
				isPressed_A = wParam == WM_KEYDOWN;
				break;
			case 'S':
				isPressed_S = wParam == WM_KEYDOWN;
				break;
			case 'D':
				isPressed_D = wParam == WM_KEYDOWN;
				break;
			}

			if (wParam == WM_KEYUP)
				goto ret;
			 
			if (kb->vkCode == 'W' && g_LastUDDirection == UP) goto ret;
			if (kb->vkCode == 'S' && g_LastUDDirection == DOWN) goto ret;
			if (kb->vkCode == 'A' && g_LastLRDirection == LEFT) goto ret;
			if (kb->vkCode == 'D' && g_LastLRDirection == RIGHT) goto ret;

			if ((isPressed_A || isPressed_D) && (isPressed_W || isPressed_S))
				goto ret;

			//printf("%c\t^v:%s\t<>:%s\n", (char)kb->vkCode, g_LastUDDirection == UNDEFINED ? "UNDEFINED" : g_LastUDDirection == UP ? "UP" : "DOWN", g_LastLRDirection == UNDEFINED ? "UNDEFINED" : g_LastLRDirection == LEFT ? "LEFT" : "RIGHT");

			if (kb->vkCode == 'W' && g_LastUDDirection == DOWN)
				std::thread(InjectChangeDirection, 'S', 'A', 'W', 10).detach();
			else if (kb->vkCode == 'S' && g_LastUDDirection == UP)
				std::thread(InjectChangeDirection, 'W', 'D', 'S', 10).detach();

			if (kb->vkCode == 'A' && g_LastLRDirection == RIGHT)
				std::thread(InjectChangeDirection, 'D', 'W', 'A', 10).detach();
			else if (kb->vkCode == 'D' && g_LastLRDirection == LEFT)
				std::thread(InjectChangeDirection, 'A', 'S', 'D', 10).detach();

			if (kb->vkCode == 'W')
				g_LastUDDirection = UP;
			else if (kb->vkCode == 'S')
				g_LastUDDirection = DOWN;

			if (kb->vkCode == 'A')
				g_LastLRDirection = LEFT;
			else if (kb->vkCode == 'D')
				g_LastLRDirection = RIGHT;

			return 1;
		}
	}

ret:
	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

void InjectChangeDirection(char oldKey, char intermediateKey, char newKey, uint64_t ms_interval)
{
	INPUT input{};

	input.type = INPUT_KEYBOARD;
	input.ki.wVk = oldKey;
	input.ki.dwFlags = KEYEVENTF_KEYUP;

	SendInput(1, &input, sizeof(input));

	std::this_thread::sleep_for(std::chrono::milliseconds(ms_interval));

	input.type = INPUT_KEYBOARD;
	input.ki.wVk = intermediateKey;
	input.ki.dwFlags = 0;

	SendInput(1, &input, sizeof(input));

	std::this_thread::sleep_for(std::chrono::milliseconds(ms_interval));

	input.type = INPUT_KEYBOARD;
	input.ki.wVk = newKey;
	input.ki.dwFlags = 0;

	SendInput(1, &input, sizeof(input));

	std::this_thread::sleep_for(std::chrono::milliseconds(ms_interval));

	input.type = INPUT_KEYBOARD;
	input.ki.wVk = intermediateKey;
	input.ki.dwFlags = KEYEVENTF_KEYUP;

	SendInput(1, &input, sizeof(input));
}
