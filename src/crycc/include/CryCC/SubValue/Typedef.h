// Copyright (c) 2017 Quenza Inc. All rights reserved.
//
// This file is part of the Cryptox project.
//
// Use of this source code is governed by a private license
// that can be found in the LICENSE file. Content can not be 
// copied and/or distributed without the express of the author.

#pragma once

#include <Cry/Cry.h>
#include <Cry/Except.h>

#include <cassert>
#include <array>
#include <string>
#include <memory>
#include <vector>
#include <list>
#include <bitset>

namespace CryCC
{
namespace SubValue
{
namespace Typedef
{

class TypeFacade;
class TypedefBase;

using BaseType = std::shared_ptr<Typedef::TypedefBase>;
using BaseType2 = std::shared_ptr<TypeFacade>;

class TypedefBase
{
public:
	// Envelope helper to identify type.
	enum class TypeVariation : uint8_t
	{
		INVAL = 0,
		BUILTIN = 100,
		RECORD,
		TYPEDEF,
		VARIADIC,
		POINTER,
	};

public:
	// Storage class specifier.
	enum class StorageClassSpecifier
	{
		// TODO: rename to use _T
		NONE,
		AUTO,
		STATIC,
		EXTERN,
		TYPEDEF,
		REGISTER,
	};

	// Type qualifier
	enum class TypeQualifier
	{
		// TODO: rename to use _T
		NONE,
		CONST_T,
		VOLATILE,
	};

	template<typename ContainerType, size_t Size>
	class StaticArray
	{
	public:
		using InternalArray = std::array<ContainerType, Size>;

		StaticArray() = default;
		StaticArray(std::initializer_list<TypeQualifier> list)
		{
			for (const auto& value : list) {
				m_array.fill(value);
			}
		}

		using value_type = typename InternalArray::value_type;
		using reference = typename InternalArray::reference;
		using const_reference = typename InternalArray::const_reference;
		using iterator = typename InternalArray::iterator;
		using const_iterator = typename InternalArray::const_iterator;
		using reverse_iterator = typename InternalArray::reverse_iterator;
		using const_reverse_iterator = typename InternalArray::const_reverse_iterator;

		bool Full() const noexcept { return m_offset >= Size; }
		bool Empty() const noexcept { return !m_offset; }

		iterator begin() { return m_array.begin(); }
		iterator end() { return m_array.end(); }
		const_iterator begin() const noexcept { return m_array.begin(); }
		const_iterator end() const noexcept { return m_array.end(); }
		const_iterator cbegin() const noexcept { return m_array.cbegin(); }
		const_iterator cend() const noexcept { return m_array.cend(); }

		const_reference operator[](size_t idx) const
		{
			return m_array[idx];
		}

		// Push item on the array.
		void PushBack(value_type tq)
		{
			if (!Full()) {
				m_array[m_offset++] = tq;
			}
		}

		// Push item on the array.
		void PushBack(value_type&& tq)
		{
			if (!Full()) {
				m_array[m_offset++] = std::move(tq);
			}
		}

	private:
		InternalArray m_array = InternalArray{};
		size_t m_offset = 0;
	};

public:
	// Abstract methods.
	virtual const std::string TypeName() const = 0;
	virtual bool AllowCoalescence() const = 0;
	virtual void Consolidate(BaseType& type) = 0;
	virtual size_t UnboxedSize() const = 0;
	virtual bool Equals(TypedefBase* other) const = 0;
	virtual std::vector<uint8_t> TypeEnvelope() const;

	// Type specifier inputs.
	inline void SetStorageClass(StorageClassSpecifier storageClass) { m_storageClass = storageClass; }
	inline void SetQualifier(TypeQualifier typeQualifier) { m_typeQualifier.PushBack(typeQualifier); }
	inline void SetInline() { m_isInline = true; }

	// Stringify type name.
	const std::string StorageClassName() const;
	const std::string QualifierName() const;
	
	// Additional type specifiers.
	inline StorageClassSpecifier StorageClass() const noexcept { return m_storageClass; }
	inline StaticArray<TypeQualifier, 2> TypeQualifiers() const noexcept { return m_typeQualifier; }
	inline bool IsInline() const noexcept { return m_isInline; }

protected:
	bool m_isInline = false;
	StorageClassSpecifier m_storageClass = StorageClassSpecifier::NONE;
	StaticArray<TypeQualifier, 2> m_typeQualifier = { TypeQualifier::NONE, TypeQualifier::NONE };
};

constexpr uint8_t SetInteralType(TypedefBase::TypeVariation type)
{
	return static_cast<uint8_t>(type);
}

#define REGISTER_TYPE(t)\
	const uint8_t m_c_internalType = SetInteralType(TypedefBase::TypeVariation::t); \

// Builtin types.
class BuiltinType : public TypedefBase
{
	// Additional type options.
	enum
	{
		IS_SIGNED,
		IS_UNSIGNED,
		IS_SHORT,
		IS_LONG,
		IS_LONG_LONG,
		IS_COMPLEX,
		IS_IMAGINARY,
	};

