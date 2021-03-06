// Copyright (c) 2017 Quenza Inc. All rights reserved.
//
// This file is part of the Cryptox project.
//
// Use of this source code is governed by a private license
// that can be found in the LICENSE file. Content can not be
// copied and/or distributed without the express of the author.

//FUTURE:
// - Macro expansion

#include "Preprocessor.h"
#include "DirectiveScanner.h" //TODO: remove, only used for tokens

#include <Cry/Cry.h>
#include <Cry/Config.h>
#include <Cry/ByteOrder.h>

#include <boost/logic/tribool.hpp>

#include <set>
#include <stack>
#include <cassert>
#include <iostream>

#define DEFINE_MACRO_STR(k,v) \
	{ \
		TokenProcessor::DataType m_data = Util::MakeString(v); \
		std::vector<Preprocessor::TokenDataPair<TokenProcessor::TokenType, const TokenProcessor::DataType>> m_definitionBody; \
		m_definitionBody.push_back({ 20, m_data }); \
		g_definitionList.insert({ k, std::move(m_definitionBody) }); \
	}

#define DEFINE_MACRO_INT(k,v) \
	{ \
		TokenProcessor::DataType m_data = Util::MakeInt(v); \
		std::vector<Preprocessor::TokenDataPair<TokenProcessor::TokenType, const TokenProcessor::DataType>> m_definitionBody; \
		m_definitionBody.push_back({ 20, m_data }); \
		g_definitionList.insert({ k, std::move(m_definitionBody) }); \
	}

#undef Yield

using namespace CoilCl;
using namespace CryCC::SubValue::Typedef;

namespace CoilCl
{
namespace MacroHelper
{

TokenProcessor::DataType DynamicGlobalCounter();
TokenProcessor::DataType DynamicSourceFile();
TokenProcessor::DataType DynamicSourceLine();
TokenProcessor::DataType DynamicDate();
TokenProcessor::DataType DynamicTime();

} // namespace MacroHelper
} // namespace CoilCl

static std::set<std::string> g_sourceGuardList; //TODO: should not be global
static std::map<std::string, std::vector<Preprocessor::TokenDataPair<TokenProcessor::TokenType, const TokenProcessor::DataType>>> g_definitionList; //TODO: should not be global
static const std::map<std::string, std::function<TokenProcessor::DataType()>> g_macroList = {
	//{ "__func__", []() { CryImplExcept(); } },
	{ "__FILE__", CoilCl::MacroHelper::DynamicSourceFile },
	{ "__LINE__", CoilCl::MacroHelper::DynamicSourceLine },
	{ "__DATE__", CoilCl::MacroHelper::DynamicDate },
	{ "__TIME__", CoilCl::MacroHelper::DynamicTime },
	//{ "__STDC__", []() { CryImplExcept(); } },
	{ "__COUNTER__", CoilCl::MacroHelper::DynamicGlobalCounter },
};

//TODO: move into cry/Algorithm
namespace Cry
{
namespace Algorithm
{

template<typename _Ty, typename _KeyTy, class _Predicate>
auto ForEachRangeEqual(_Ty& set, _KeyTy& key, _Predicate p)
{
	auto range = set.equal_range(key);
	for (auto it = range.first; it != range.second; it++) {
		if (p(it->second)) {
			return it;
		}
	}

	return set.end();
}

} // namespace Algorithm
} // namespace Cry

static class TokenSubscription
{
public:
	using CallbackFunc = void(*)(bool, Preprocessor::DefaultTokenDataPair&);

public:
	// Register callback for token, but only if token callback pair does not exist
	void SubscribeOnToken(int token, CallbackFunc cb)
	{
		const auto& range = m_subscriptionTokenSet.equal_range(token);
		bool isRegistered = std::any_of(range.first, range.second, [&cb](std::multimap<int, CallbackFunc>::value_type pair)
		{
			return pair.second == cb;
		});

		if (isRegistered) { return; }

		m_subscriptionTokenSet.emplace(token, cb);
	}

