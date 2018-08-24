// Copyright (c) 2017 Quenza Inc. All rights reserved.
//
// This file is part of the Cryptox project.
//
// Use of this source code is governed by a private license
// that can be found in the LICENSE file. Content can not be 
// copied and/or distributed without the express of the author.

#pragma once

#include <CryCC/SubValue/Typedef.h>

namespace CryCC
{
namespace SubValue
{
namespace Typedef
{

// The Nil type is a type to test the completeness fo type system.
// It has limited use in most languages and should be used with
// causion. The Nil type is not the allocated type for uninitialized
// values.
class NilType : public TypedefBase
{
	REGISTER_TYPE(NIL);

public:
	//
	// Implement abstract base type methods.
	//

	// Return type identifier.
	int TypeId() const { return TypeIdentifier(); }
	// Return type name string.
	const std::string TypeName() const final;
	// Return native size.
	size_type UnboxedSize() const;
	// Test if types are equal.
	bool Equals(BasePointer) const;
	// Pack the type into a byte stream.
	buffer_type TypeEnvelope() const override;
};

} // namespace Typedef
} // namespace SubValue
} // namespace CryCC