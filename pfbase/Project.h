// Copyright (c) 2017 Quenza Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef PROJECT_H_
#define PROJECT_H_

#include "pfbase.h"

#include "Store.h"
#include "File.h"
#include "ObjectStore.h"

#include <string>
#include <memory>
#include <map>

namespace ProjectBase
{

class Project
{
public:
	Project() = default;

	Project(const std::string& name)
		: m_name{ name }
	{
		StartProject();
	}

	Project(const std::string& name, const MetaData& meta)
		: m_name{ name }
		, m_metaPtr{ std::make_unique<MetaData>(meta) }
	{
		StartProject();
	}

	~Project()
	{
		Close();
	}

	// Write all changes to disk
	void Save()
	{
		CommitToDisk();
	}

	// Close project and save all changes
	void Close()
	{
		CommitToDisk();
	}

	// Project file name
	inline std::string Name() const
	{
		return m_name;
	}

	// Project name
	inline std::string ProjectName() const
	{
		if (!m_metaPtr) {
			throw std::runtime_error{ "Metadata not provided" };
		}

		return m_metaPtr->ProjectName();
	}

	// Project author
	inline std::string Author() const
	{
		if (!m_metaPtr) {
			throw std::runtime_error{ "Metadata not provided" };
		}

		return m_metaPtr->Author();
	}

	inline time_t CreateTimestamp() const
	{
		return created;
	}

	inline time_t UpdateTimestamp() const
	{
		return updated;
	}

	size_t StoreCount() const
	{
		return m_objectStores.size();
	}

	std::list<std::string> Stores()
	{
		std::list<std::string> nameList;
		std::for_each(m_objectStores.begin(), m_objectStores.end(), [&nameList] (auto& map) {
			nameList.push_back(map.first);
		});

		return nameList;
	}

	template <typename T>
	void AddStore(const std::string name)
	{
		m_objectStores.insert(std::make_pair(name, std::make_shared<T>()));
	}

	template <typename T>
	std::shared_ptr<T> GetStore(const std::string& name)
	{
		return std::dynamic_pointer_cast<T>(m_objectStores[name]);
	}

	// Load project directly from file. This wil return a pointer to the
	// project object.
	static std::unique_ptr<Project> LoadFile(const std::string& fileName)
	{
		return std::move(std::make_unique<Project>(fileName));
	}

private:
	void StartProject()
	{
		// Check if the given filename reflects a existing file. If so
		// read contents. When the file does not exist, create
		// a new store and flush to disk.
		if (StoreFileExist()) {
			ReadFromDisk();
		}
		else {
			CommitToDisk();
		}
	}

private:
	PFBASEAPI bool StoreFileExist();
	PFBASEAPI void CommitToDisk();
	PFBASEAPI void ReadFromDisk();

private:
	time_t created = 0;
	time_t updated = 0;
	std::string m_name;
	std::map<std::string, std::shared_ptr<Store>> m_objectStores;
	std::unique_ptr<MetaData> m_metaPtr = nullptr;

};

}

#endif  // PROJECT_H_
