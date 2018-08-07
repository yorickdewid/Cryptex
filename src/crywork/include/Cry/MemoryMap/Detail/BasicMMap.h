// Copyright (c) 2017 Quenza Inc. All rights reserved.
//
// This file is part of the Cryptox project.
//
// Use of this source code is governed by a private license
// that can be found in the LICENSE file. Content can not be 
// copied and/or distributed without the express of the author.

#pragma once

#include <Cry/Cry.h>

#include <iterator>
#include <string>
#include <system_error>

namespace Cry
{
namespace MemoryMap
{
namespace Detail
{

enum { map_entire_file = 0 };

#ifdef _WIN32
using file_handle_type = HANDLE;
#else
using file_handle_type = int;
#endif

template<typename ByteT>
struct basic_mmap
{
	using value_type = ByteT;
	using size_type = int64_t;
	using reference = value_type & ;
	using const_reference = const value_type&;
	using pointer = value_type * ;
	using const_pointer = const value_type*;
	using difference_type = std::ptrdiff_t;
	using iterator = pointer;
	using const_iterator = const_pointer;
	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;
	using iterator_category = std::random_access_iterator_tag;
	using handle_type = file_handle_type;

	static_assert(sizeof(ByteT) == sizeof(char), "ByteT must be the same size as char.");

private:
	// Points to the first requested byte, and not to the actual start of the mapping.
	pointer data_ = nullptr;

	// Length, in bytes, requested by user, which may not be the length of the full
	// mapping, and the entire length of the full mapping.
	size_type length_ = 0;
	size_type mapped_length_ = 0;

	// Letting user map a file using both an existing file handle and a path introcudes
	// On POSIX, we only need a file handle to create a mapping, while on Windows
	// systems the file handle is necessary to retrieve a file mapping handle, but any
	// subsequent operations on the mapped region must be done through the latter.
	handle_type file_handle_ = INVALID_HANDLE_VALUE;
#ifdef _WIN32
	handle_type file_mapping_handle_ = INVALID_HANDLE_VALUE;
#endif

	// some complexity in that we must not close the file handle if user provided it,
	// but we must close it if we obtained it using the provided path. For this reason,
	// this flag is used to determine when to close file_handle_.
	bool is_handle_internal_;

public:
	basic_mmap() = default;
	basic_mmap(const basic_mmap&) = delete;
	basic_mmap& operator=(const basic_mmap&) = delete;
	basic_mmap(basic_mmap&&);
	basic_mmap& operator=(basic_mmap&&);
	~basic_mmap();

	handle_type file_handle() const noexcept { return file_handle_; }
	handle_type mapping_handle() const noexcept;

	bool is_open() const noexcept { return file_handle_ != INVALID_HANDLE_VALUE; }
	bool is_mapped() const noexcept;
	bool empty() const noexcept { return length() == 0; }

	size_type offset() const noexcept { return mapped_length_ - length_; }
	size_type length() const noexcept { return length_; }
	size_type mapped_length() const noexcept { return mapped_length_; }

	pointer data() noexcept { return data_; }
	const_pointer data() const noexcept { return data_; }

	iterator begin() noexcept { return data(); }
	const_iterator begin() const noexcept { return data(); }
	const_iterator cbegin() const noexcept { return data(); }

	iterator end() noexcept { return begin() + length(); }
	const_iterator end() const noexcept { return begin() + length(); }
	const_iterator cend() const noexcept { return begin() + length(); }

	reverse_iterator rbegin() { return reverse_iterator(end()); }
	const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
	const_reverse_iterator crbegin() const { return const_reverse_iterator(end()); }

	reverse_iterator rend() { return reverse_iterator(begin()); }
	const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }
	const_reverse_iterator crend() const { return const_reverse_iterator(begin()); }

	reference operator[](const size_type i) noexcept { return data_[i]; }
	const_reference operator[](const size_type i) const noexcept { return data_[i]; }

	template<typename String>
	void map(String& path, size_type offset, size_type length, access_mode mode, std::error_code& error);
	void map(handle_type handle, size_type offset, size_type length, access_mode mode, std::error_code& error);
	void unmap();
	void sync(std::error_code& error);

	void swap(basic_mmap& other);

private:
	pointer get_mapping_start() noexcept { return !data() ? nullptr : data() - offset(); }
};

template<typename ByteT>
bool operator==(const basic_mmap<ByteT>& a, const basic_mmap<ByteT>& b);
template<typename ByteT>
bool operator!=(const basic_mmap<ByteT>& a, const basic_mmap<ByteT>& b);
template<typename ByteT>
bool operator<(const basic_mmap<ByteT>& a, const basic_mmap<ByteT>& b);
template<typename ByteT>
bool operator<=(const basic_mmap<ByteT>& a, const basic_mmap<ByteT>& b);
template<typename ByteT>
bool operator>(const basic_mmap<ByteT>& a, const basic_mmap<ByteT>& b);
template<typename ByteT>
bool operator>=(const basic_mmap<ByteT>& a, const basic_mmap<ByteT>& b);

} // namespace Detail
} // namespace MemoryMap
} // namespace Cry