	// Find token and callback, then erase from set
	void UnsubscribeOnToken(int token, CallbackFunc cb)
	{
		auto it = Cry::Algorithm::ForEachRangeEqual(m_subscriptionTokenSet, token, [&cb](CallbackFunc& _cb)
		{
			return _cb == cb;
		});

		if (it != m_subscriptionTokenSet.end()) {
			m_subscriptionTokenSet.erase(it);
		}
	}

	// Register any calls that trigger on each token
	void SubscribeOnAll(CallbackFunc cb)
	{
		m_subscriptionSet.emplace(cb);
	}

	// Find the callback, and erase from set
	void UnsubscribeOnAll(CallbackFunc cb)
	{
		m_subscriptionSet.erase(cb);
	}

	// Invoke all callbacks for this token
	void CallAnyOf(bool isDirective, Preprocessor::DefaultTokenDataPair& tokenData)
	{
		auto range = m_subscriptionTokenSet.equal_range(tokenData.Token());
		for (auto& it = range.first; it != range.second; ++it) {
			(it->second)(isDirective, tokenData);
		}

		// Invoke any callback function that was registered for all tokens
		auto it = m_subscriptionSet.begin();
		while (it != m_subscriptionSet.end()) {
			(*it++)(isDirective, tokenData);
		}
	}

	void Clear()
	{
		m_subscriptionTokenSet.clear();
		m_subscriptionSet.clear();
	}

private:
	std::multimap<int, CallbackFunc> m_subscriptionTokenSet;
	std::set<CallbackFunc> m_subscriptionSet;
} g_tokenSubscription;

// Convert program version parts into version integer
constexpr int ProgramCounterId()
{
	return (PRODUCT_VERSION_MAJOR * 1000000)
		+ (PRODUCT_VERSION_MINOR * 10000)
		+ (PRODUCT_VERSION_PATCH * 100)
		+ (PRODUCT_VERSION_LOCAL);
}

//TODO: register some marcros in the frontend
void RegisterMacros()
{
	DEFINE_MACRO_STR("__VERSION__", PROGRAM_VERSION);
	DEFINE_MACRO_INT("__CRYC__", 1);
	DEFINE_MACRO_INT("__CRYC_VERSION__", ProgramCounterId());
	DEFINE_MACRO_INT("__TIMESTAMP__", static_cast<int>(time(nullptr)));

	// Limit to big and little endian only
#ifdef CRY_BIG_ENDIAN
	DEFINE_MACRO_INT("__BIG_ENDIAN__", 1);
#else
	DEFINE_MACRO_INT("__LITTLE_ENDIAN__", 1);
#endif

#ifdef _WIN32
	DEFINE_MACRO_INT("_WIN32", 1);
#endif
#ifdef _WIN64
	DEFINE_MACRO_INT("_WIN64", 1);
#endif
#if defined(linux) || defined(__linux) || defined(__linux__)
	DEFINE_MACRO_INT("linux", 1);
	DEFINE_MACRO_INT("__linux", 1);
	DEFINE_MACRO_INT("__linux__", 1);
	DEFINE_MACRO_INT("__gnu_linux", 1);
#endif
#ifdef __FreeBSD__
	DEFINE_MACRO_INT("__FreeBSD__", 1);
#endif
#ifdef __DragonFly__
	DEFINE_MACRO_INT("__DragonFly__", 1);
#endif
#ifdef __NetBSD__
	DEFINE_MACRO_INT("__NetBSD__", 1);
#endif
#ifdef __OpenBSD__
	DEFINE_MACRO_INT("__OpenBSD__", 1);
#endif
#ifdef __APPLE__
	DEFINE_MACRO_INT("__APPLE__", 1);
#endif
#ifdef __MACH__
	DEFINE_MACRO_INT("__MACH__", 1);
#endif
#if defined(unix) || defined(__unix) || defined(__unix__)
	DEFINE_MACRO_INT("unix", 1);
	DEFINE_MACRO_INT("__unix", 1);
	DEFINE_MACRO_INT("__unix__", 1);
#endif

	//FUTURE:
	//DEFINE_MACRO_INT("_DEBUG", 1);
}

Preprocessor& Preprocessor::CheckCompatibility()
{
	return (*this);
}

