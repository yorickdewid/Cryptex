// Copyright (c) 2017 Quenza Inc. All rights reserved.
//
// This file is part of the Cryptox project.
//
// Use of this source code is governed by a private license
// that can be found in the LICENSE file. Content can not be
// copied and/or distributed without the express of the author.

#include "DirectiveScanner.h"

#include <Cry/Algorithm.h>

constexpr char EndOfUnit = '\0';

namespace CoilCl
{

template<typename PreprocessorClass>
class TokenProcessorProxy<PreprocessorClass>::ProfileWrapper : public CoilCl::Profile
{
	std::shared_ptr<Profile>& m_profileOrigin;
	std::function<bool(const std::string&)> m_sourceIncludeCb;

public:
	ProfileWrapper(std::shared_ptr<Profile>& profile)
		: m_profileOrigin{ profile }
	{
	}

	ProfileWrapper(std::shared_ptr<Profile>& profile, std::function<bool(const std::string&)>&& sourceIncludeCb)
		: m_profileOrigin{ profile }
		, m_sourceIncludeCb{ std::move(sourceIncludeCb) }
	{
	}

	[[noreturn]] // TODO: throw something usefull
	virtual std::string ReadInput() { throw 1; };

	// Load source via lexer as opposed to the default frontend loader
	virtual bool Include(const std::string& source)
	{
		return m_sourceIncludeCb(source);
	};

	// Pass meta info request through to original profile
	virtual std::shared_ptr<metainfo_t> MetaInfo()
	{
		return m_profileOrigin->MetaInfo();
	};

	// Pass error handler through to original profile
	virtual void Error(const std::string& message, bool isFatal)
	{
		m_profileOrigin->Error(message, isFatal);
	};

