// Copyright (c) 2017 Quenza Inc. All rights reserved.
//
// This file is part of the Cryptox project.
//
// Use of this source code is governed by a private license
// that can be found in the LICENSE file. Content can not be 
// copied and/or distributed without the express of the author.

#pragma once

#include "Exportable.h"
#include "Image.h"
#include "Section.h"
#include "Directory.h"

#include <string>
#include <deque>
#include <bitset>
#include <vector>
#include <algorithm>
#include <memory>
#include <array>

namespace CryExe
{

template<typename _Ty>
class OSFilePositionImpl;

using OSFilePosition = OSFilePositionImpl<>;

namespace Structure
{

struct CexFileFormat;

} // namespace Structure

class COILCEXAPI InvalidCexFormat
{
};

enum class COILCEXAPI InternalImageVersion
{
	IMAGE_STRUCT_FORMAT_INVAL = 0,
	IMAGE_STRUCT_FORMAT_03 = 3,
	IMAGE_STRUCT_FOMART_LAST = IMAGE_STRUCT_FORMAT_03,
};

enum class COILCEXAPI ExecType
{
	TYPE_EXECUTABLE,
	TYPE_LIBRARY_DYNAMIC,
};

using SectionList = std::vector<CryExe::Section>;
using DirectoryList = std::vector<CryExe::Directory>;

class COILCEXAPI Executable : public Image
{
	friend struct Meta;

	InternalImageVersion m_interalImageVersion = InternalImageVersion::IMAGE_STRUCT_FORMAT_INVAL;
	std::unique_ptr<Structure::CexFileFormat> m_interalImageStructure;
	std::unique_ptr<std::array<std::deque<OSFilePosition>, 2>> m_offsetStack;
	std::bitset<UINT16_MAX> m_allocSections = 0;
	SectionList m_foundSectionList;
	DirectoryList m_foundDirectoryList;
	int m_execType;

public:
	enum class COILCEXAPI Option
	{
		OPT_BINREP = 1 << 1,
		OPT_READONLY = 1 << 2,
	};

	friend Option operator|(Option opt1, Option opt2)
	{
		return static_cast<Option>(static_cast<int>(opt1) | static_cast<int>(opt2));
	}

	friend int operator&(Option opt1, Option opt2)
	{
		return static_cast<int>(opt1) & static_cast<int>(opt2);
	}

public:
	Executable(const std::string& path, FileMode fm = FileMode::FM_OPEN, ExecType type = ExecType::TYPE_EXECUTABLE);
	explicit Executable(Executable& exe, FileMode fm);
	
	Executable(const Executable&) = default;
	Executable(Executable&&);
	
	~Executable();

	// Check if the image is sealed and thus readonly
	bool IsSealed() const;

	// Open executable with file mode
	void Open(FileMode);

	// Close image handler
	void Close();

	// Add new directory to CEX image
	void AddDirectory(Directory *);

	// Add new section to CEX image
	void AddSection(Section *);
	void GetSection(Section *);

	inline const SectionList& Sections() const { return m_foundSectionList; }

	// Return iterator on requested sections, if any
	SectionList::iterator FindSection(CryExe::Section::SectionType type);

	// Fill structure
	void GetSectionDataFromImage(Section&);

	// Get image version as integer
	short ImageVersion() const;

	// Set executable flags
	void SetOption(Option);

	// Check image executable type
	bool IsExecutable() const;
	bool IsDynamicLibrary() const;

	// Seal the executable in order to generate a valid CEX image. The sealing
	// process guantees a valid CEX is generated and the object cannot be 
	// altered when commited to disk.
	static const Executable& Seal(Executable&);

private:
	void ConveySectionsFromDisk();
	void ConveyDirectoriesFromDisk();
	void ValidateImageFormat();
	void CreateNewImage();
	void AlignBounds();
	void CalculateInternalOffsets();
	void CalculateImageSize();
	void CalculateSectionOffsets();
	void CalculateDirectoryOffsets();
};

class COILCEXAPI DynamicLibrary : public Executable
{
public:
	template<typename... _TyArgs>
	DynamicLibrary(_TyArgs&&... args)
		: Executable{ std::forward<_TyArgs>(args)..., ExecType::TYPE_LIBRARY_DYNAMIC }
	{
	}
};

} // namespace CryExe
