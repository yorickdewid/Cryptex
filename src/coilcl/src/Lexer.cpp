// Copyright (c) 2017 Quenza Inc. All rights reserved.
//
// This file is part of the Cryptox project.
//
// Use of this source code is governed by a private license
// that can be found in the LICENSE file. Content can not be 
// copied and/or distributed without the express of the author.

#include "Lexer.h"

#include <boost/lexical_cast.hpp>

#include <cctype>
#include <string>
#include <iostream>

constexpr char EndOfUnit = '\0';

void Lexer::Error(const std::string& errormsg)
{
	if (errHandlerFunc) {
		errHandlerFunc(errormsg, m_currentChar, m_lastTokenLine, m_currentColumn);
	}

	m_isEof = true;
	m_prevToken = m_currentToken;
	m_currentChar = EndOfUnit;
}

void Lexer::InitKeywords()
{
	AddKeyword("auto", TK_AUTO);
	AddKeyword("_Bool", TK_BOOL);
	AddKeyword("break", TK_BREAK);
	AddKeyword("case", TK_CASE);
	AddKeyword("char", TK_CHAR);
	AddKeyword("_Complex", TK_COMPLEX);
	AddKeyword("const", TK_CONST);
	AddKeyword("continue", TK_CONTINUE);
	AddKeyword("default", TK_DEFAULT);
	AddKeyword("do", TK_DO);
	AddKeyword("double", TK_DOUBLE);
	AddKeyword("else", TK_ELSE);
	AddKeyword("enum", TK_ENUM);
	AddKeyword("extern", TK_EXTERN);
	AddKeyword("float", TK_FLOAT);
	AddKeyword("for", TK_FOR);
	AddKeyword("goto", TK_GOTO);
	AddKeyword("if", TK_IF);
	AddKeyword("_Imaginary", TK_IMAGINARY);
	AddKeyword("inline", TK_INLINE);
	AddKeyword("int", TK_INT);
	AddKeyword("long", TK_LONG);
	AddKeyword("register", TK_REGISTER);
	AddKeyword("restrict", TK_RESTRICT);
	AddKeyword("return", TK_RETURN);
	AddKeyword("short", TK_SHORT);
	AddKeyword("signed", TK_SIGNED);
	AddKeyword("sizeof", TK_SIZEOF);
	AddKeyword("static", TK_STATIC);
	AddKeyword("struct", TK_STRUCT);
	AddKeyword("switch", TK_SWITCH);
	AddKeyword("typedef", TK_TYPEDEF);
	AddKeyword("union", TK_UNION);
	AddKeyword("unsigned", TK_UNSIGNED);
	AddKeyword("void", TK_VOID);
	AddKeyword("volatile", TK_VOLATILE);
	AddKeyword("while", TK_WHILE);
}

// Retrieve Next character from content and store it 
// as the current token. If there is no Next token this
// function will set the end of file toggle and push the
// EndofUnit as current character.
void Lexer::Next()
{
read_again:
	if (m_offset < m_content.size()) {
		m_currentChar = m_content[m_offset++];
		m_currentColumn++;
		return;
	}

	// If the next offset is zero, there are no more source chunks left.
	// Otherwise ask the caller for more input data.
	if (m_offset > 0) {
		ConsumeNextChunk();
		goto read_again;
	}

	m_isEof = true;
	m_currentChar = EndOfUnit;
}

void Lexer::VNext()
{
	m_currentLine++;
	m_prevToken = m_currentToken;
	m_currentToken = '\n';
	Next();
	m_currentColumn = 1;
}

