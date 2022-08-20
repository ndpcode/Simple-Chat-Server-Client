
#include <iostream>
#include "SimpleChatServer.h"

int main()
{
	SimpleChatServer myChatServer;

	myChatServer.AddBot();
	myChatServer.AddBot();

	myChatServer.Start(9001);

	return 0;
}
