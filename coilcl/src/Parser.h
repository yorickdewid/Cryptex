// Copyright (c) 2017 Quenza Inc. All rights reserved.
//
// This file is part of the Cryptox project.
//
// Use of this source code is governed by a private license
// that can be found in the LICENSE file. Content can not be 
// copied and/or distributed without the express of the author.

#pragma once

#include "Profile.h"
#include "Lexer.h"
#include "AST.h"
#include "Stage.h"
#include "LockPipe.h"

#include <deque>
#include <stack>
#include <map>

using namespace CoilCl::Valuedef;

template<typename _Ty>
struct is_stack : public std::false_type {};

template<typename _Ty, typename _Alloc>
struct is_stack<std::stack<_Ty, _Alloc>> : public std::true_type {};

template<typename _Ty, class = typename std::enable_if<is_stack<_Ty>::value>::type>
void ClearStack(_Ty& c)
{
	while (!c.empty()) { c.pop(); }
}

class TokenState
{
	Token m_currentToken;
	std::shared_ptr<Value> m_currentData = nullptr;
	int m_line;
	int m_column;

public:
	TokenState(Token currentToken, std::shared_ptr<Value>& currentData, int line = 0, int column = 0)
		: m_currentToken{ currentToken }
		, m_currentData{ currentData }
		, m_line{ line }
		, m_column{ column }
	{
	}

	TokenState(int currentToken, std::shared_ptr<Value>& currentData, int line = 0, int column = 0)
		: m_currentToken{ static_cast<Token>(currentToken) }
		, m_currentData{ currentData }
		, m_line{ line }
		, m_column{ column }
	{
	}

	TokenState(int currentToken, std::shared_ptr<Value>& currentData, std::pair<int, int>&& location)
		: m_currentToken{ static_cast<Token>(currentToken) }
		, m_currentData{ currentData }
		, m_line{ location.first }
		, m_column{ location.second }
	{
	}

	TokenState(const TokenState& other) = default;
	TokenState(TokenState&& other) = default;

	inline bool HasData() const { return !!m_currentData; }

	inline const std::shared_ptr<Value>& FetchData() { return m_currentData; }

	// Fetch token from current token state
	inline auto FetchToken() const { return m_currentToken; }

	// Fetch source line from current token state
	inline auto FetchLine() const { return m_line; }

	// Fetch source column from current token state
	inline auto FetchColumn() const { return m_column; }

	inline auto FetchLocation() const { return std::make_pair(m_line, m_column); }
};

template<typename _Ty>
class Stash
{
public:
	template<typename _STy>
	void Enlist(_STy& type)
	{
		using shared_type = typename _STy::element_type;
		m_stash.push_back(static_cast<std::weak_ptr<_Ty>>(std::weak_ptr<shared_type>(type)));
	}

	template<typename _DeclTy, typename _BaseTy = _DeclTy, class = typename std::enable_if<std::is_base_of<_BaseTy, _DeclTy>::value>::type>
	auto Resolve(std::function<bool(std::shared_ptr<_DeclTy>)> checkCb) -> std::shared_ptr<_BaseTy>
	{
		for (auto& ptr : m_stash) {
			if (auto node = ptr.lock()) {
				auto declRs = std::dynamic_pointer_cast<_DeclTy>(node);
				if (declRs != nullptr) {
					if (checkCb(declRs)) {
						return std::dynamic_pointer_cast<_BaseTy>(declRs);
					}
				}
			}
		}

		return nullptr;
	}

private:
	std::vector<std::weak_ptr<_Ty>> m_stash;
};

template<typename _Ty>
class StateContainer
{
	std::stack<size_t> m_snapshopList;
	std::vector<_Ty> m_tokenList;
	mutable size_t index = 0;

public:
	StateContainer(size_t reserved_elements = 10)
	{
		m_tokenList.reserve(reserved_elements);
	}

	// Prevent all forms of data transfer from this object
	StateContainer(const StateContainer&) = delete;
	StateContainer(StateContainer&&) = delete;
	StateContainer& operator=(const StateContainer&) = delete;
	StateContainer& operator=(StateContainer&&) = delete;

	void Push(_Ty&& state)
	{
		m_tokenList.push_back(std::move(state));
		index++;
	}

	inline auto& Previous() { return m_tokenList[index > 1 ? (index - 1) : index]; }
	inline auto& Current() { return m_tokenList[index - 1]; }

	// Take snapshot of current index
	inline void Snapshot()
	{
		m_snapshopList.push(index);
	}

	// Revert to last snapshot
	inline void Revert()
	{
		index = m_snapshopList.top();
		m_snapshopList.pop();
	}

	// Dispose last snapshot
	inline void DisposeSnapshot()
	{
		m_snapshopList.pop();
	}