namespace CoilCl
{
namespace LocalMethod
{

class AbstractDirective
{
public:
	AbstractDirective() = default;
	virtual void Yield() {}
	virtual void Dispence(TokenProcessor::DefaultTokenDataPair&) = 0;

protected:
	class DirectiveException : public std::runtime_error
	{
	public:
		DirectiveException(const std::string& message) noexcept
			: std::runtime_error{ message.c_str() }
		{
		}

		explicit DirectiveException(const std::string& directive, const std::string& message) noexcept
			: std::runtime_error{ (directive + ": " + message).c_str() }
		{
		}
	};

	class UnexpectedTokenException : public DirectiveException
	{
	public:
		UnexpectedTokenException(const std::string& message) noexcept
			: DirectiveException{ message }
		{
		}
	};

	void RequireToken(int expectedToken, int token)
	{
		//TODO:
		if (expectedToken != token) {
			throw UnexpectedTokenException{ "expected token" };
		}
	}

	void RequireData(const TokenProcessor::DefaultTokenDataPair& tokenData)
	{
		// Data was expected, throw if not found
		if (!tokenData.HasData()) {
			throw DirectiveException{ "expected constant" };
		}
	}
};

class ImportSource : public AbstractDirective
{
	std::shared_ptr<Profile>& m_profile;
	bool hasBegin = false;
	std::string tempSource;

	// Request input source push from the frontend
	void Import(const std::string& source)
	{
		// Return value is a mock, ignore for now
		m_profile->Include(source);
	}

public:
	ImportSource(std::shared_ptr<Profile>& profile)
		: m_profile{ profile }
	{
	}

	void Dispence(TokenProcessor::DefaultTokenDataPair& tokenData)
	{
		switch (tokenData.Token()) {
		case TK_LESS_THAN: // Global includes begin
			hasBegin = true;
			break;
		case TK_GREATER_THAN: // Global includes end
			if (!hasBegin) throw;
			Import(tempSource);
			break;
		case TK_CONSTANT: // Local include
			RequireData(tokenData);
			Import(Util::ValueCastString(tokenData.Data()));
			break;
		default:
			if (hasBegin) {
				if (!tokenData.HasData()) {
					//FUTURE: Each token must be able to fetch its characteral representation
					//TODO: This list not complete
					switch (tokenData.Token()) {
					case TK_DOT:
						tempSource.push_back('.');
						break;
					case TK_SLASH:
						tempSource.push_back('/');
						break;
					case TK_COMMA:
						tempSource.push_back(',');
					}
					break;
				}

				tempSource.append(Util::ValueCastString(tokenData.Data()));
				break;
			}

			// Did not match the parsing pattern, throw
			throw DirectiveException{ "include", "expected constant or '<' after 'include'" };
		}
	}
};

// Definition and expansion
class DefinitionTag : public AbstractDirective
{
	std::string m_definitionName;
	std::vector<Preprocessor::TokenDataPair<TokenProcessor::TokenType, const TokenProcessor::DataType>> m_definitionBody;

public:
	void Dispence(TokenProcessor::DefaultTokenDataPair& tokenData)
	{
		// First item must be the definition name
		if (m_definitionName.empty()) {
			RequireData(tokenData);
			const auto definitionName = Util::ValueCastString(tokenData.Data());
			if (g_definitionList.find(definitionName) != g_definitionList.end()) {
				throw DirectiveException{ "define", "'" + definitionName + "' already defined" };
			}

			m_definitionName = definitionName;
			return;
		}

		// Save token with optional data on the vector
		m_definitionBody.push_back({ tokenData.Token(), tokenData.Data() });
	}

	// If the data matches a definition in the global definition list, replace it
	static void OnPropagateCallback(bool isDirective, Preprocessor::DefaultTokenDataPair& dataPair)
	{
		using namespace Valuedef;
		using namespace Typedef;

		// Do not interfere with preprocessor lines
		if (isDirective) { return; }

		auto mit = g_macroList.find(Util::ValueCastString(dataPair.Data()));
		if (mit != g_macroList.end()) {
			dataPair.AssignToken(TK_CONSTANT);
			dataPair.AssignData(mit->second());
			return;
		}

		auto it = g_definitionList.find(Util::ValueCastString(dataPair.Data()));
		if (it == g_definitionList.end()) { return; }

		// Definition without body, reset all
		if (it->second.empty()) {
			dataPair.ResetToken();
			dataPair.ResetData();
			return;
		}

		// Always assign first token and optional data
		dataPair.AssignToken(it->second.at(0).Token());
		dataPair.AssignData(it->second.at(0).Data());

		// When multiple tokens are registered for this definition, create a token queue
		if (it->second.size() > 1) {
			auto dequqPtr = std::make_unique<std::deque<decltype(g_definitionList)::mapped_type::value_type>>();
			for (auto subit = it->second.begin() + 1; subit != it->second.end(); ++subit) {
				dequqPtr->push_back(std::move((*subit)));
			}

			dataPair.EmplaceTokenQueue(std::move(dequqPtr));
		}
	}

