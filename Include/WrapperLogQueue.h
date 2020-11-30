/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */

#pragma once
#include <queue>
#include <mutex>

struct WrapperLogContainer
{
	int messageType; // 0 Info, 1 Warning, 2 Error.
	std::string message;
};

class WrapperLogQueue
{
private:
	std::queue<WrapperLogContainer> logQueue;
	std::mutex queueMutex;
public:
	WrapperLogQueue() {}
	WrapperLogQueue(WrapperLogQueue const&) = delete;
	void operator=(WrapperLogQueue const&) = delete;

	__declspec(dllexport) bool LogIsQueued ();
	__declspec(dllexport) int PeekNextSize ();
	__declspec(dllexport) int DequeueLog (char * buffer, int & messageType);
	__declspec(dllexport) void QueueLog (std::string message, int messageType);
};

extern "C" __declspec(dllexport) WrapperLogQueue & GetWrapperLogQueue ();
