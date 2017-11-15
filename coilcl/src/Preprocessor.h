#pragma once

#include "Profile.h"
#include "Stage.h"

#include <functional>
#include <unordered_map>

namespace CoilCl
{

class Preprocessor : public Stage<Preprocessor>
{
public:
	enum Option
	{
		PARSE_INCLUDE = 0x1,
		PARSE_DEFINE = 0x2,
		PARSE_MACRO = 0x4,
		PARSE_PRAGMA = 0x8,
		PARSE_ALL = 0xff,
	};

public:
	Preprocessor(std::shared_ptr<Compiler::Profile>&);

	Preprocessor& Options(int optionSet)
	{
		m_bitset = optionSet;
		return (*this);
	}

	Preprocessor& Transform();
	void ImportSource(std::string);
	void Definition(std::string);
	void DefinitionUntag(std::string);
	void ConditionalStatement();
	void ProcessStatement(const std::string& str);
	bool SkipWhitespace(char c);

private:
	int m_bitset;
	std::string pipe;
	std::unordered_map<std::string, std::function<void(std::string)>> m_keywords;
	std::shared_ptr<Compiler::Profile> m_profile;
};

} // namespace CoilCl