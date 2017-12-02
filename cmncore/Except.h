// Copyright (c) 2017 Quenza Inc. All rights reserved.
//
// This file is part of the Cryptox project.
//
// Use of this source code is governed by a private license
// that can be found in the LICENSE file. Content can not be 
// copied and/or distributed without the express of the author.

#pragma once

#include <stdexcept>

namespace CmnCore
{
namespace Except
{

class UnsupportedOperationException : public std::runtime_error
{
public:
	UnsupportedOperationException(const std::string& message) noexcept
		: std::runtime_error{ message }
	{
	}

	explicit UnsupportedOperationException(char const* const message) noexcept
		: std::runtime_error{ message }
	{
	}

	virtual const char *what() const noexcept
	{
		return std::runtime_error::what();
	}
};

class IncompatibleException : public std::runtime_error
{
public:
	IncompatibleException(const std::string& message) noexcept
		: std::runtime_error{ message }
	{
	}

	explicit IncompatibleException(char const* const message) noexcept
		: std::runtime_error{ message }
	{
	}

	virtual const char *what() const noexcept
	{
		return std::runtime_error::what();
	}
};


} // namespace Except
} // namespace CmnCore
