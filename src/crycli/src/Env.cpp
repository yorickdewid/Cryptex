// Copyright (c) 2017 Quenza Inc. All rights reserved.
//
// This file is part of the Cryptox project.
//
// Use of this source code is governed by a private license
// that can be found in the LICENSE file. Content can not be 
// copied and/or distributed without the express of the author.

#include "Env.h"

#include <Cry/Cry.h>

#include <cstdlib>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#define ENV_STR_PREFIX "CRYCL_"
#define IMAGE_EXTENSION "cex"

using namespace boost::filesystem;

namespace Detail {

template<typename _Ty>
_Ty GetEnvVar(const char *str, _Ty d)
{
	using boost::lexical_cast;
	using boost::bad_lexical_cast;

	const char *envPtr = std::getenv(str);
	if (!envPtr) { return d; }

	try
	{
		return lexical_cast<_Ty>(envPtr);
	}
	catch (const bad_lexical_cast&)
	{
	}

	return d;
}

template<>
std::string GetEnvVar(const char *str, std::string d)
{
	const char *envPtr = std::getenv(str);
	if (!envPtr) { return d; }
	return std::string{ envPtr };
}

template<>
std::vector<path> GetEnvVar(const char *str, std::vector<path> d)
{
	using namespace boost::algorithm;

	const char *envPtr = std::getenv(str);
	if (!envPtr) { return d; }

	std::vector<path> list;
	split(list, envPtr, is_any_of(";"));
	list.erase(std::remove_if(list.begin(), list.end(), [](const path& path) {
		return path.empty();
	}), list.end());
	return list;
}

} // namespace Detail

template<typename _Ty>
inline void GetEnvVar(const char *str, _Ty& var)
{
	var = Detail::GetEnvVar<_Ty>(str, var);
}

Env::Env()
{
	DefaultSettings();
}

void Env::SetImageName(const std::string& name)
{
	path ref{ name };
	Env::SetImageName(ref);
}

void Env::SetImageName(path& path)
{
	imageFile = path
		.replace_extension(IMAGE_EXTENSION)
		.filename();
}

bool Env::HasImageName() const noexcept
{
	return !imageFile.empty();
}

std::string Env::ImageName() const noexcept
{
	return imageFile.string();
}

// Load specific settings from program environment if they
// are set. The current setting is not changed if a matching
// key could not be located int the environment.
void Env::GatherEnvVars()
{
	GetEnvVar(ENV_STR_PREFIX "DEBUG", debugMode);
	GetEnvVar(ENV_STR_PREFIX "DEBUG_LEVEL", debugLevel);
	GetEnvVar(ENV_STR_PREFIX "SAFE", safeMode);
	GetEnvVar(ENV_STR_PREFIX "INC_PATH", includePaths);
	GetEnvVar(ENV_STR_PREFIX "STD_PATH", standardPaths);
	GetEnvVar(ENV_STR_PREFIX "LIB_PATH", libraryPaths);
}

void Env::DefaultSettings()
{
	// TODO: Platform and hardware specific settings
	//  - Arch, Byte-order, OS, etc..
}

// Load the default settings from the specification file.
void Env::LoadSpecification(Specification& spec)
{
	// Check if specification file was loaded.
	if (!spec.HasProperties()) { return; }
}

const std::vector<path>& Env::GetSettingIncludePaths() const
{
	return includePaths;
}

const std::vector<path>& Env::GetSettingStandardPaths() const
{
	return standardPaths;
}

const std::vector<path>& Env::GetSettingLibraryPaths() const
{
	return libraryPaths;
}

// Intialize environment specific to production requirements
// Defaults in order of processing:
//  - Load defaults
//  - Load settings from specification file, if exist
//  - Load settings from environment, overriding current settings
Env Env::InitBasicEnvironment(Specification& spec)
{
	Env env;
	env.LoadSpecification(spec);
	env.GatherEnvVars();

	return std::move(env);
}

// Intialize environment specific to development requirements
// Defaults in order of processing:
//  - Load defaults
//  - Enable debug mode
Env Env::InitDevelopmentEnvironment(Specification& spec)
{
	CRY_UNUSED(spec);
	Env env;
	env.SetDebug(true);

	return std::move(env);
}

// Intialize environment specific to testing requirements
// Defaults in order of processing:
//  - Load defaults
//  - Enable debug mode
Env Env::InitTestEnvironment(Specification& spec)
{
	CRY_UNUSED(spec);
	Env env;
	env.SetDebug(true);

	return std::move(env);
}
