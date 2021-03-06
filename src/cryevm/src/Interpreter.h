// Copyright (c) 2017 Quenza Inc. All rights reserved.
//
// This file is part of the Cryptox project.
//
// Use of this source code is governed by a private license
// that can be found in the LICENSE file. Content can not be 
// copied and/or distributed without the express of the author.

#pragma once

#include "Planner.h"

namespace EVM
{

class Interpreter : public Strategy
{
public:
	Interpreter(Planner&);

	// Check if strategy can run the program.
	virtual bool IsRunnable() const noexcept;
	// Program entry point.
	virtual std::string EntryPoint(const char *);
	// Run the program with current strategy.
	virtual ReturnCode Execute(const std::string& entry, const ArgumentList&, const ArgumentList&);

protected:
	void PreliminaryCheck(const std::string&);

private:

};

} // namespace EVM
