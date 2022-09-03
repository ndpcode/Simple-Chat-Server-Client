//*********************************************************************************************************//
//Simple chat server demo app, for Skillbox.
//Created 20.03.2021
//Created by Novikov Dmitry
//*********************************************************************************************************//

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
