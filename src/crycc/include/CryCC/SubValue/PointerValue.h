// Copyright (c) 2017 Quenza Inc. All rights reserved.
//
// This file is part of the Cryptox project.
//
// Use of this source code is governed by a private license
// that can be found in the LICENSE file. Content can not be 
// copied and/or distributed without the express of the author.

#pragma once

#include <CryCC/SubValue/ValueContract.h>
#include <CryCC/SubValue/Typedef.h>

#include <Cry/Cry.h>
#include <Cry/TypeTrait.h>
#include <Cry/Serialize.h>

namespace CryCC
{
namespace SubValue
{
namespace Valuedef
{

// A pointer value points to a memory chunk containing data.
class PointerValue final : public AbstractValue<PointerValue>
{
public:
    using typdef_type = nullptr_t;
    using value_category = ValueCategory::Singular;

    // Expose the value variants that this category can process.
    constexpr static const int value_variant_order = 0;
    // Unique value identifier.
    constexpr static const int value_category_identifier = 12;

	//
	// Implement value category contract.
	//

	// NOTE: PointerValue holds no data, thus serialization can be ignored.
	static void Serialize(const PointerValue&, buffer_type&) {}
	// NOTE: PointerValue holds no data, thus deserialization can be ignored.
	static void Deserialize(PointerValue&, buffer_type&) {}

    // All the nil values are the same.
	bool operator==(const PointerValue&) const { return true; }

    // Convert current value to string.
	std::string ToString() const { return "(ptr)"; }
};

} // namespace Valuedef
} // namespace SubValue
} // namespace CryCC
