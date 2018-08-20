// Copyright (c) 2017 Quenza Inc. All rights reserved.
//
// This file is part of the Cryptox project.
//
// Use of this source code is governed by a private license
// that can be found in the LICENSE file. Content can not be 
// copied and/or distributed without the express of the author.

#pragma once

#include <CryCC/SubValue/Typedef.h>

#include <Cry/Cry.h>
#include <Cry/Except.h>

namespace CryCC
{
namespace SubValue
{
namespace Typedef
{

class TypeFacade;
class TypedefBase;

// ...
class ArrayType : public TypedefBase
{
	REGISTER_TYPE(ARRAY);

public:
	ArrayType(size_t elements, BaseType arrayType);
	ArrayType(size_t elements, BaseType&& arrayType);

	// Return the size of the array.
	inline size_t Order() const noexcept { return m_elements; }

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

private:
	size_t m_elements;
	BaseType m_elementType;
};

} // namespace Typedef
} // namespace SubValue
} // namespace CryCC