	void Yield() override
	{
		if (m_definitionName.empty()) { return; }

		// FUTURE: OPTIMIZATION: Try intergral evaluation before move. Since it is unknown of the
		//         definition body consists of an arithmetic construction the operation has a good
		//         change of throwing an exception.
		// Insert definition body into global definition list
		const auto& result = g_definitionList.insert({ m_definitionName, std::move(m_definitionBody) });
		assert(result.second);
	}
};

// Remove definition from list
class DefinitionUntag : public AbstractDirective
{
public:
	void Dispence(TokenProcessor::DefaultTokenDataPair& tokenData)
	{
		RequireData(tokenData);
		auto it = g_definitionList.find(Util::ValueCastString(tokenData.Data()));
		if (it == g_definitionList.end()) { return; }

		// Remove definition from global define list
		g_definitionList.erase(it);
	}
};

// Conditional compilation
class ConditionalStatement : public AbstractDirective
{
	static std::stack<std::pair<bool, bool>> evaluationResult;
	std::vector<Preprocessor::TokenDataPair<TokenProcessor::TokenType, const TokenProcessor::DataType>> m_statementBody;

	class ConditionalStatementException : public std::runtime_error
	{
	public:
		ConditionalStatementException(const std::string& message) noexcept
			: std::runtime_error{ message.c_str() }
		{
		}
	};

