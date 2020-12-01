/*
 * Copyright (C) 2020 - LensCalibrator contributors, see Contributors.txt at the root of the project.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include "WrapperInterface.h"
#include "WrapperLogQueue.h"
#include <string>

void WrapperInterface::PollLog()
{
	if (!GetWrapperLogQueue().LogIsQueued())
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