	// Return the original, wrapped, profile object
	std::shared_ptr<Profile> NativeProfile()
	{
		return m_profileOrigin;
	}
};

template<typename PreprocessorClass>
template<typename... ArgTypes>
TokenProcessorProxy<PreprocessorClass>::TokenProcessorProxy(std::shared_ptr<Profile>& profile, CryCC::Program::ConditionTracker::Tracker& tracker, ArgTypes&&... args)
	: m_profile{ std::shared_ptr<Profile>{ new ProfileWrapper{ profile, std::forward<ArgTypes>(args)... } } }
	, tokenProcessor{ m_profile, tracker }
{
}

template<typename PreprocessorClass>
TokenProcessor::TokenDataPair<TokenProcessor::TokenType, const TokenProcessor::DataType> TokenProcessorProxy<PreprocessorClass>::ProcessBacklog()
{
	assert(m_tokenBacklog && !m_tokenBacklog->empty());

	//FUTURE: Look at descending token queues
	auto pair = std::move(m_tokenBacklog->front());
	m_tokenBacklog->pop_front();
	if (m_tokenBacklog->empty()) {
		m_tokenBacklog.reset();
	}

	return pair;
}

//TODO: Too much magic, refactor
template<typename PreprocessorClass>
int TokenProcessorProxy<PreprocessorClass>::operator()(
	std::function<int()> lexerLexCall,
	std::function<bool()> lexerHasDataCall,
	std::function<Tokenizer::ValuePointer()> lexerDataCall,
	std::function<void(const Tokenizer::ValuePointer&)> lexerSetDataCall)
{
	int token = -1;
	bool skipNewline = false;
	bool onPreprocLine = false;

	// Clear the token backlog first
	if (m_tokenBacklog && !m_tokenBacklog->empty()) {
		auto pair = ProcessBacklog();
		lexerSetDataCall(pair.Data());
		return pair.Token();
	}

	do {
		token = lexerLexCall();
		switch (token) {
		case TK_PREPROCESS:
		{
			onPreprocLine = true;
			skipNewline = false;
			continue;
		}

		// Line continuation will ignore newline
		case TK_LINE_CONT:
		{
			skipNewline = skipNewline ? false : true;
			continue;
		}

		// Newline is end of directive line, if no line continuations were found.
		case TK_LINE_NEW:
		{
			if (!skipNewline && onPreprocLine) {
				tokenProcessor.EndOfLine();
				onPreprocLine = false;
			}
			continue;
		}
		}

		// Stop processing when end of source is reached. This is excluded from the switch
		// by desing since the operation needs to break out for the outer loop.
		if (token == TK_HALT) {
			break;
		}

		skipNewline = false;

		//TODO: the optional is a temporary measure
		// If token contains data, get data pointer.
		/*boost::optional<Tokenizer::ValuePointer> data;
		if (lexerHasDataCall()) {
			data = std::move(lexerDataCall());
		}*/

		// Before returning back to the frontend caller process present the token and data to the
		// hooked methods. Since token processors can hook onto any token they are allowed
		// to change the token and/or data before continuing downwards. If the hooked methods reset
		// the token, we skip all further operations and continue on with a new token.
		auto preprocPair = lexerHasDataCall()
			? (TokenProcessor::DefaultTokenDataPair{ token, std::move(lexerDataCall()) })
			: (TokenProcessor::DefaultTokenDataPair{ token });
		tokenProcessor.Propagate(onPreprocLine, preprocPair);

		// If the token processor cleared the token, we must not return and request
		// next token instead. This allows the token processor to skip over tokens.
		if (!preprocPair.HasToken()) { continue; }

		// Replace token if changed.
		if (preprocPair.HasTokenChanged()) {
			token = preprocPair.Token();
		}

		// If the token contains data, and the data pointer was changed, swap data.
		if (preprocPair.HasData() && preprocPair.HasDataChanged()) {
			lexerSetDataCall(preprocPair.Data());
		}

		// If the token processor wants to inject multiple tokens at this position, queue them in the backlog.
		if (preprocPair.HasTokenQueue()) {
			m_tokenBacklog = std::move(preprocPair.TokenQueue());
		}

		// Break for all non token processor items and non subscribed tokens.
		Cry::Algorithm::MatchOn<decltype(token)> pred{ token };
		if (!onPreprocLine && (m_subscribedTokens.empty() || !std::any_of(m_subscribedTokens.cbegin(), m_subscribedTokens.cend(), pred))) {
			break;
		}

		// Call token processor if any of the token conditions was met.
		auto dispatchPair = lexerHasDataCall()
			? (TokenProcessor::DefaultTokenDataPair{ token, std::move(lexerDataCall()) })
			: (TokenProcessor::DefaultTokenDataPair{ token });
		tokenProcessor.Dispatch(dispatchPair);
	} while (true);

	return token;
}

int DirectiveScanner::PreprocessLexSet(char lexChar)
{
	switch (lexChar) {
	case '#':
		Next();
		return AssembleToken(TK_PREPROCESS);

	case '\\':
		Next();
		return AssembleToken(TK_LINE_CONT);

	case '\n':
		Next();
		return AssembleToken(TK_LINE_NEW);
	}

	return CONTINUE_NEXT_TOKEN;
}

int DirectiveScanner::LexWrapper()
{
	m_data.reset();
	auto& context = m_context.top();
	context.m_lastTokenLine = context.m_currentLine;
	while (context.m_currentChar != EndOfUnit) {
		int token = PreprocessLexSet(context.m_currentChar);
		if (token == CONTINUE_NEXT_TOKEN) {
			token = Lexer::DefaultLexSet(context.m_currentChar);
			if (token == CONTINUE_NEXT_TOKEN) { continue; }
		}

		return token;
	}

	// Halt if end of unit is reached.
	return TK_HALT;
}

int DirectiveScanner::Lex()
{
	// Setup proxy between directive scanner and token processor.
	return m_proxy([this]() { return this->LexWrapper(); },
		[this]() { return this->HasData(); },
		[this]() { return m_data.get(); },
		[this](const Tokenizer::ValuePointer& dataPtr)
	{
		m_data = boost::none;
		m_data = dataPtr;
	});
}

DirectiveScanner::DirectiveScanner(std::shared_ptr<Profile>& profile, CryCC::Program::ConditionTracker::Tracker& tracker)
	: Lexer{ profile }
, m_proxy{ profile, tracker,  [this](const std::string& source) -> bool { this->SwapSource(source); return true; /*TODO: Unmock*/ } }
{
	AddKeyword("include", TK_PP_INCLUDE);
	AddKeyword("define", TK_PP_DEFINE);
	AddKeyword("defined", TK_PP_DEFINED);
	AddKeyword("undef", TK_PP_UNDEF);
	AddKeyword("ifdef", TK_PP_IFDEF);
	AddKeyword("ifndef", TK_PP_IFNDEF);
	AddKeyword("elif", TK_PP_ELIF);
	AddKeyword("endif", TK_PP_ENDIF);
	AddKeyword("pragma", TK_PP_PRAGMA);
	AddKeyword("line", TK_PP_LINE);
	AddKeyword("warning", TK_PP_WARNING);
	AddKeyword("error", TK_PP_ERROR);
}

} // namespace CoilCl