	// Evaluate statemenet and return either true for positive
	// result, or false for negative. An evaluation error will
	// throw an exception.
	static bool Eval(std::vector<Preprocessor::TokenDataPair<TokenProcessor::TokenType, const TokenProcessor::DataType>>&& statement)
	{
		class ChainAction
		{
			boost::logic::tribool chainState{ boost::logic::indeterminate };

		public:
			bool conjunction : 1;
			bool disjunction : 1;
			bool negation : 1;

			ChainAction() { Reset(); }

			// Reset all actions
			void Reset()
			{
				conjunction = false;
				disjunction = false;
				negation = false;
			}

			// Return evaluated expression as boolean
			inline bool Consensus() const noexcept
			{
				assert(!boost::logic::indeterminate(chainState));
				return chainState;
			}

			// Boolean arithmetic
			void Consolidate(bool b)
			{
				if (negation) { b = !b; }
				if (conjunction) {
					assert(!boost::logic::indeterminate(chainState));
					chainState = chainState && b;
				}
				else if (disjunction) {
					assert(!boost::logic::indeterminate(chainState));
					chainState = chainState || b;
				}
				else { chainState = b; }

				Reset();
			}
		} consensusAction;

		// Two value stack represents lhs and rhs
		int stack[2] = { 0,0 };

		for (auto it = statement.begin(); it != statement.end(); ++it) {
			switch (it->Token()) {
			case TK_IDENTIFIER:
			case TK_CONSTANT:
			{
				assert(it->HasData());
				switch (it->Data().Type().DataType<BuiltinType>()->TypeSpecifier()) {
				case BuiltinType::Specifier::INT:
				{
					stack[0] = Util::ValueCastNative<int>(it->Data());
					consensusAction.Consolidate(stack[0]);
					break;
				}
				case BuiltinType::Specifier::CHAR:
				{
					//TODO: Still the case?
					const std::string definition = Util::IsArray(it->Data().Type())
						? Util::ValueCastString(it->Data())
						: std::string{ Util::ValueCastNative<char>(it->Data()) };
					bool hasDefinition = g_definitionList.find(definition) != g_definitionList.end();
					consensusAction.Consolidate(hasDefinition);
					break;
				}
				default:
					throw ConditionalStatementException{ "invalid constant in preprocessor expression" };
				}

				continue;
			}

			case TK_PARENTHESE_OPEN:
			case TK_PARENTHESE_CLOSE:
				continue;

				// defined statement
			case TK_PP_DEFINED:
			{
				++it;
				if (it->Token() == TK_PARENTHESE_OPEN) {
					++it;
					if (it->Token() != TK_IDENTIFIER) {
						throw ConditionalStatementException{ "expected identifier" };
					}
					assert(it->HasData());
					const std::string definition = Util::ValueCastString(it->Data());
					++it;
					if (it->Token() != TK_PARENTHESE_CLOSE) {
						throw ConditionalStatementException{ "expected )" };
					}
					consensusAction.Consolidate(g_definitionList.find(definition) != g_definitionList.end());
					continue;
				}
				if (it->Token() != TK_IDENTIFIER) {
					throw ConditionalStatementException{ "expected identifier" };
				}
				assert(it->HasData());
				const std::string definition = Util::ValueCastString(it->Data());
				consensusAction.Consolidate(g_definitionList.find(definition) != g_definitionList.end());
				continue;
			}

			// Logical operators
			case TK_AND_OP:
			{
				consensusAction.conjunction = true;
				continue;
			}
			case TK_OR_OP:
			{
				consensusAction.disjunction = true;
				continue;
			}
			case TK_NOT:
			{
				consensusAction.negation = true;
				continue;
			}

#define COMPARE_OP(o) \
			++it; \
			if (it->Token() != TK_CONSTANT) {  throw ConditionalStatementException{ "expected constant" }; } \
			assert(it->HasData()); \
			stack[1] = Util::ValueCastNative<int>(it->Data()); \
			consensusAction.Consolidate(stack[0] o stack[1]); \
			stack[0] = 0; stack[1] = 0;

			// Comparison operators
			case TK_GREATER_THAN:
			{
				COMPARE_OP(> );
				continue;
			}
			case TK_LESS_THAN:
			{
				COMPARE_OP(< );
				continue;
			}
			case TK_EQ_OP:
			{
				COMPARE_OP(== );
				continue;
			}
			case TK_GE_OP:
			{
				COMPARE_OP(>= );
				continue;
			}
			case TK_LE_OP:
			{
				COMPARE_OP(<= );
				continue;
			}
			case TK_NE_OP:
			{
				COMPARE_OP(!= );
				continue;
			}

#define ARITHMETIC_OP(o) \
			++it; \
			if (it->Token() != TK_CONSTANT) { throw ConditionalStatementException{ "expected constant" }; } \
			assert(it->HasData()); \
			stack[0] = stack[0] o Util::ValueCastNative<int>(it->Data());

			// Integral arithmetic
			case TK_PLUS:
			{
				ARITHMETIC_OP(+);
				continue;
			}
			case TK_MINUS:
			{
				ARITHMETIC_OP(-);
				continue;
			}
			case TK_ASTERISK:
			{
				ARITHMETIC_OP(*);
				continue;
			}
			case TK_SLASH:
			{
				ARITHMETIC_OP(/ );
				continue;
			}

			//FUTURE: %, ~, ^

			default:
				throw ConditionalStatementException{ "invalid token in preprocessor directive" };
			}
		}

		return consensusAction.Consensus();
	}

public:
	ConditionalStatement() = default;

	template<typename TokenType>
	ConditionalStatement(TokenType token)
	{
		m_statementBody.push_back({ token });
	}

	template<typename TokenType, typename... _ArgsTy>
	ConditionalStatement(TokenType token, _ArgsTy... args)
		: ConditionalStatement{ args... }
	{
		m_statementBody.push_back({ token });
	}

	void Dispence(TokenProcessor::DefaultTokenDataPair& tokenData)
	{
		m_statementBody.push_back({ tokenData.Token(), tokenData.Data() });
	}

	void Yield() override
	{
		if (m_statementBody.empty()) {
			throw ConditionalStatementException{ "statement with no expression" };
		}

		// Evaluate the statement and push the boolean result on the stack
		auto evalResult = Eval(std::move(m_statementBody));
		evaluationResult.push(std::make_pair(evalResult, evalResult));
		g_tokenSubscription.SubscribeOnAll(&ConditionalStatement::OnPropagateCallback);
	}

