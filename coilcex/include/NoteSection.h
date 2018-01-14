// Copyright (c) 2017 Quenza Inc. All rights reserved.
//
// This file is part of the Cryptox project.
//
// Use of this source code is governed by a private license
// that can be found in the LICENSE file. Content can not be 
// copied and/or distributed without the express of the author.

#pragma once

#include "Section.h"

namespace CryExe
{

class COILCEXAPI NoteSection : public Section
{
	const std::string m_name;
	const std::string m_description;

	std::string m_context;
	Section::SectionType m_sectionLink;

public:
	NoteSection()
		: Section{ Section::SectionType::NOTE }
	{
	}

	NoteSection(const std::string& name, const std::string& description)
		: Section{ Section::SectionType::NOTE }
		, m_name{ name }
		, m_description{ description }
	{
	}

	/*NoteSection()
		: Section{ Section{ Section::SectionType::NOTE } }
	{
	}*/

	inline const ByteArray& Data() const
	{
		return Section::Data();
	}

	// Request meta info for note section
	inline std::string Name() const { return m_name; }
	inline std::string Description() const { return m_description; }

	// Set note context directly
	void SetContext(const std::string& context) { m_context = context; }
	void SetContext(std::string&& context) { m_context = std::move(context); }

	// Append operators
	void operator<<(const std::string& str) { m_context.append(str); }
	void operator+=(const std::string& str) { m_context.append(str); }
};

} // namespace CryExe
