#include <iostream>
#include <string>
#include <vector>
#include <windows.h>

namespace
{
	unsigned long const
		checkDelay = 10,
		windowDelay = 2000,
		errorDelay = 3000;
}

bool getWindowTitle(HWND handle, std::string & output);
BOOL CALLBACK windowEnumerationHandler(HWND hwnd, LPARAM lParam);
DWORD WINAPI keyboardThread(LPVOID lpParameter);

class FullscreenChat
{
public:

	FullscreenChat(std::string const & gameTitle, std::string const & chatTitle);
	void performScreenCheck();
	void processWindow(HWND handle);
	bool determineChatWindow();
	void createKeyboardThread();
	void keyboardHandler();

private:

	std::string
		gameTitle,
		chatTitle;

	bool inGame;
	bool chattingMode;
	HWND chatHandle;
	bool gotChatHandle;
	bool keyStates[0x100];
	bool shift;

	void printErrorAndWait(std::string const & message);
};

FullscreenChat::FullscreenChat(std::string const & chatTitle, std::string const & gameTitle):
	gameTitle(gameTitle),
	chatTitle(chatTitle),
	inGame(false),
	chattingMode(false),
	gotChatHandle(false)
{
	for(std::size_t i = 0; i < sizeof(keyStates); i++)
		keyStates[i] = false;
}

bool FullscreenChat::determineChatWindow()
{
	if(!EnumWindows(&windowEnumerationHandler, reinterpret_cast<LPARAM>(this)))
		return false;
	return gotChatHandle;
}

void FullscreenChat::createKeyboardThread()
{
	CreateThread(0, 0, &keyboardThread, static_cast<LPVOID>(this), 0, 0);
}

char capitaliseKey(char input)
{
	if(input >= 'a' && input <= 'z')
		return input - 'a' + 'A';

	return input;
}

void FullscreenChat::keyboardHandler()
{
	shift = false;
	while(true)
	{
		for(int i = 0; i < sizeof(keyStates); i++)
		{
			SHORT state = GetAsyncKeyState(i);
			bool isShiftKey = i == VK_SHIFT || i == VK_LSHIFT || i == VK_RSHIFT;
			bool wasPressed = (state & 1) != 0;
			if(wasPressed)
			{
				if(i == VK_F1)
				{
					chattingMode = !chattingMode;
					std::string message =
						chattingMode ?
							"Chatting mode activated" :
							"Chatting mode deactivated";
					std::cout << message << std::endl;
				}
				else if(chattingMode && !isShiftKey)
				{
					char key = i;
					if(shift)
						key = capitaliseKey(key);
					std::cout << shift << " " <<  key << " (" << i << ")" << std::endl;
					//SendMessage(chatHandle, WM_KEYDOWN, key, 0);
					//SendMessage(chatHandle, WM_KEYUP, key, 0);
					SendMessage(chatHandle, WM_CHAR, key, 0);
				}
			}
			bool & isDown = keyStates[i];
			bool newIsDown = (state >> (sizeof(SHORT) * 8 - 1)) != 0;
			isDown = newIsDown;
			if(isShiftKey)
			{
				if(isDown)
				{
					//std::cout << "Shift state: " << state << std::endl;
				}
				shift = isDown;
			}
		}
		Sleep(checkDelay);
	}
}

void FullscreenChat::printErrorAndWait(std::string const & message)
{
	std::cout << message << std::endl;
	Sleep(errorDelay);
}

void FullscreenChat::performScreenCheck()
{
	while(true)
	{
		HWND windowHandle = GetForegroundWindow();
		if(windowHandle == 0)
		{
			//printErrorAndWait("Unable to get the active window");
			Sleep(checkDelay);
			continue;
		}
		std::string windowTitle;
		if(!getWindowTitle(windowHandle, windowTitle))
		{
			//printErrorAndWait("Unable to retrieve the title of the active window");
			Sleep(checkDelay);
			continue;
		}
		bool newInGame = windowTitle.find(gameTitle) != std::string::npos;
		//std::cout << gameTitle << ": " << windowTitle << ", " << newInGame << std::endl;
		if(inGame != newInGame)
		{
			std::string message =
				newInGame ?
					"Entered the game" :
					"Left the game";

			std::cout << message << std::endl;

			inGame = newInGame;

			if(inGame)
			{
				Sleep(windowDelay);

				RECT gameRectangle;
				if(!GetWindowRect(windowHandle, &gameRectangle))
				{
					printErrorAndWait("Unable to get game window rectangle");
					continue;
				}

				RECT chatRectangle;
				if(!GetWindowRect(chatHandle, &chatRectangle))
				{
					printErrorAndWait("Unable to get chat window rectangle");
					continue;
				}

				SetWindowPos(chatHandle, HWND_NOTOPMOST, gameRectangle.right, chatRectangle.top, 0, 0, SWP_NOSIZE);
			}
			else
			{
				Sleep(windowDelay);

				ShowWindow(chatHandle, SW_MINIMIZE);
				ShowWindow(chatHandle, SW_MAXIMIZE);
			}
		}
		Sleep(checkDelay);
	}
}

void FullscreenChat::processWindow(HWND handle)
{
	std::string title;
	if(!getWindowTitle(handle, title))
		return;

	if(title.find(chatTitle) != std::string::npos)
	{
		chatHandle = handle;
		gotChatHandle = true;
	}
}

bool getWindowTitle(HWND handle, std::string & output)
{
	char buffer[256];
	int stringLength = GetWindowText(handle, buffer, static_cast<int>(sizeof(buffer)));
	if(stringLength == 0)
		return false;
	output = buffer;
	return true;
}

BOOL CALLBACK windowEnumerationHandler(HWND hwnd, LPARAM lParam)
{
	FullscreenChat & chat = *reinterpret_cast<FullscreenChat *>(lParam);
	chat.processWindow(hwnd);
	return TRUE;
}

DWORD WINAPI keyboardThread(LPVOID lpParameter)
{
	FullscreenChat & chat = *reinterpret_cast<FullscreenChat *>(lpParameter);
	chat.keyboardHandler();
	return 0;
}

int main(int argc, char ** argv)
{
	if(argc != 3)
	{
		std::cout << "Usage:" << std::endl;
		std::cout << argv[0] << " <chat window partial title> <game window partial title>" << std::endl;
		return 1;
	}
	FullscreenChat chat(argv[1], argv[2]);
	if(!chat.determineChatWindow())
	{
		std::cout << "Unable to determine the chat window" << std::endl;
		return 1;
	}
	chat.createKeyboardThread();
	chat.performScreenCheck();
	return 0;
}