int Lexer::DefaultLexSet(char lexChar)
{
	switch (lexChar) {

		//TODO: \f
		//TODO: \v

	case '\t':
	case '\r':
	case ' ':
		// Ignore all whitespaces and continue with the Next character
		Next();
		return CONTINUE_NEXT_TOKEN;

	case '\n':
		// Move onto the next line and keep track of where we are in the source
		VNext();
		return CONTINUE_NEXT_TOKEN;

	case '/':
		Next();
		switch (m_currentChar) {
		case '*':
			Next(); //TODO: remove?
			LexBlockComment();
			return CONTINUE_NEXT_TOKEN;
		case '/':
			LexLineComment();
			return CONTINUE_NEXT_TOKEN;
		case '=':
			Next();
			return AssembleToken(TK_DIV_ASSIGN);
		default:
			return AssembleToken(TK_SLASH);
		}

	case '=':
		Next();
		if (m_currentChar != '=') {
			return AssembleToken(TK_ASSIGN);
		}
		else {
			Next();
			return AssembleToken(TK_EQ_OP);
		}

	case '<':
		Next();
		switch (m_currentChar) {
		case '=':
			Next();
			return AssembleToken(TK_LE_OP);
			break;
		case '<':
			Next();
			if (m_currentChar == '=') {
				Next();
				return AssembleToken(TK_LEFT_ASSIGN);
			}
			else {
				return AssembleToken(TK_LEFT_OP);
			}
		}
		return AssembleToken(TK_LESS_THAN);

	case '>':
		Next();
		if (m_currentChar == '=') {
			Next();
			return AssembleToken(TK_GE_OP);
		}
		else if (m_currentChar == '>') {
			Next();
			if (m_currentChar == '=') {
				Next();
				return AssembleToken(TK_RIGHT_ASSIGN);
			}
			else {
				return AssembleToken(TK_RIGHT_OP);
			}
		}
		else {
			return AssembleToken(TK_GREATER_THAN);
		}

	case '!':
		Next();
		if (m_currentChar != '=') {
			return AssembleToken(TK_NOT);
		}
		else {
			Next();
			return AssembleToken(TK_NE_OP);
		}

	case '"':
	case '\'':
	{
		int stype;
		if ((stype = ReadString(m_currentChar)) != -1) {
			return AssembleToken(stype);
		}
		Error("error parsing string");
	}

	case '{':
		Next();
		return AssembleToken(TK_BRACE_OPEN);
	case '}':
		Next();
		return AssembleToken(TK_BRACE_CLOSE);
	case '(':
		Next();
		return AssembleToken(TK_PARENTHESE_OPEN);
	case ')':
		Next();
		return AssembleToken(TK_PARENTHESE_CLOSE);
	case '[':
		Next();
		return AssembleToken(TK_BRACKET_OPEN);
	case ']':
		Next();
		return AssembleToken(TK_BRACKET_CLOSE);
	case ';':
		Next();
		return AssembleToken(TK_COMMIT);
	case ',':
		Next();
		return AssembleToken(TK_COMMA);
	case '?':
		Next();
		return AssembleToken(TK_QUESTION_MARK);
	case '~':
		Next();
		return AssembleToken(TK_TILDE);
	case ':':
		Next();
		return AssembleToken(TK_COLON);

	case '^':
		Next();
		if (m_currentChar == '=') {
			Next();
			return AssembleToken(TK_XOR_ASSIGN);
		}
		else {
			return AssembleToken(TK_CARET);
		}

	case '.':
		Next();
		if (m_currentChar != '.') {
			return AssembleToken(TK_DOT);
		}
		Next();
		if (m_currentChar != '.') {
			Error("invalid token '..'");
		}
		Next();
		return AssembleToken(TK_ELLIPSIS);

	case '&':
		Next();
		if (m_currentChar != '&') {
			return AssembleToken(TK_AMPERSAND);
		}
		else if (m_currentChar == '=') {
			Next();
			return AssembleToken(TK_AND_ASSIGN);
		}
		else {
			Next();
			return AssembleToken(TK_AND_OP);
		}

	case '|':
		Next();
		if (m_currentChar != '|') {
			return AssembleToken(TK_VERTIAL_BAR);
		}
		else if (m_currentChar == '=') {
			Next();
			return AssembleToken(TK_OR_ASSIGN);
		}
		else {
			Next();
			return AssembleToken(TK_OR_OP);
		}

	case '*':
		Next();
		if (m_currentChar == '=') {
			Next();
			return AssembleToken(TK_MUL_ASSIGN);
		}
		else {
			return AssembleToken(TK_ASTERISK);
		}

	case '%':
		Next();
		if (m_currentChar == '=') {
			Next();
			return AssembleToken(TK_MOD_ASSIGN);
		}
		else {
			return AssembleToken(TK_PERCENT);
		}

	case '-':
		Next();
		if (m_currentChar == '=') {
			Next();
			return AssembleToken(TK_SUB_ASSIGN);
		}
		else if (m_currentChar == '-') {
			Next();
			return AssembleToken(TK_DEC_OP);
		}
		else if (m_currentChar == '>') {
			Next();
			return AssembleToken(TK_PTR_OP);
		}
		else {
			return AssembleToken(TK_MINUS);
		}

	case '+':
		Next();
		if (m_currentChar == '=') {
			Next();
			return AssembleToken(TK_ADD_ASSIGN);
		}
		else if (m_currentChar == '+') {
			Next();
			return AssembleToken(TK_INC_OP);
		}
		else {
			return AssembleToken(TK_PLUS);
		}

	case EndOfUnit:
		// Reached end of input, so long ...
		return TK_HALT;

	default:
		// No token sequence matched so we either deal with scalars, ids or 
		// control carachters. If the first character is a digit, try to
		// parse the entire token as number.
		if (std::isdigit(m_currentChar)) {
			return AssembleToken(LexScalar());
		}
		else if (std::isalpha(m_currentChar) || m_currentChar == '_') {
			return AssembleToken(ReadID());
		}
		else {
			Error("stray '" + std::string{ m_currentChar } +"' in program");
		}
	}

	return TK_HALT;
}

