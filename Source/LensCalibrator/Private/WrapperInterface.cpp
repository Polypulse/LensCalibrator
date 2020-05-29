#include "WrapperInterface.h"
#include "WrapperLogQueue.h"
#include <string>

void WrapperInterface::PollLog()
{
	if (GetWrapperLogQueue().LogIsQueued())
		return;

	int peekedSize = GetWrapperLogQueue().PeekNextSize();
	std::string stdMsg(peekedSize, ' ');

	int messageType;
	GetWrapperLogQueue().DequeueLog(&stdMsg[0], messageType);

	FString msg(UTF8_TO_TCHAR(stdMsg.c_str()));

	if (messageType == 0)
	{
		UE_LOG(LogTemp, Log, TEXT("Wrapper (INFO): %s"), *msg);
	}

	else if (messageType == 1)
	{
		UE_LOG(LogTemp, Warning, TEXT("Wrapper (WARNING): %s"), *msg);
	}

	else if (messageType == 2)
	{
		UE_LOG(LogTemp, Error, TEXT("Wrapper (ERROR): %s"), *msg);
	}
}
