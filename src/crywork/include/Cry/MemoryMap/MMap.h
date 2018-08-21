// Copyright (c) 2017 Quenza Inc. All rights reserved.
//
// This file is part of the Cryptox project.
//
// Use of this source code is governed by a private license
// that can be found in the LICENSE file. Content can not be 
// copied and/or distributed without the express of the author.

#pragma once

#include <Cry/MemoryMap/Detail/BasicMMap.h>
#include <Cry/MemoryMap/Page.h>

#include <system_error>

namespace Cry
{
namespace MemoryMap
{

// This value may be provided as the 'length' parameter to the constructor or
// 'map', in which case a memory mapping of the entire file is created.
//using Detail::map_entire_file;

template<AccessModeType AccessMode, typename ByteType>
class BasicMMap
{
	using self_type = Detail::BasicMMap<ByteType>;
	self_type impl_;

public:
	using value_type = typename self_type::value_type;
	using size_type = typename self_type::size_type;
	using reference = typename self_type::reference;
	using const_reference = typename self_type::const_reference;
	using pointer = typename self_type::pointer;
	using const_pointer = typename self_type::const_pointer;
	using difference_type = typename self_type::difference_type;
	using iterator = typename self_type::iterator;
	using const_iterator = typename self_type::const_iterator;
	using reverse_iterator = typename self_type::reverse_iterator;
	using const_reverse_iterator = typename self_type::const_reverse_iterator;
	using iterator_category = typename self_type::iterator_category;
	using handle_type = typename self_type::handle_type;

	// The default constructed mmap object is in a non-mapped state, that is, any
	// operation that attempts to access nonexistent underlying data will result in
	// undefined behaviour/segmentation faults.
	BasicMMap() = default;

	// The same as invoking the 'map' function, except any error that may occur while
	// establishing the mapping is thrown.
	template<typename String>
	BasicMMap(const String& path, const size_type offset, const size_type length)
	{
		std::error_code error;
		map(path, offset, length, error);
		if (error) { throw error; }
	}

	// The same as invoking the 'map' function, except any error that may occur while
	// establishing the mapping is thrown.
	BasicMMap(const handle_type handle, const size_type offset, const size_type length)
	{
		std::error_code error;
		map(handle, offset, length, error);
		if (error) { throw error; }
	}

	// This class has single-ownership semantics, so transferring ownership may only be
	// accomplished by moving the object.
	BasicMMap(BasicMMap&&) = default;
	BasicMMap& operator=(BasicMMap&&) = default;

	// The destructor invokes unmap.
	~BasicMMap() = default;

	// On UNIX systems 'file_handle' and 'mapping_handle' are the same. On Windows,
	// however, a mapped region of a file gets its own handle, which is returned by
	// 'mapping_handle'.
	handle_type file_handle() const noexcept { return impl_.file_handle(); }
	handle_type mapping_handle() const noexcept { return impl_.mapping_handle(); }

	// Returns whether a valid memory mapping has been created.
	bool is_open() const noexcept { return impl_.is_open(); }

	// Returns true if no mapping was established, that is, conceptually the
	// same as though the length that was mapped was 0. This function is
	// provided so that this class has Container semantics.
	bool empty() const noexcept { return impl_.empty(); }

	// 'size' and 'length' both return the logical length, i.e. the number of bytes
	// user requested to be mapped, while 'mapped_length' returns the actual number of
	// bytes that were mapped which is a multiple of the underlying operating system's
	// page allocation granularity.
	size_type size() const noexcept { return impl_.length(); }
	size_type length() const noexcept { return impl_.length(); }
	size_type mapped_length() const noexcept { return impl_.mapped_length(); }

	// Returns the offset, relative to the file's start, at which the mapping was
	// requested to be created.
	size_type offset() const noexcept { return impl_.offset(); }

	// Returns a pointer to the first requested byte, or 'nullptr' if no memory mapping exists.
	template<AccessModeType A = AccessMode, typename = typename std::enable_if<A == AccessModeType::WRITE>::type>
	pointer data() noexcept { return impl_.data(); }
	const_pointer data() const noexcept { return impl_.data(); }