	static void OnPropagateCallback(bool isDirective, Preprocessor::DefaultTokenDataPair& dataPair)
	{
		CRY_UNUSED(isDirective);

		// If this token happens to be a conditional statement token, redirect
		switch (dataPair.Token()) {
		case TK_IF: { return; };
		case TK_PP_ELIF: { if (EncounterElseIf()) { return; } break; };
		case TK_ELSE: { EncounterElse(); dataPair.ResetToken(); return; };
		case TK_PP_ENDIF: { EncounterEndif(); dataPair.ResetToken(); return; }
		}

		// If the evaluation stack is empty, or the top item is true, bail
		if (evaluationResult.empty() || evaluationResult.top().first) { return; }

		// Resetting the token indicates the proxy must skip the token
		dataPair.ResetToken();
	}

	static bool EncounterElseIf()
	{
		if (evaluationResult.empty()) {
			throw ConditionalStatementException{ "unexpected elif" };
		}

		// If true was ever encoutered, skip clause and set state to false
		if (evaluationResult.top().second) {
			auto& top = evaluationResult.top();
			top.first = false;
			return false;
		}

		evaluationResult.pop();
		g_tokenSubscription.UnsubscribeOnAll(&ConditionalStatement::OnPropagateCallback);
		return true;
	}

	// Flip evaluation result
	static void EncounterElse()
	{
		if (evaluationResult.empty()) {
			throw ConditionalStatementException{ "unexpected else" };
		}

		// State was already toggled once, ignore else clause
		if (evaluationResult.top().first != evaluationResult.top().second) {
			return;
		}

		// Inverse top most element
		auto& top = evaluationResult.top();
		top.first = !top.first;
	}

	// End of if statement
	static void EncounterEndif()
	{
		if (evaluationResult.empty()) {
			throw ConditionalStatementException{ "unexpected endif" };
		}

		// Pop top item from evaluation stack. If this happens to be the 
		// last item on the stack also unregister the callback subscription.
		evaluationResult.pop();
		if (evaluationResult.empty()) {
			g_tokenSubscription.UnsubscribeOnAll(&ConditionalStatement::OnPropagateCallback);
		}
	}
};

std::stack<std::pair<bool, bool>> ConditionalStatement::evaluationResult;

// Parse compiler pragmas
class CompilerDialect : public AbstractDirective
{
	const std::array<std::string, 1> trivialToken = std::array<std::string, 1>{ "once" };

	bool HandleTrivialCase(const std::string& identifier)
	{
		if (!std::any_of(trivialToken.cbegin(), trivialToken.cend(), [&identifier](const std::string& tok)
		{
			return tok == identifier;
		})) {
			return false;
		}

		// In case of 'once' fetch the source name and add to source
		// guard list. If the source was already on the guard list, return
		// imediatly and skip this source.
		if (identifier == trivialToken[0]) {
			auto result = g_sourceGuardList.emplace("somefile.c");
			if (result.second) {
				//TODO: bail by exception
			}
		}

		return true;
	}

public:
	void Dispence(TokenProcessor::DefaultTokenDataPair& tokenData)
	{
		RequireToken(TK_IDENTIFIER, tokenData.Token());
		if (HandleTrivialCase(Util::ValueCastString(tokenData.Data()))) { return; }
	}
};

// Set source location to fixed line,col pair
class FixLocation : public AbstractDirective
{
public:
	void Dispence(TokenProcessor::DefaultTokenDataPair& tokenData)
	{
		CRY_UNUSED(tokenData);

		//TODO: We have no clue what to do with #line
	}
};

// Report linquistic error
class CompilerMessage : public AbstractDirective
{
public:
	void Dispence(TokenProcessor::DefaultTokenDataPair& tokenData)
	{
		if (tokenData.Token() != TK_CONSTANT) {
			throw DirectiveException{ "message", "expected constant after 'message'" };
		}

		// For now print to terminal
		std::cout << Util::ValueCastString(tokenData.Data()) << std::endl;
	}
};

// Report linquistic error
class LinguisticError : public AbstractDirective
{
	const bool m_isFatal;

public:
	LinguisticError(bool fatal = true)
		: m_isFatal{ fatal }
	{
	}