int Lexer::Lex()
{
	m_data.reset();
	m_lastTokenLine = m_currentLine;
	while (m_currentChar != EndOfUnit) {
		int token = DefaultLexSet(m_currentChar);
		if (token == CONTINUE_NEXT_TOKEN) { continue; }
		return token;
	}

	// Halt if enf of unit is reached
	return TK_HALT;
}

void Lexer::LexBlockComment()
{
	bool done = false;
	while (!done) {
		switch (m_currentChar) {
		case '*':
		{
			Next();
			if (m_currentChar == '/') {
				done = true;
				Next();
			}
		};
		continue;
		case '\n':
			m_currentLine++;
			Next();
			continue;
		case EndOfUnit:
			return;
		default:
			Next();
		}
	}
}

void Lexer::LexLineComment()
{
	do {
		Next();
	} while (m_currentChar != '\n' && m_currentChar != EndOfUnit);
}

int Lexer::ReadString(int ndelim)
{
	std::string _longstr;

	Next();
	/*if (IS_EOB()) {
		return -1;
	}*/

	while (m_currentChar != ndelim && m_currentChar != EndOfUnit) {
		_longstr.push_back(m_currentChar);
		Next();
	}

	Next();

	int len = _longstr.size();
	if (ndelim == '\'') {
		if (len == 0) { Error("empty constant"); }
		if (len > 1) { Error("constant too long"); }

		auto _cvalue = _longstr[0];
		m_data = CoilCl::Util::MakeValueObject<decltype(_cvalue)>(CoilCl::Typedef::BuiltinType::Specifier::CHAR, _cvalue);
		return Token::TK_CONSTANT;
	}

	std::string _svalue = _longstr;
	m_data = CoilCl::Util::MakeValueObject<decltype(_svalue)>(CoilCl::Typedef::BuiltinType::Specifier::CHAR, _svalue);
	return Token::TK_CONSTANT;
}