	// Returns an iterator to the first requested byte, if a valid memory mapping
	// exists, otherwise this function call is undefined behaviour.
	template<AccessModeType A = AccessMode, typename = typename std::enable_if<A == AccessModeType::WRITE>::type>
	iterator begin() noexcept { return impl_.begin(); }
	const_iterator begin() const noexcept { return impl_.begin(); }
	const_iterator cbegin() const noexcept { return impl_.cbegin(); }

	// Returns an iterator one past the last requested byte, if a valid memory mapping
	// exists, otherwise this function call is undefined behaviour.
	template<AccessModeType A = AccessMode, typename = typename std::enable_if<A == AccessModeType::WRITE>::type>
	iterator end() noexcept { return impl_.end(); }
	const_iterator end() const noexcept { return impl_.end(); }
	const_iterator cend() const noexcept { return impl_.cend(); }

	// Returns a reverse iterator to the last memory mapped byte, if a valid
	// memory mapping exists, otherwise this function call is undefined
	// behaviour.
	template<AccessModeType A = AccessMode, typename = typename std::enable_if<A == AccessModeType::WRITE>::type>
	reverse_iterator rbegin() noexcept { return impl_.rbegin(); }
	const_reverse_iterator rbegin() const noexcept { return impl_.rbegin(); }
	const_reverse_iterator crbegin() const noexcept { return impl_.crbegin(); }

	// Returns a reverse iterator past the first mapped byte, if a valid memory
	// mapping exists, otherwise this function call is undefined behaviour.
	template<AccessModeType A = AccessMode, typename = typename std::enable_if<A == AccessModeType::WRITE>::type>
	reverse_iterator rend() noexcept { return impl_.rend(); }
	const_reverse_iterator rend() const noexcept { return impl_.rend(); }
	const_reverse_iterator crend() const noexcept { return impl_.crend(); }

	// Returns a reference to the 'i'th byte from the first requested byte (as returned
	// by 'data'). If this is invoked when no valid memory mapping has been created
	// prior to this call, undefined behaviour ensues.
	reference operator[](const size_type i) noexcept { return impl_[i]; }
	const_reference operator[](const size_type i) const noexcept { return impl_[i]; }

	// Establishes a memory mapping with AccessMode. If the mapping is unsuccesful, the
	// reason is reported via 'error' and the object remains in a state as if this
	// function hadn't been called.
	//
	// 'path', which must be a path to an existing file, is used to retrieve a file
	// handle (which is closed when the object destructs or 'unmap' is called), which is
	// then used to memory map the requested region. Upon failure, 'error' is set to
	// indicate the reason and the object remains in an unmapped state.
	//
	// 'offset' is the number of bytes, relative to the start of the file, where the
	// mapping should begin. When specifying it, there is no need to worry about
	// providing a value that is aligned with the operating system's page allocation
	// granularity. This is adjusted by the implementation such that the first requested
	// byte (as returned by 'data' or 'begin'), so long as 'offset' is valid, will be at
	// 'offset' from the start of the file.
	//
	// 'length' is the number of bytes to map. It may be 'map_entire_file', in which
	// case a mapping of the entire file is created.
	template<typename String>
	void map(const String& path, const size_type offset, const size_type length, std::error_code& error)
	{
		impl_.map(path, offset, length, AccessMode, error);
	}

	// Establishes a memory mapping with AccessMode. If the mapping is unsuccesful, the
	// reason is reported via 'error' and the object remains in a state as if this
	// function hadn't been called.
	//
	// 'handle', which must be a valid file handle, which is used to memory map the
	// requested region. Upon failure, 'error' is set to indicate the reason and the
	// object remains in an unmapped state.
	//
	// 'offset' is the number of bytes, relative to the start of the file, where the
	// mapping should begin. When specifying it, there is no need to worry about
	// providing a value that is aligned with the operating system's page allocation
	// granularity. This is adjusted by the implementation such that the first requested
	// byte (as returned by 'data' or 'begin'), so long as 'offset' is valid, will be at
	// 'offset' from the start of the file.
	//
	// 'length' is the number of bytes to map. It may be 'map_entire_file', in which
	// case a mapping of the entire file is created.
	void map(const handle_type handle, const size_type offset, const size_type length, std::error_code& error)
	{
		impl_.map(handle, offset, length, AccessMode, error);
	}

