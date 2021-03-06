// Copyright (c) 2017 Quenza Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#pragma once

#include <Cry/Indep.h>

#include "StoreConfig.h"

#include <stdint.h>
#include <memory.h>
#include <string.h>
#include <time.h>
#include <stdexcept>

namespace ProjectBase {

typedef uint64_t SequenceNumber;
typedef time_t TimeStamp;

// Defines project options which are required upfront in order to 
// read the store contents.
enum StoreFormatOptions
{
	StoreOptPlain = 0x1,
	StoreOptFullEncrypt = 0x2,
	StoreOptBackComp = 0x4,
	StoreOptUnpacked = 0x8,
};

struct StoreFormat
{
	// Magic value contains a readable string so that accidential ascii
	// conversions return a tracable dump
	unsigned char magic[16];

	// File format version number, this is only increased if the format changes
	unsigned short version;

	// Project store options
	enum StoreFormatOptions projopt;

	// The sequence number is increased every time the store is altered
	// this makes it possible to compare to project files of the same type
	SequenceNumber seqNum;

	// Create and update timestamps
	TimeStamp createdAt;
	TimeStamp updatedAt;

	// Number of object stores.
	unsigned int objectStores = 0;

	// Size of the file store header. Even if the structure is unknown, the parser knows the size
	// of the header and can skip to the actual contents
	unsigned hdrsz;

	// By default projet files do not have metadata. When metadata is added to the project file
	// this flag must be triggered
	bool hasMeta = false;

	StoreFormat()
		: version{ Config::version }
		, projopt{ StoreOptPlain }
		, seqNum{ 0 }
		, createdAt{ time(nullptr) }
		, updatedAt{ time(nullptr) }
	{
		CRY_MEMCPY(magic, 16, Config::BANNER, sizeof(Config::BANNER));
		hdrsz = sizeof(this);
	}

	void Reset()
	{
		projopt = static_cast<enum StoreFormatOptions>(0);
		version = 0;
		seqNum = 0;
		CRY_MEMSET(magic, '\0', 16);
	}

	void Validate()
	{
		if (strncmp(reinterpret_cast<const char *>(magic), reinterpret_cast<const char *>(Config::BANNER), sizeof(Config::BANNER))) {
			throw std::runtime_error{ "Magic value mismatch, illegal filetype" };
		}

		if (version < Config::version) {
			throw std::runtime_error{ "Incompatible version" };
		}
	}
};

} // namespace ProjectBase
