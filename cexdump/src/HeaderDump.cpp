// Copyright (c) 2017 Quenza Inc. All rights reserved.
//
// This file is part of the Cryptox project.
//
// Use of this source code is governed by a private license
// that can be found in the LICENSE file. Content can not be 
// copied and/or distributed without the express of the author.

#include "HeaderDump.h"

#include <sstream>
#include <iostream>

using namespace CryExe;

namespace Detail
{

std::string ImageVersion(const CryExe::Executable& exec)
{
	const Meta::ImageVersionCompound& imageVersion = Meta::ImageVersion(exec);

	std::stringstream ss;
	ss << imageVersion.first << "." << imageVersion.second;

	if (Meta::IsLatestImageVersion(exec)) {
		ss << " (current)";
	}

	return ss.str();
}

std::string ImageIdentifier(const CryExe::Executable& exec)
{
	std::stringstream ss;
	ss << Meta::StructureIdentity();
	ss << " (checked)";

	return ss.str();
}

std::string ImageType(const CryExe::Executable& exec)
{
	ExecType type = Meta::ImageType(exec);

	std::stringstream ss;

	switch (type) {
	case ExecType::TYPE_EXECUTABLE:
		ss << "TYPE_EXECUTABLE";
		ss << " (Executable file)";
		break;
	case ExecType::TYPE_LIBRARY_DYNAMIC:
		ss << "TYPE_LIBRARY_DYNAMIC";
		ss << " (Dynamic library)";
		break;
	default:
		ss << "[UNKNOWN]";
	}

	return ss.str();
}

} // namespace Detail

void HeaderDump::ParseImageHeader(const CryExe::Executable& exec)
{
	std::cout << "Image Header:" << '\n'
		<< "  Version:                   " << Detail::ImageVersion(exec) << '\n'
		<< "  Identification:            " << Detail::ImageIdentifier(exec) << '\n'
		<< "  Type:                      " << Detail::ImageType(exec) << '\n'
		<< "  Start of program headers:  X (bytes into file)" << '\n'
		<< "  Flags:                     0x01" << '\n'
		<< "  Size of this header:       " << 0 << " (bytes)"
		<< std::endl;
}

void HeaderDump::ParseProgramHeader(const CryExe::Executable& exec)
{
	const std::string programVersion = Meta::ProgramVersion(exec);

	std::cout << "Program Header:" << '\n'
		<< "  Magic:                  7f 45 4c 46" << '\n'
		<< "  Timestamp:              YYYY-MM-DD HH:II:SS" << '\n'
		<< "  Subsystem Target:       Advanced Micro Devices X86 - 64" << '\n'
		<< "  Subsystem Version:      0x1" << '\n'
		<< "  Platform Runner:        Native" << '\n'
		<< "  Size of Code Segment:   X (bytes)" << '\n'
		<< "  Size of Stack:          X (bytes)" << '\n'
		<< "  Number of Sections:     X" << '\n'
		<< "  Number of Directories:  X" << '\n'
		<< "  Start of Sections:      64 (bytes into file)" << '\n'
		<< "  Start of Directories:   6232 (bytes into file)" << '\n'
		<< "  Size of this header:    X (bytes)"
		<< std::endl;
}