	// Check if the next item is the last item
	inline auto IsIndexHead() const { return index == m_tokenList.size(); }

	// Take one step forward
	inline void ShiftForward() const
	{
		if (m_tokenList.size() > index) {
			++index;
		}
	}

	// Take one step back, but preserve the list
	inline void ShiftBackward() const
	{
		if (index > 0) {
			--index;
		}
	}

	// Access token at random position
	inline auto& operator[](size_t idx) const
	{
		return m_tokenList[idx];
	}

	// Remove all snapshots, and restore index
	void Reset()
	{
		index = m_tokenList.size();
		ClearStack(m_snapshopList);
	}

	// Clear the token and snapshot list, this action will free all allocated memory
	void Clear()
	{
		index = 0;
		m_tokenList.clear();
		ClearStack(m_snapshopList);
	}

	// If the next item is the last item, clear the list
	// and copy the last element back in the container
	void TryClear()
	{
		if (IsIndexHead()) {
			auto currentItem = Current();
			Clear();
			Push(std::move(currentItem));
		}
	}
};

struct CompareStringPair
{
	using type = std::pair<std::string, int>;

	auto operator()(const type& a, const type& b) const
	{
		return a.first < b.first;
	}
};

class Parser : public CoilCl::Stage<Parser>
{
public:
	Parser(std::shared_ptr<CoilCl::Profile>& profile);

	std::string Name() const { return "Parser"; }

	Parser& Execute();
	Parser& CheckCompatibility();

	std::shared_ptr<TranslationUnitDecl> DumpAST() const
	{
		if (m_ast == nullptr) {
			throw StageBase::StageException{ Name(), "abstract program is empty" };
		}

		return m_ast;
	}

protected:
	void Error(const char *err, Token token);
	void ExpectToken(Token token);
	void ExpectIdentifier();

	void NextToken()
	{
		if (m_comm.IsIndexHead()) {
			auto itok = lex.Lex();
			auto location = std::make_pair(lex.TokenLine(), lex.TokenColumn());
			auto val = lex.HasData() ? std::shared_ptr<Value>(std::move(lex.Data())) : nullptr;
			m_comm.Push(TokenState(itok, val, std::move(location)));
		}
		else {
			m_comm.ShiftForward();
		}
	}

private:
	auto StorageClassSpecifier();
	auto TypeQualifier();
	bool TypeSpecifier();
	bool DeclarationSpecifiers();
	bool TypenameSpecifier();
	bool StructOrUnionSpecifier();
	void SpecifierQualifierList();
	bool EnumSpecifier();
	bool UnaryOperator();
	bool AssignmentOperator();
	void CompoundLiteral();

private: // Expressions
	void PrimaryExpression();
	void ArgumentExpressionList();
	void PostfixExpression();
	void UnaryExpression();
	void CastExpression();
	void MultiplicativeExpression();
	void AdditiveExpression();
	void ShiftExpression();
	void RelationalExpression();
	void EqualityExpression();
	void AndExpression();
	void ExclusiveOrExpression();
	void LogicalAndExpression();
	void LogicalOrExpression();
	void ConditionalExpression();
	void AssignmentExpression();
	void Expression();
	void ConstantExpression();

private: // Statements
	bool JumpStatement();
	bool LabeledStatement();
	bool CompoundStatement();
	void ExpressionStatement();
	bool SelectionStatement();
	bool IterationStatement();
	void Statement();

private: // Declarations
	void BlockItems();
	void Declaration();
	void InitDeclaratorList();
	void TypeName();
	void AbstractDeclarator();
	void DirectAbstractDeclarator();
	void Initializer();
	void Designation();
	void Designators();
	void Pointer();
	bool Declarator();
	bool DirectDeclarator();
	void TypeQualifierList();
	bool ParameterTypeList();
	bool ParameterDeclaration();

private:
	bool FunctionDefinition();
	void ExternalDeclaration();
	void TranslationUnit();

private:
	Lexer lex;
	std::shared_ptr<TranslationUnitDecl> m_ast;
	StateContainer<TokenState> m_comm;
	std::shared_ptr<CoilCl::Profile> m_profile;

	// Temporary parser containers
	size_t m_pointerCounter = 0;
	std::map<std::pair<std::string, int>, std::shared_ptr<RecordDecl>, CompareStringPair> m_recordList;
	std::map<std::string, std::shared_ptr<Typedef::TypedefBase>> m_typedefList;
	std::stack<std::shared_ptr<Typedef::TypedefBase>> m_typeStack;
	std::stack<std::string> m_identifierStack;
	LockPipe<std::shared_ptr<ASTNode>> m_elementDescentPipe;
};
