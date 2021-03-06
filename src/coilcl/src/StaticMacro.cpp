// Copyright (c) 2017 Quenza Inc. All rights reserved.
//
// This file is part of the Cryptox project.
//
// Use of this source code is governed by a private license
// that can be found in the LICENSE file. Content can not be 
// copied and/or distributed without the express of the author.

#include "Preprocessor.h"

#include <CryCC/SubValue.h>

#include <cassert>

inline constexpr static const char *CILDateFormat{ "%b %d %Y" };
inline constexpr static const char *CILTimeFormat{ "%R:%S" };

static int g_staticTokenProcessorCounter = 0;

namespace CoilCl::MacroHelper
{

// Keep global counter throughout the entire code processing.
// On each invokation the counter is incremented by one.
TokenProcessor::DataType DynamicGlobalCounter()
{
	int counter = g_staticTokenProcessorCounter++;
	return Util::MakeInt(counter);
}

TokenProcessor::DataType DynamicSourceFile()
{
	//TODO:
	return Util::MakeString("somefile.c");
}

// Return the current source code line.
TokenProcessor::DataType DynamicSourceLine()
{
	//TODO:
	return Util::MakeInt(0);
}

// Return the current local date.
TokenProcessor::DataType DynamicDate()
{
	struct tm timeinfo;
	CRY_LOCALTIME(&timeinfo);
	char buffer[12];

	strftime(buffer, sizeof(buffer), CILDateFormat, &timeinfo);
	return Util::MakeString(buffer);
}

// Return the current local time.
TokenProcessor::DataType DynamicTime()
{
	struct tm timeinfo;
	CRY_LOCALTIME(&timeinfo);
	char buffer[12];

	strftime(buffer, sizeof(buffer), CILTimeFormat, &timeinfo);
	return Util::MakeString(buffer);
}

} // namespace CoilCl::MacroHelper