	REGISTER_TYPE(BUILTIN);
	std::bitset<8> m_typeOptions;

private:
	// If specifier matches a type option, set the option bit
	// and default the type to integer.
	void SpecifierToOptions();

public:
	enum class Specifier
	{
		// TODO: rename to use _T
		VOID_T,
		CHAR,
		SHORT,
		INT,
		LONG,
		SIGNED,
		UNSIGNED,
		FLOAT,
		DOUBLE,
		BOOL,
	};

public:
	BuiltinType(Specifier specifier);

	// Test type options.
	inline auto Unsigned() const { return m_typeOptions.test(IS_UNSIGNED); }
	inline auto Signed() const { return !Unsigned(); }
	inline auto Short() const { return m_typeOptions.test(IS_SHORT); }
	inline auto Long() const { return m_typeOptions.test(IS_LONG); }
	inline auto Complex() const { return m_typeOptions.test(IS_COMPLEX); }
	inline auto Imaginary() const { return m_typeOptions.test(IS_IMAGINARY); }

	// Return type name string.
	const std::string TypeName() const;
	// If any type options are set, allow type coalescence.
	bool AllowCoalescence() const { return m_typeOptions.any(); }
	// Return the type specifier.
	Specifier TypeSpecifier() const { return m_specifier; }

	size_t UnboxedSize() const;

	bool Equals(TypedefBase* other) const;

	std::vector<uint8_t> TypeEnvelope() const override;

	void Consolidate(BaseType& type);

private:
	Specifier m_specifier;
};

// Record types are types that consist of multiple types mapped to a name.
class RecordType : public TypedefBase
{
	REGISTER_TYPE(RECORD);

public:
	enum class Specifier
	{
		STRUCT,
		UNION,
	};

public:
	RecordType(const std::string& name, Specifier specifier = Specifier::STRUCT);
	//RecordType(const std::string& name, Specifier specifier, size_t elements, BaseType type);

	const std::string TypeName() const final
	{
		return (m_specifier == Specifier::UNION ? "union " : "struct ") + m_name;
	}

	void AddField(const std::string& field, const BaseType2& type)
	{
		m_fields.push_back({ field, type });
	}

	inline bool IsAnonymous() const noexcept { return m_name.empty(); }

	inline std::string Name() const noexcept { return m_name; }

	inline size_t FieldSize() const noexcept { return m_fields.size(); }

	inline std::vector<std::pair<std::string, BaseType2>> Fields() const noexcept { return m_fields; }

	bool AllowCoalescence() const final { return false; }
	// Return the record specifier.
	Specifier TypeSpecifier() const { return m_specifier; }

	//TODO: quite the puzzle
	size_t UnboxedSize() const { return 0; }

	bool Equals(TypedefBase* other) const;

	std::vector<uint8_t> TypeEnvelope() const override;

	void Consolidate(BaseType&)
	{
		throw Cry::Except::UnsupportedOperationException{ "RecordType::Consolidate" };
	}

private:
	std::string m_name;
	Specifier m_specifier;
	std::vector<std::pair<std::string, BaseType2>> m_fields;
};

class TypedefType : public TypedefBase
{
	REGISTER_TYPE(TYPEDEF);
	std::string m_name;
	BaseType m_resolveType;

public:
	TypedefType(const std::string& name, BaseType& nativeType);

	const std::string TypeName() const final
	{
		return m_name + ":" + m_resolveType->TypeName();
	}

	bool AllowCoalescence() const final { return false; }

	size_t UnboxedSize() const { return m_resolveType->UnboxedSize(); }

	bool Equals(TypedefBase* other) const;

	std::vector<uint8_t> TypeEnvelope() const override;

	void Consolidate(BaseType&)
	{
		throw Cry::Except::UnsupportedOperationException{ "TypedefType::Consolidate" };
	}
};

class VariadicType : public TypedefBase
{
	REGISTER_TYPE(VARIADIC);

public:
	const std::string TypeName() const final { return "..."; }
	bool AllowCoalescence() const final { return false; }

	size_t UnboxedSize() const
	{
		throw Cry::Except::UnsupportedOperationException{ "VariadicType::UnboxedSize" };
	}

	bool Equals(TypedefBase* other) const
	{
		return dynamic_cast<VariadicType*>(other) != nullptr;
	}

	std::vector<uint8_t> TypeEnvelope() const override;

	void Consolidate(BaseType&)
	{
		throw Cry::Except::UnsupportedOperationException{ "VariadicType::Consolidate" };
	}
};

//TODO: How about no?
class PointerType : public TypedefBase
{
	REGISTER_TYPE(POINTER);

public:
	const std::string TypeName() const final { return "(ptr)"; }
	bool AllowCoalescence() const final { return false; }
	size_t UnboxedSize() const { return sizeof(int); } //TODO: not quite

	bool Equals(TypedefBase* other) const
	{
		return dynamic_cast<PointerType*>(other) != nullptr;
	}

	std::vector<uint8_t> TypeEnvelope() const override;

	void Consolidate(BaseType&)
	{
		throw Cry::Except::UnsupportedOperationException{ "PointerType::Consolidate" };
	}
};

} // namespace Typedef
} // namespace SubValue
} // namespace CryCC