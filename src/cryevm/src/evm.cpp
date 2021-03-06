// Copyright (c) 2017 Quenza Inc. All rights reserved.
//
// This file is part of the Cryptox project.
//
// Use of this source code is governed by a private license
// that can be found in the LICENSE file. Content can not be 
// copied and/or distributed without the express of the author.

// Local includes.
#include <CryEVM/evm.h> //TODO: move down
#include "Functional.h"
#include <CryEVM/ExternalMethod.h>  //TODO: move down
#include <CryEVM/RuntimeInterface.h>  //TODO: move down
#include "State.h"
#include "Planner.h"
#include "UniquePreservePtr.h"

// Project includes.
#include <CryCC/Program.h>

// Framework includes.
#include <Cry/Cry.h>
#include <Cry/Config.h>
#include <Cry/Loader.h>

#ifdef AUTO_CONVERT
#include <boost/lexical_cast.hpp>
#endif

#include <memory>

//#define AUTO_CONVERT 1

//TODO:
// - Involve planner and determine strategy
//    - Check if local arch compile is required
// - Run program in runner resulting from planner
//    - Either Interpreter
//    - Or Virtual machine
//    - Or native

namespace Loader = Cry::Module;

using ProgramPtr = Detail::UniquePreservePtr<CryCC::Program::Program>;

#ifdef AUTO_CONVERT
bool has_digits(const std::string& s)
{
	return s.find_first_not_of("0123456789") == std::string::npos;
}
#endif

void ConvertToArgumentList(ArgumentList& list, const datalist_t pointerList)
{
	size_t sz = 0;
	if (!pointerList) { return; }

	do {
		// Reconstruct parameters.
		datachunk_t *arg = pointerList[sz++];
		if (!arg) { return; }
		auto str = std::string{ arg->ptr, arg->size };

#ifdef AUTO_CONVERT
		// Cast to builtin type.
		if (has_digits(str)) {
			list.emplace_back(boost::lexical_cast<int>(str));
		}
		else {
			list.push_back(std::move(str));
		}
#else
		list.push_back(std::move(str));
#endif
		// Free datachunk if required.
		datachunk_internal_release(arg);
	} while (pointerList[sz]);
}

class Configuration
{

public:
	Configuration(struct vm_config *config)
		: m_config{ config }
	{
		Configuration::Assert(config);
	}

	// Test the combination of configuration items.
	static void Assert(const struct vm_config *config)
	{
		CRY_UNUSED(config);
	}

private:
	struct vm_config *m_config;
};

// [ API ENTRY ]
// Program executor.
EVMAPI int ExecuteProgram(runtime_settings_t *runtime) noexcept
{
	using namespace EVM;

	assert(runtime);

	CHECK_API_VERSION(runtime, EVMAPIVER);

	assert(runtime->error_handler);
	assert(runtime->program.program_ptr);

	// Create a configuration object.
	Configuration config{ &runtime->cfg };

	// Capture program pointer and cast into program structure.
	ProgramPtr program = ProgramPtr{ runtime->program.program_ptr };

	// Load modules to import external functionality.
	auto runtimeModules = Loader::Load<RuntimeInterface>(DIST_BINARY_DIR);
	Loader::ForEachLoad(runtimeModules);

	// Collect all external symbols. Load the local symbols first, and give
	// the external symbols loaded from modules the chance to override functions.
	std::list<EVM::ExternalMethod> list;
	list.merge(EVM::SymbolIndex(), [](auto, auto) { return false; });
	for (auto& module : runtimeModules) {
		module->LoadSymbolIndex(list);
	}

	// Set the execution options.
	GlobalExecutionState::Set(list);
	//GlobalExecutionState::Set(config);

	// Determine strategy for program.
	auto runner = Planner{ std::move(program), Planner::Plan::ALL }.DetermineStrategy();
	if (!runner->IsRunnable()) {
		return RETURN_NOT_RUNNABLE;
	}

	try {
		ArgumentList args;
		ConvertToArgumentList(args, runtime->args);

		ArgumentList envs;
		ConvertToArgumentList(envs, runtime->envs);

		// Execute the program in the designated strategy.
		runtime->return_code = runner->Execute(runner->EntryPoint(runtime->entry_point), args, envs);
	}
	// Catch any runtime errors.
	catch (const std::exception& e) {
		runtime->error_handler(runtime->user_data, e.what(), true);
	}

	// Unset the execution options.
	GlobalExecutionState::UnsetAll();
	Loader::ForEachUnload(runtimeModules);

	return RETURN_OK;
}

// [ API ENTRY ]
// Get library information.
EVMAPI void GetLibraryInfo(library_info_t *info) NOTHROW
{
	assert(info);

	info->version_number.major = PRODUCT_VERSION_MAJOR;
	info->version_number.minor = PRODUCT_VERSION_MINOR;
	info->version_number.patch = PRODUCT_VERSION_PATCH;
	info->version_number.local = PRODUCT_VERSION_LOCAL;
	info->product = PROGRAM_NAME;
	info->api_version = EVMAPIVER;
	info->description = PROGRAM_DESCRIPTION;
}