	// If a valid memory mapping has been created prior to this call, this call
	// instructs the kernel to unmap the memory region and disassociate this object
	// from the file.
	//
	// The file handle associated with the file that is mapped is only closed if the
	// mapping was created using a file path. If, on the other hand, an existing
	// file handle was used to create the mapping, the file handle is not closed.
	void unmap() { impl_.unmap(); }

	void swap(BasicMMap& other) { impl_.swap(other.impl_); }

	// Flushes the memory mapped page to disk. Errors are reported via 'error'.
	template<AccessModeType A = AccessMode, typename = typename std::enable_if<A == AccessModeType::WRITE>::type>
	void sync(std::error_code& error) { impl_.sync(error); }

	//
	// All operators compare the address of the first byte and size of the two mapped regions.
	//

	friend bool operator==(const BasicMMap& lhs, const BasicMMap& rhs)
	{
		return lhs.impl_ == rhs.impl_;
	}

	friend bool operator!=(const BasicMMap& lhs, const BasicMMap& rhs)
	{
		return !(lhs == rhs);
	}

	friend bool operator<(const BasicMMap& lhs, const BasicMMap& rhs)
	{
		return lhs.impl_ < rhs.impl_;
	}

	friend bool operator<=(const BasicMMap& lhs, const BasicMMap& rhs)
	{
		return lhs.impl_ <= rhs.impl_;
	}

	friend bool operator>(const BasicMMap& lhs, const BasicMMap& rhs)
	{
		return lhs.impl_ > rhs.impl_;
	}

	friend bool operator>=(const BasicMMap& lhs, const BasicMMap& rhs)
	{
		return lhs.impl_ >= rhs.impl_;
	}
};

// This is the basis for all read-only mmap objects and should be preferred over
// directly using BasicMMap.
template<typename ByteType>
using basic_mmap_source = BasicMMap<AccessModeType::READ, ByteType>;

// This is the basis for all read-write mmap objects and should be preferred over
// directly using BasicMMap.
template<typename ByteType>
using basic_mmap_sink = BasicMMap<AccessModeType::WRITE, ByteType>;

// These aliases cover the most common use cases, both representing a raw byte stream
// (either with a char or an unsigned char/uint8_t).
using mmap_source = basic_mmap_source<char>;
using ummap_source = basic_mmap_source<unsigned char>;

using mmap_sink = basic_mmap_sink<char>;
using ummap_sink = basic_mmap_sink<unsigned char>;

// Convenience factory method that constructs a mapping for any BasicMMap<> type.
template<typename MMap, typename MappingToken>
MMap MakeMMap(const MappingToken& token, int64_t offset, int64_t length, std::error_code& error)
{
	MMap mmap;
	mmap.map(token, offset, length, error);
	return mmap;
}

// Convenience factory method.
//
// MappingToken may be a String ('std::string', 'std::string_view', 'const char*',
// 'std::filesystem::path', 'std::vector<char>', or similar), or a mmap_source::file_handle.
template<typename MappingToken>
mmap_source MakeMMapSource(const MappingToken& token, mmap_source::size_type offset, mmap_source::size_type length, std::error_code& error)
{
	return MakeMMap<mmap_source>(token, offset, length, error);
}

// Convenience factory method.
//
// MappingToken may be a String ('std::string', 'std::string_view', 'const char*',
// 'std::filesystem::path', 'std::vector<char>', or similar), or a mmap_sink::file_handle.
template<typename MappingToken>
mmap_sink MakeMMapSink(const MappingToken& token, mmap_sink::size_type offset, mmap_sink::size_type length, std::error_code& error)
{
	return MakeMMap<mmap_sink>(token, offset, length, error);
}

} // namespace Cry
} // namespace MemoryMap
