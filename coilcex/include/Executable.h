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

#include <string>

namespace CryExe
{

class COILCEXAPI InvalidCexFormat
{
};

class COILCEXAPI Executable : public Image
{
	void *m_interalImageStructure = nullptr;

public:
	Executable(const std::string& path, FileMode fm = FileMode::FM_OPEN);
	~Executable();

	// Check if the image is sealed and thus readonly
	bool IsSealed() const;

	void Open(FileMode mode) override;
	void Close() override;

	// Add new directory to CEX image
	void AddDirectory();

	// Add new section to CEX image
	void AddSection(Section *);

	// Seal the executable in order to generate a valid CEX image. The sealing
	// process guantees a valid CEX is generated and the object cannot be 
	// altered when commited to disk.
	static const Executable& Seal(Executable&);

private:
	void ValidateImageFormat();
	void CreateNewImage();
	void CalculateInternalOffsets();
	void CalculateImageSize();
};

} // namespace CryExecutable