int Lexer::ReadID()
{
	std::string _longstr;

	do {
		_longstr.push_back(m_currentChar);
		Next();
	} while (std::isalnum(m_currentChar) || m_currentChar == '_');

	// Match string as keyword
	auto result = m_keywords.find(_longstr);
	if (result != m_keywords.end()) {
		return static_cast<int>(result->second.m_token);
	}

	// Save string as identifier
	auto _svalue = _longstr;
	m_data = CoilCl::Util::MakeValueObject<decltype(_svalue)>(CoilCl::Typedef::BuiltinType::Specifier::CHAR, _svalue);
	return Token::TK_IDENTIFIER;
}

int Lexer::LexScalar()
{
	enum
	{
		INT = 1,
		DOUBLE = 2,
		HEX = 3,
		SCIENTIFIC = 4,
		OCTAL = 5,
	} ScalarType;

	const int firstchar = m_currentChar;

	const auto isodigit = [](int c) -> bool { return c >= '0' && c <= '7'; };
	const auto isexponent = [](int c) -> bool { return c == 'e' || c == 'E'; };

	std::string _longstr;

	Next();

	// Check if we dealing with an octal or hex. If not then we know it is some integer
	if (firstchar == '0' && (std::toupper(m_currentChar) == 'X' || isdigit(m_currentChar))) {
		if (isodigit(m_currentChar)) {
			ScalarType = OCTAL;
			while (isodigit(m_currentChar)) {
				_longstr.push_back(m_currentChar);
				Next();
			}
			if (std::isdigit(m_currentChar)) {
				Error("invalid octal number");
			}
		}
		else {
			Next();
			ScalarType = HEX;
			while (isxdigit(m_currentChar)) {
				_longstr.push_back(m_currentChar);
				Next();
			}
			if (_longstr.size() > sizeof(int) * 2) {
				Error("too many digits for an Hex number");
			}
		}
	}
	else {
		// At this point we know the temporary buffer contains an integer.
		ScalarType = INT;
		_longstr.push_back(static_cast<char>(const_cast<int&>(firstchar)));
		while (m_currentChar == '.' || std::isdigit(m_currentChar) || isexponent(m_currentChar)) {
			if (m_currentChar == '.' || isexponent(m_currentChar)) {
				ScalarType = DOUBLE;
			}
			if (isexponent(m_currentChar)) {
				if (ScalarType != DOUBLE) {
					Error("invalid numeric format");
				}

				ScalarType = SCIENTIFIC;
				_longstr.push_back(m_currentChar);
				Next();
				if (m_currentChar == '+' || m_currentChar == '-') {
					_longstr.push_back(m_currentChar);
					Next();
				}
				if (!std::isdigit(m_currentChar)) {
					Error("exponent expected");
				}
			}

			_longstr.push_back(m_currentChar);
			Next();
		}
	}

	switch (ScalarType) {
	case SCIENTIFIC:
	case DOUBLE:
	{
		auto _fvalue = boost::lexical_cast<double>(_longstr);
		m_data = CoilCl::Util::MakeValueObject<decltype(_fvalue)>(CoilCl::Typedef::BuiltinType::Specifier::DOUBLE, _fvalue);
		return TK_CONSTANT;
	}
	case OCTAL:
	case INT:
	{
		auto _nvalue = boost::lexical_cast<int>(_longstr);
		m_data = CoilCl::Util::MakeValueObject<decltype(_nvalue)>(CoilCl::Typedef::BuiltinType::Specifier::INT, _nvalue);
		return TK_CONSTANT;
	}
	case HEX:
	{
		int _nvalue = std::stoul(_longstr, nullptr, 16);
		m_data = CoilCl::Util::MakeValueObject<decltype(_nvalue)>(CoilCl::Typedef::BuiltinType::Specifier::INT, _nvalue);
		return TK_CONSTANT;
	}
	}

	return TK_HALT;
}

Lexer::Lexer(std::shared_ptr<CoilCl::Profile>& profile)
	: m_profile{ profile }
{
	// Register all tokenized keywords
	InitKeywords();

	// Fetch first datachunk
	ConsumeNextChunk();

	// Push the first character into the current character variable
	Next();
}