	void Dispence(TokenProcessor::DefaultTokenDataPair& tokenData)
	{
		if (tokenData.Token() != TK_CONSTANT) {
			throw DirectiveException{ "error", "expected constant after 'error'" };
		}

		// Throw if message was fatal, this wil halt all operations
		if (m_isFatal) {
			throw DirectiveException{ Util::ValueCastString(tokenData.Data()) };
		}
	}
};

} // namespace LocalMethod
} // namespace CoilCl

Preprocessor::Preprocessor(std::shared_ptr<CoilCl::Profile>& profile, CryCC::Program::ConditionTracker::Tracker tracker)
	: Stage{ this, CryCC::Program::StageType::Type::TokenProcessor, tracker }
	, m_profile{ profile }
{
	RegisterMacros();

	// Start with clean states.
	g_tokenSubscription.Clear();
	g_sourceGuardList.clear();
	g_definitionList.clear();

	// Subscribe on all identifier tokens apart from the define tag. This indicates
	// we can do work on 'normal' tokens that would otherwise flow directly through to
	// the caller frontend. In this case we register identifiers to examine and on match
	// replace with the corresponding value. This essentially allows for find-and-replace
	// semantics on the provided input before any other stage has perceived the token stream.
	// Since standard and common definition can always be used, register the hook in the
	// constructor and invoke the definition call on any identifier.
	g_tokenSubscription.SubscribeOnToken(TK_IDENTIFIER, &CoilCl::LocalMethod::DefinitionTag::OnPropagateCallback);
}

//TODO: Why not use references?
template<typename DirectiveType, typename... ArgTypes>
auto MakeMethod(ArgTypes... args) -> std::shared_ptr<DirectiveType>
{
	return std::make_shared<DirectiveType>(std::forward<ArgTypes>(args)...);
}

void Preprocessor::MethodFactory(TokenType token)
{
	using namespace ::LocalMethod;

	switch (token) {
	case TK_PP_INCLUDE:
		m_method = MakeMethod<ImportSource>(std::ref(m_profile));
		break;
	case TK_PP_DEFINE:
		m_method = MakeMethod<DefinitionTag>();
		break;
	case TK_PP_UNDEF:
		m_method = MakeMethod<DefinitionUntag>();
		break;
	case TK_IF:
		m_method = MakeMethod<ConditionalStatement>();
		break;
	case TK_PP_IFDEF:
		m_method = MakeMethod<ConditionalStatement>(TK_PP_DEFINED);
		break;
	case TK_PP_IFNDEF:
		m_method = MakeMethod<ConditionalStatement>(TK_PP_DEFINED, TK_NOT);
		break;
	case TK_PP_ELIF:
		m_method = MakeMethod<ConditionalStatement>();
		break;
	case TK_PP_PRAGMA:
		m_method = std::make_unique<CompilerDialect>();
		break;
	case TK_PP_LINE:
		m_method = MakeMethod<FixLocation>();
		break;
	case TK_PP_MESSAGE:
		m_method = MakeMethod<LinguisticError>();
		break;
	case TK_PP_WARNING:
		m_method = MakeMethod<LinguisticError>(false);
		break;
	case TK_PP_ERROR:
		m_method = MakeMethod<LinguisticError>();
		break;
	default:
		throw StageBase::StageException{ Name(), "invalid preprocessing directive" };
	}
}

void Preprocessor::Propagate(bool isDirective, DefaultTokenDataPair& tokenData)
{
	assert(tokenData.HasToken() /*&& tokenData.HasData()*/);

	g_tokenSubscription.CallAnyOf(isDirective, tokenData);
}

void Preprocessor::Dispatch(DefaultTokenDataPair& tokenData)
{
	// Call the method factory and store the next method as continuation
	if (!m_method) {
		return MethodFactory(tokenData.Token());
	}

	// If continuation is set, continue on
	m_method->Dispence(tokenData);
}

void Preprocessor::EndOfLine()
{
	// Reset directive method for next preprocessor line.
	if (m_method) {
		m_method->Yield();
		m_method.reset();
	}
}
