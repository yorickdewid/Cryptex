// Copyright (c) 2017 Quenza Inc. All rights reserved.
//
// This file is part of the Cryptox project.
//
// Use of this source code is governed by a private license
// that can be found in the LICENSE file. Content can not be 
// copied and/or distributed without the express of the author.

#include <CryCC/Program/Stage.h>

namespace CryCC
{
namespace Program
{

StageType::Type g_compilerStage;

const char *StageType::Print(Type name) noexcept
{
	switch (name) {
	case Frontend:
		return "Frontend";
	case TokenProcessor:
		return "TokenProcessor";
	case LexicalAnalysis:
		return "LexicalAnalysis";
	case SyntacticAnalysis:
		return "SyntacticAnalysis";
	case SemanticAnalysis:
		return "SemanticAnalysis";
	case Optimizer:
		return "Optimizer";
	case Emitter:
		return "Emitter";
	}

	return "<unknown>";
}

} // namespace Program
} // namespace CryCC
