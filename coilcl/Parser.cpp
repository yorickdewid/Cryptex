#include "Parser.h"

#include <iostream>
#include <exception>
#include <sstream>

#define CURRENT_DATA() m_comm.Current().FetchData()
#define PREVIOUS_TOKEN() m_comm.Previous().FetchToken()
#define CURRENT_TOKEN() m_comm.Current().FetchToken()
#define MATCH_TOKEN(t) (CURRENT_TOKEN() == t)
#define NOT_TOKEN(t) (CURRENT_TOKEN() != t)

#define AST_ROOT() m_ast

//TODO: remove EMIT helpers
#define EMIT(m) std::cout << "EMIT::" << m << std::endl;
#define EMIT_IDENTIFIER() std::cout << "EMIT::IDENTIFIER" << "("<< CURRENT_DATA()->As<std::string>() << ")" << std::endl;

#define MAKE_RESV_REF() std::make_shared<DeclRefExpr>(m_identifierStack.top()); m_identifierStack.pop();
#define MAKE_BUILTIN_FUNC(n) std::make_shared<BuiltinExpr>(std::make_shared<DeclRefExpr>(n));

class UnexpectedTokenException : public std::exception
{
public:
	UnexpectedTokenException() noexcept
	{
	}

	explicit UnexpectedTokenException(char const* const message, int line, int column, Token token) noexcept
		: m_line{ line }
		, m_column{ column }
	{
		std::stringstream ss;
		ss << "Semantic error: " << message;
		ss << " before " << "'" + Keyword{ token }.Print() + "'" << " token at "; //TODO: token
		ss << line << ':' << column;
		_msg = ss.str();
	}

	virtual int Line() const noexcept
	{
		return m_line;
	}

	virtual int Column() const noexcept
	{
		return m_column;
	}

	virtual const char *what() const noexcept
	{
		return _msg.c_str();
	}

protected:
	std::string _msg;

private:
	int m_line;
	int m_column;
};

class SyntaxException : public std::exception
{
public:
	SyntaxException() noexcept
	{
	}

	explicit SyntaxException(char const* const message, char token, int line, int column) noexcept
		: m_token{ token }
		, m_line{ line }
		, m_column{ column }
	{
		std::stringstream ss;
		ss << "Syntax error: " << message << " at ";
		ss << line << ':' << column;
		_msg = ss.str();
	}

	virtual int Line() const noexcept
	{
		return m_line;
	}

	virtual int Column() const noexcept
	{
		return m_column;
	}

	virtual const char *what() const noexcept
	{
		return _msg.c_str();
	}

protected:
	std::string _msg;

private:
	char m_token;
	int m_line;
	int m_column;
};

class ParseException : public std::exception
{
public:
	ParseException() noexcept
	{
	}

	explicit ParseException(char const* const message, int line, int column) noexcept
		: m_line{ line }
		, m_column{ column }
	{
		std::stringstream ss;
		ss << "Parser error: " << message;
		ss << " " << line << ':' << column;
		_msg = ss.str();
	}

	virtual int Line() const noexcept
	{
		return m_line;
	}

	virtual int Column() const noexcept
	{
		return m_column;
	}

	virtual const char *what() const noexcept
	{
		return _msg.c_str();
	}

protected:
	std::string _msg;

private:
	int m_line;
	int m_column;
};

Parser::Parser(std::shared_ptr<Compiler::Profile>& profile)
	: m_profile{ profile }
	, lex{ profile }
{
	lex.ErrorHandler([](const std::string& err, char token, int line, int column)
	{
		throw SyntaxException(err.c_str(), token, line, column);
	});
}

Parser& Parser::CheckCompatibility()
{
	//TODO: check profile options here
	return (*this);
}

void Parser::Error(const char* err, Token token)
{
	throw UnexpectedTokenException{ err, m_comm.Current().FetchLine(), m_comm.Current().FetchColumn(), token };
}

void Parser::ExpectToken(Token token)
{
	if (NOT_TOKEN(token)) {
		Error("expected expression", token);
	}

	NextToken();
}

//TODO: remove ?
void Parser::ExpectIdentifier()
{
	if (NOT_TOKEN(TK_IDENTIFIER)) {
		Error("expected identifier", TK_IDENTIFIER);
	}

	assert(m_comm.Current().HasData());

	NextToken();
}

auto Parser::StorageClassSpecifier()
{
	switch (CURRENT_TOKEN()) {
	case TK_EXTERN:
		return Value::StorageClassSpecifier::EXTERN;
	case TK_REGISTER:
		return Value::StorageClassSpecifier::REGISTER;
	case TK_STATIC:
		return Value::StorageClassSpecifier::STATIC;
	case TK_AUTO:
		return Value::StorageClassSpecifier::AUTO;
	case TK_TYPEDEF:
		return Value::StorageClassSpecifier::TYPEDEF;
	}

	return Value::StorageClassSpecifier::NONE;
}

std::unique_ptr<Value> Parser::TypeSpecifier()
{
	// TODO:
	// - TYPE_NAME
	switch (CURRENT_TOKEN()) {
	case TK_VOID:
		return std::move(std::make_unique<ValueObject<void>>(Value::TypeSpecifier::T_VOID));
	case TK_CHAR:
		return std::move(std::make_unique<ValueObject<std::string>>(Value::TypeSpecifier::T_CHAR));
	case TK_SHORT:
		return std::move(std::make_unique<ValueObject<short>>(Value::TypeSpecifier::T_SHORT));
	case TK_INT:
		return std::move(std::make_unique<ValueObject<int>>(Value::TypeSpecifier::T_INT));
	case TK_LONG:
		return std::move(std::make_unique<ValueObject<long>>(Value::TypeSpecifier::T_LONG));
	case TK_FLOAT:
		return std::move(std::make_unique<ValueObject<float>>(Value::TypeSpecifier::T_FLOAT));
	case TK_DOUBLE:
		return std::move(std::make_unique<ValueObject<double>>(Value::TypeSpecifier::T_DOUBLE));
	case TK_SIGNED:
		return std::move(std::make_unique<ValueObject<signed>>(Value::TypeSpecifier::T_INT));
	case TK_UNSIGNED:
		return std::move(std::make_unique<ValueObject<unsigned>>(Value::TypeSpecifier::T_INT));
	case TK_BOOL:
		return std::move(std::make_unique<ValueObject<bool>>(Value::TypeSpecifier::T_BOOL));
	case TK_COMPLEX:
		break;
	case TK_IMAGINARY:
		break;
	}

	if (EnumSpecifier()) {
		//return something
	}
	if (StructOrUnionSpecifier()) {
		//return something
	}

	return nullptr;
}

auto Parser::TypeQualifier()
{
	switch (CURRENT_TOKEN()) {
	case TK_CONST:
		return Value::TypeQualifier::CONST;
	case TK_VOLATILE:
		return Value::TypeQualifier::VOLATILE;
	}

	return Value::TypeQualifier::NONE;
}

bool Parser::DeclarationSpecifiers()
{
	std::shared_ptr<Value> tmpType = nullptr;
	Value::StorageClassSpecifier tmpSCP = Value::StorageClassSpecifier::NONE;
	Value::TypeQualifier tmpTQ = Value::TypeQualifier::NONE;
	auto isInline = false;

	bool cont = true;
	while (cont) {
		cont = false;

		auto sc = StorageClassSpecifier();
		if (static_cast<int>(sc)) {
			tmpSCP = sc;
			NextToken();
			cont = true;
		}

		auto type = TypeSpecifier();
		if (type != nullptr) {
			NextToken();
			cont = true;
			tmpType = std::move(type);
		}

		auto tq = TypeQualifier();
		if (static_cast<int>(tq)) {
			tmpTQ = tq;
			NextToken();
			cont = true;
		}

		// Function inline specifier
		if (MATCH_TOKEN(TK_INLINE)) {
			NextToken();
			isInline = true;
			cont = true;
		}
	}

	if (tmpType == nullptr) {
		return false;
	}

	if (tmpSCP != Value::StorageClassSpecifier::NONE) {
		tmpType->StorageClass(tmpSCP);
	}
	if (tmpTQ != Value::TypeQualifier::NONE) {
		tmpType->Qualifier(tmpTQ);
	}
	if (isInline) {
		tmpType->SetInline();
	}

	//m_specifiekStack.push(std::make_unique<ValueNode>(type));
	return true;
}

bool Parser::StructOrUnionSpecifier()
{
	bool isUnion = false;

	switch (CURRENT_TOKEN()) {
	case TK_STRUCT:
		NextToken();
		break;
	case TK_UNION:
		NextToken();
		isUnion = true;
		break;
	default:
		return false;
	}

	auto rec = std::make_shared<RecordDecl>(isUnion ? RecordDecl::RecordType::UNION : RecordDecl::RecordType::STRUCT);

	if (MATCH_TOKEN(TK_IDENTIFIER)) {
		EMIT_IDENTIFIER();
		rec->SetName(CURRENT_DATA()->As<std::string>());
		NextToken();
	}

	if (MATCH_TOKEN(TK_BRACE_OPEN)) {
		NextToken();

		do {
			SpecifierQualifierList();

			for (;;) {
				Declarator();

				auto decl = m_identifierStack.top();
				m_identifierStack.pop();
				auto field = std::make_shared<FieldDecl>(decl);

				if (MATCH_TOKEN(TK_COLON)) {
					NextToken();
					ConstantExpression();
					field->SetBitField(std::dynamic_pointer_cast<IntegerLiteral>(m_elementDescentPipe.next()));
					m_elementDescentPipe.pop();
				}

				rec->AddField(field);

				if (NOT_TOKEN(TK_COMMA)) {
					break;
				}

				NextToken();
			}

			ExpectToken(TK_COMMIT);
		} while (NOT_TOKEN(TK_BRACE_CLOSE));
		ExpectToken(TK_BRACE_CLOSE);
	}

	m_elementDescentPipe.push(rec);
	m_elementDescentPipe.lock();
	return true;
}

void Parser::SpecifierQualifierList()
{
	bool cont = false;
	do {
		cont = false;
		auto type = TypeSpecifier();
		if (type != nullptr) {
			NextToken();
			cont = true;
		}

		auto tq = TypeQualifier();
		if (tq != Value::TypeQualifier::NONE) {
			cont = true;
		}
	} while (cont);
}

bool Parser::EnumSpecifier()
{
	if (MATCH_TOKEN(TK_ENUM)) {
		NextToken();

		auto enm = std::make_shared<EnumDecl>();

		if (MATCH_TOKEN(TK_IDENTIFIER)) {
			EMIT_IDENTIFIER();
			enm->SetName(CURRENT_DATA()->As<std::string>());
			NextToken();
		}

		if (MATCH_TOKEN(TK_BRACE_OPEN)) {
			for (;;) {
				NextToken();

				if (MATCH_TOKEN(TK_IDENTIFIER)) {
					EMIT_IDENTIFIER();

					auto enmConst = std::make_shared<EnumConstantDecl>(CURRENT_DATA()->As<std::string>());

					NextToken();
					if (MATCH_TOKEN(TK_ASSIGN)) {
						NextToken();
						ConstantExpression();
						enmConst->SetAssignment(m_elementDescentPipe.next());
						m_elementDescentPipe.pop();
					}

					enm->AddConstant(enmConst);
				}

				if (NOT_TOKEN(TK_COMMA)) {
					break;
				}
			}
			ExpectToken(TK_BRACE_CLOSE);
		}

		m_elementDescentPipe.push(enm);
		m_elementDescentPipe.lock();
		return true;
	}

	return false;
}

bool Parser::UnaryOperator()
{
	switch (CURRENT_TOKEN()) {
	case TK_AMPERSAND:
	{
		NextToken();
		CastExpression();

		auto resv = MAKE_RESV_REF();
		auto unaryOp = std::make_shared<CoilCl::AST::UnaryOperator>(CoilCl::AST::UnaryOperator::UnaryOperator::ADDR, CoilCl::AST::UnaryOperator::OperandSide::PREFIX, std::dynamic_pointer_cast<ASTNode>(resv));
		m_elementDescentPipe.push(unaryOp);
		break;
	}
	case TK_ASTERISK:
	{
		NextToken();
		CastExpression();

		auto resv = MAKE_RESV_REF();
		auto unaryOp = std::make_shared<CoilCl::AST::UnaryOperator>(CoilCl::AST::UnaryOperator::UnaryOperator::PTRVAL, CoilCl::AST::UnaryOperator::OperandSide::PREFIX, std::dynamic_pointer_cast<ASTNode>(resv));
		m_elementDescentPipe.push(unaryOp);
		break;
	}
	case TK_PLUS:
	{
		NextToken();
		CastExpression();

		auto resv = MAKE_RESV_REF();
		auto unaryOp = std::make_shared<CoilCl::AST::UnaryOperator>(CoilCl::AST::UnaryOperator::UnaryOperator::INTPOS, CoilCl::AST::UnaryOperator::OperandSide::PREFIX, std::dynamic_pointer_cast<ASTNode>(resv));
		m_elementDescentPipe.push(unaryOp);
		break;
	}
	case TK_MINUS:
	{
		NextToken();
		CastExpression();

		auto resv = m_elementDescentPipe.next();
		m_elementDescentPipe.pop();
		auto unaryOp = std::make_shared<CoilCl::AST::UnaryOperator>(CoilCl::AST::UnaryOperator::UnaryOperator::INTNEG, CoilCl::AST::UnaryOperator::OperandSide::PREFIX, resv);
		m_elementDescentPipe.push(unaryOp);
		break;
	}
	case TK_TILDE:
	{
		NextToken();
		CastExpression();

		auto resv = MAKE_RESV_REF();
		auto unaryOp = std::make_shared<CoilCl::AST::UnaryOperator>(CoilCl::AST::UnaryOperator::UnaryOperator::BITNOT, CoilCl::AST::UnaryOperator::OperandSide::PREFIX, std::dynamic_pointer_cast<ASTNode>(resv));
		m_elementDescentPipe.push(unaryOp);
		break;
	}
	case TK_NOT:
	{
		NextToken();
		CastExpression();

		auto resv = MAKE_RESV_REF();
		auto unaryOp = std::make_shared<CoilCl::AST::UnaryOperator>(CoilCl::AST::UnaryOperator::UnaryOperator::BOOLNOT, CoilCl::AST::UnaryOperator::OperandSide::PREFIX, std::dynamic_pointer_cast<ASTNode>(resv));
		m_elementDescentPipe.push(unaryOp);
		break;
	}
	default:
		return false;
	}

	return true;
}

bool Parser::AssignmentOperator()
{
	switch (CURRENT_TOKEN()) {
	case TK_ASSIGN:
	{
		auto binOp = std::make_shared<BinaryOperator>(BinaryOperator::BinOperand::ASSGN, m_elementDescentPipe.next());
		m_elementDescentPipe.pop();

		NextToken();
		AssignmentExpression();

		binOp->SetRightSide(m_elementDescentPipe.next());
		m_elementDescentPipe.pop();
		m_elementDescentPipe.push(binOp);
		break;
	}
	case TK_MUL_ASSIGN:
	{
		auto resv = std::dynamic_pointer_cast<DeclRefExpr>(m_elementDescentPipe.next());
		auto comOp = std::make_shared<CompoundAssignOperator>(CompoundAssignOperator::CompoundAssignOperand::MUL, resv);
		m_elementDescentPipe.pop();

		NextToken();
		AssignmentExpression();

		comOp->SetRightSide(m_elementDescentPipe.next());
		m_elementDescentPipe.pop();
		m_elementDescentPipe.push(comOp);
		break;
	}
	case TK_DIV_ASSIGN:
	{
		auto resv = std::dynamic_pointer_cast<DeclRefExpr>(m_elementDescentPipe.next());
		auto comOp = std::make_shared<CompoundAssignOperator>(CompoundAssignOperator::CompoundAssignOperand::DIV, resv);
		m_elementDescentPipe.pop();

		NextToken();
		AssignmentExpression();

		comOp->SetRightSide(m_elementDescentPipe.next());
		m_elementDescentPipe.pop();
		m_elementDescentPipe.push(comOp);
		break;
	}
	case TK_MOD_ASSIGN:
	{
		auto resv = std::dynamic_pointer_cast<DeclRefExpr>(m_elementDescentPipe.next());
		auto comOp = std::make_shared<CompoundAssignOperator>(CompoundAssignOperator::CompoundAssignOperand::MOD, resv);
		m_elementDescentPipe.pop();

		NextToken();
		AssignmentExpression();

		comOp->SetRightSide(m_elementDescentPipe.next());
		m_elementDescentPipe.pop();
		m_elementDescentPipe.push(comOp);
		break;
	}
	case TK_ADD_ASSIGN:
	{
		auto resv = std::dynamic_pointer_cast<DeclRefExpr>(m_elementDescentPipe.next());
		auto comOp = std::make_shared<CompoundAssignOperator>(CompoundAssignOperator::CompoundAssignOperand::ADD, resv);
		m_elementDescentPipe.pop();

		NextToken();
		AssignmentExpression();

		comOp->SetRightSide(m_elementDescentPipe.next());
		m_elementDescentPipe.pop();
		m_elementDescentPipe.push(comOp);
		break;
	}
	case TK_SUB_ASSIGN:
	{
		auto resv = std::dynamic_pointer_cast<DeclRefExpr>(m_elementDescentPipe.next());
		auto comOp = std::make_shared<CompoundAssignOperator>(CompoundAssignOperator::CompoundAssignOperand::SUB, resv);
		m_elementDescentPipe.pop();

		NextToken();
		AssignmentExpression();

		comOp->SetRightSide(m_elementDescentPipe.next());
		m_elementDescentPipe.pop();
		m_elementDescentPipe.push(comOp);
		break;
	}
	case TK_LEFT_ASSIGN:
	{
		auto resv = std::dynamic_pointer_cast<DeclRefExpr>(m_elementDescentPipe.next());
		auto comOp = std::make_shared<CompoundAssignOperator>(CompoundAssignOperator::CompoundAssignOperand::LEFT, resv);
		m_elementDescentPipe.pop();

		m_elementDescentPipe.pop();
		NextToken();
		AssignmentExpression();

		comOp->SetRightSide(m_elementDescentPipe.next());
		m_elementDescentPipe.pop();
		m_elementDescentPipe.push(comOp);
		break;
	}
	case TK_RIGHT_ASSIGN:
	{
		auto resv = std::dynamic_pointer_cast<DeclRefExpr>(m_elementDescentPipe.next());
		auto comOp = std::make_shared<CompoundAssignOperator>(CompoundAssignOperator::CompoundAssignOperand::RIGHT, resv);
		m_elementDescentPipe.pop();

		NextToken();
		AssignmentExpression();

		comOp->SetRightSide(m_elementDescentPipe.next());
		m_elementDescentPipe.pop();
		m_elementDescentPipe.push(comOp);
		break;
	}
	case TK_AND_ASSIGN:
	{
		auto resv = std::dynamic_pointer_cast<DeclRefExpr>(m_elementDescentPipe.next());
		auto comOp = std::make_shared<CompoundAssignOperator>(CompoundAssignOperator::CompoundAssignOperand::AND, resv);
		m_elementDescentPipe.pop();

		NextToken();
		AssignmentExpression();

		comOp->SetRightSide(m_elementDescentPipe.next());
		m_elementDescentPipe.pop();
		m_elementDescentPipe.push(comOp);
		break;
	}
	case TK_XOR_ASSIGN:
	{
		auto resv = std::dynamic_pointer_cast<DeclRefExpr>(m_elementDescentPipe.next());
		auto comOp = std::make_shared<CompoundAssignOperator>(CompoundAssignOperator::CompoundAssignOperand::XOR, resv);
		m_elementDescentPipe.pop();

		NextToken();
		AssignmentExpression();

		comOp->SetRightSide(m_elementDescentPipe.next());
		m_elementDescentPipe.pop();
		m_elementDescentPipe.push(comOp);
		break;
	}
	case TK_OR_ASSIGN:
	{
		auto resv = std::dynamic_pointer_cast<DeclRefExpr>(m_elementDescentPipe.next());
		auto comOp = std::make_shared<CompoundAssignOperator>(CompoundAssignOperator::CompoundAssignOperand::OR, resv);
		m_elementDescentPipe.pop();

		NextToken();
		AssignmentExpression();

		comOp->SetRightSide(m_elementDescentPipe.next());
		m_elementDescentPipe.pop();
		m_elementDescentPipe.push(comOp);
		break;
	}
	default:
		return false;
	}

	return true;
}

void Parser::PrimaryExpression()
{
	switch (CURRENT_TOKEN()) {
	case TK_IDENTIFIER:
		EMIT_IDENTIFIER();
		m_identifierStack.push(CURRENT_DATA()->As<std::string>());
		NextToken();
		break;

	case TK_CONSTANT:
		switch (CURRENT_DATA()->DataType()) {
		case Value::TypeSpecifier::T_INT:
		{
			auto object = std::dynamic_pointer_cast<ValueObject<int>>(CURRENT_DATA());
			m_elementDescentPipe.push(std::make_shared<IntegerLiteral>(std::move(object)));
			break;
		}
		case Value::TypeSpecifier::T_DOUBLE:
		{
			auto object = std::dynamic_pointer_cast<ValueObject<double>>(CURRENT_DATA());
			m_elementDescentPipe.push(std::make_shared<FloatingLiteral>(std::move(object)));
			break;
		}
		case Value::TypeSpecifier::T_CHAR:
		{
			if (CURRENT_DATA()->IsArray()) {
				auto object = std::dynamic_pointer_cast<ValueObject<std::string>>(CURRENT_DATA());
				m_elementDescentPipe.push(std::make_shared<StringLiteral>(std::move(object)));
			}
			else {
				auto object = std::dynamic_pointer_cast<ValueObject<char>>(CURRENT_DATA());
				m_elementDescentPipe.push(std::make_shared<CharacterLiteral>(std::move(object)));
			}
			break;
		}
		}
		NextToken();
		break;

	case TK_PARENTHESE_OPEN:
		NextToken();
		Expression();

		auto parenthesis = std::make_shared<ParenExpr>(m_elementDescentPipe.next());
		m_elementDescentPipe.pop();
		m_elementDescentPipe.push(parenthesis);
		ExpectToken(TK_PARENTHESE_CLOSE);
	}
}

void Parser::ArgumentExpressionList()
{
	for (;;) {
		auto itemState = m_elementDescentPipe.state();
		AssignmentExpression();
		if (m_elementDescentPipe.is_changed(itemState)) {
			m_elementDescentPipe.lock();
		}

		if (NOT_TOKEN(TK_COMMA)) {
			break;
		}

		NextToken();
	}
}

void Parser::PostfixExpression()
{
	if (MATCH_TOKEN(TK_PARENTHESE_OPEN)) {
		// Snapshot current state in case of rollback
		m_comm.Snapshot();
		try {
			NextToken();
			TypeName();
			ExpectToken(TK_PARENTHESE_CLOSE);
			ExpectToken(TK_BRACE_OPEN);

			// Remove snapshot since we can continue this path
			m_comm.DisposeSnapshot();

			auto list = std::make_shared<InitListExpr>();

			for (;;) {
				Initializer();

				list->AddListItem(m_elementDescentPipe.next());
				m_elementDescentPipe.pop();

				if (NOT_TOKEN(TK_COMMA)) {
					break;
				}

				NextToken();
			}
			ExpectToken(TK_BRACE_CLOSE);

			m_elementDescentPipe.push(std::make_shared<CompoundLiteralExpr>(list));
		}

		// Cannot cast, rollback the command state
		catch (const UnexpectedTokenException&) {
			m_comm.Revert();
		}
	}

	auto startSz = m_identifierStack.size();

	PrimaryExpression();

	switch (CURRENT_TOKEN()) {
	case TK_BRACKET_OPEN:
	{
		NextToken();

		auto resv = MAKE_RESV_REF();

		Expression();
		ExpectToken(TK_BRACKET_CLOSE);

		auto arrsub = std::make_shared<ArraySubscriptExpr>(resv, m_elementDescentPipe.next());
		m_elementDescentPipe.pop();
		m_elementDescentPipe.push(arrsub);
		break;
	}
	case TK_PARENTHESE_OPEN:
	{
		NextToken();

		auto resv = MAKE_RESV_REF();

		if (MATCH_TOKEN(TK_PARENTHESE_CLOSE)) {
			NextToken();
			m_elementDescentPipe.push(std::make_shared<CallExpr>(resv));
		}
		else {
			auto startState = m_elementDescentPipe.state();
			ArgumentExpressionList();
			m_elementDescentPipe.release_until(startState);
			ExpectToken(TK_PARENTHESE_CLOSE);

			auto arg = std::make_shared<ArgumentStmt>();
			while (!m_elementDescentPipe.empty()) {
				arg->AppendArgument(m_elementDescentPipe.next());
				m_elementDescentPipe.pop();
			}

			m_elementDescentPipe.push(std::make_shared<CallExpr>(resv, arg));
		}
		break;
	}
	case TK_DOT: //TODO
	{
		NextToken();
		ExpectIdentifier();

		/*
		auto resv = MAKE_RESV_REF();
		auto memr = std::make_shared<MemberExpr>(resv);
		m_elementDescentPipe.push(memr);
		*/

		break;
	}
	case TK_PTR_OP:  //TODO
	{
		NextToken();
		ExpectIdentifier();

		/*
		*/

		break;
	}
	case TK_INC_OP:
	{
		if (m_identifierStack.size() == startSz) {
			throw ParseException{ "expression is not assignable", 0, 0 };
		}

		auto resv = MAKE_RESV_REF();
		auto unaryOp = std::make_shared<CoilCl::AST::UnaryOperator>(CoilCl::AST::UnaryOperator::UnaryOperator::INC, CoilCl::AST::UnaryOperator::OperandSide::POSTFIX, std::dynamic_pointer_cast<ASTNode>(resv));
		m_elementDescentPipe.push(unaryOp);

		NextToken();
		break;
	}
	case TK_DEC_OP:
	{
		if (m_identifierStack.size() == startSz) {
			throw ParseException{ "expression is not assignable", 0, 0 };
		}

		auto resv = MAKE_RESV_REF();
		auto unaryOp = std::make_shared<CoilCl::AST::UnaryOperator>(CoilCl::AST::UnaryOperator::UnaryOperator::DEC, CoilCl::AST::UnaryOperator::OperandSide::POSTFIX, std::dynamic_pointer_cast<ASTNode>(resv));
		m_elementDescentPipe.push(unaryOp);

		NextToken();
		break;
	}
	default:
		if (m_identifierStack.size() > startSz) {
			auto resv = MAKE_RESV_REF();
			m_elementDescentPipe.push(resv);
		}
	}
}

void Parser::UnaryExpression()
{
	auto startSz = m_identifierStack.size();

	switch (CURRENT_TOKEN()) {
	case TK_INC_OP:
	{
		NextToken();
		UnaryExpression();

		/*if (m_identifierStack.size() != startSz) {
			throw ParseException{ "expression is not assignable", 0, 0 };
		}*/

		auto unaryOp = std::make_shared<CoilCl::AST::UnaryOperator>(CoilCl::AST::UnaryOperator::UnaryOperator::INC, CoilCl::AST::UnaryOperator::OperandSide::PREFIX, m_elementDescentPipe.next());
		m_elementDescentPipe.pop();
		m_elementDescentPipe.push(unaryOp);

		break;
	}
	case TK_DEC_OP:
	{
		NextToken();
		UnaryExpression();

		/*if (m_identifierStack.size() != startSz) {
			throw ParseException{ "expression is not assignable", 0, 0 };
		}*/

		auto unaryOp = std::make_shared<CoilCl::AST::UnaryOperator>(CoilCl::AST::UnaryOperator::UnaryOperator::DEC, CoilCl::AST::UnaryOperator::OperandSide::PREFIX, m_elementDescentPipe.next());
		m_elementDescentPipe.pop();
		m_elementDescentPipe.push(unaryOp);

		break;
	}
	case TK_SIZEOF:
	{
		NextToken();
		auto func = MAKE_BUILTIN_FUNC("sizeof");

		// Snapshot current state in case of rollback
		m_comm.Snapshot();
		try {
			if (MATCH_TOKEN(TK_PARENTHESE_OPEN)) {
				NextToken();
				TypeName();
				ExpectToken(TK_PARENTHESE_CLOSE);

				// Remove snapshot since we can continue this path
				m_comm.DisposeSnapshot();
				m_elementDescentPipe.push(func);
				break;
			}
		}
		// No typename, rollback the command state
		catch (const UnexpectedTokenException&) {
			m_comm.Revert();
		}

		UnaryExpression();

		func->SetExpression(m_elementDescentPipe.next());
		m_elementDescentPipe.pop();
		m_elementDescentPipe.push(func);
		break;
	}
	default:
		if (!UnaryOperator()) {
			PostfixExpression();
		}
	}
}

void Parser::CastExpression()
{
	if (MATCH_TOKEN(TK_PARENTHESE_OPEN)) {
		// Snapshot current state in case of rollback
		m_comm.Snapshot();
		try {
			NextToken();
			TypeName();
			ExpectToken(TK_PARENTHESE_CLOSE);

			CastExpression();

			// If there are no element on the queue, no cast was found
			if (m_elementDescentPipe.empty()) {
				throw UnexpectedTokenException{};
			}

			// Remove snapshot since we can continue this path
			m_comm.DisposeSnapshot();

			auto cast = std::make_shared<CastExpr>(m_elementDescentPipe.next());
			m_elementDescentPipe.pop();
			m_elementDescentPipe.push(cast);
			return;
		}
		// Cannot cast, rollback the command state
		catch (const UnexpectedTokenException&) {
			m_comm.Revert();
		}
	}

	UnaryExpression();
}

void Parser::MultiplicativeExpression()
{
	CastExpression();

	switch (CURRENT_TOKEN()) {
	case TK_ASTERISK:
	{
		auto binOp = std::make_shared<BinaryOperator>(BinaryOperator::BinOperand::MUL, m_elementDescentPipe.next());
		m_elementDescentPipe.pop();

		NextToken();
		CastExpression();

		binOp->SetRightSide(m_elementDescentPipe.next());
		m_elementDescentPipe.pop();
		m_elementDescentPipe.push(binOp);
		break;
	}
	case TK_SLASH:
	{
		auto binOp = std::make_shared<BinaryOperator>(BinaryOperator::BinOperand::DIV, m_elementDescentPipe.next());
		m_elementDescentPipe.pop();

		NextToken();
		CastExpression();

		binOp->SetRightSide(m_elementDescentPipe.next());
		m_elementDescentPipe.pop();
		m_elementDescentPipe.push(binOp);
		break;
	}
	case TK_PERCENT:
	{
		auto binOp = std::make_shared<BinaryOperator>(BinaryOperator::BinOperand::MOD, m_elementDescentPipe.next());
		m_elementDescentPipe.pop();

		NextToken();
		CastExpression();

		binOp->SetRightSide(m_elementDescentPipe.next());
		m_elementDescentPipe.pop();
		m_elementDescentPipe.push(binOp);
		break;
	}
	}
}

void Parser::AdditiveExpression()
{
	MultiplicativeExpression();

	switch (CURRENT_TOKEN()) {
	case TK_PLUS:
	{
		auto binOp = std::make_shared<BinaryOperator>(BinaryOperator::BinOperand::PLUS, m_elementDescentPipe.next());
		m_elementDescentPipe.pop();

		NextToken();
		MultiplicativeExpression();

		binOp->SetRightSide(m_elementDescentPipe.next());
		m_elementDescentPipe.pop();
		m_elementDescentPipe.push(binOp);
		break;
	}
	case TK_MINUS:
	{
		auto binOp = std::make_shared<BinaryOperator>(BinaryOperator::BinOperand::MINUS, m_elementDescentPipe.next());
		m_elementDescentPipe.pop();

		NextToken();
		MultiplicativeExpression();

		binOp->SetRightSide(m_elementDescentPipe.next());
		m_elementDescentPipe.pop();
		m_elementDescentPipe.push(binOp);
		break;
	}
	}
}

void Parser::ShiftExpression()
{
	AdditiveExpression();

	switch (CURRENT_TOKEN()) {
	case TK_LEFT_OP:
	{
		auto binOp = std::make_shared<BinaryOperator>(BinaryOperator::BinOperand::SLEFT, m_elementDescentPipe.next());
		m_elementDescentPipe.pop();

		NextToken();
		AdditiveExpression();

		binOp->SetRightSide(m_elementDescentPipe.next());
		m_elementDescentPipe.pop();
		m_elementDescentPipe.push(binOp);
		break;
	}
	case TK_RIGHT_OP:
	{
		auto binOp = std::make_shared<BinaryOperator>(BinaryOperator::BinOperand::SRIGHT, m_elementDescentPipe.next());
		m_elementDescentPipe.pop();

		NextToken();
		AdditiveExpression();

		binOp->SetRightSide(m_elementDescentPipe.next());
		m_elementDescentPipe.pop();
		m_elementDescentPipe.push(binOp);
		break;
	}
	}
}

void Parser::RelationalExpression()
{
	ShiftExpression();

	switch (CURRENT_TOKEN()) {
	case TK_LESS_THAN:
	{
		auto binOp = std::make_shared<BinaryOperator>(BinaryOperator::BinOperand::LT, m_elementDescentPipe.next());
		m_elementDescentPipe.pop();

		NextToken();
		ShiftExpression();

		binOp->SetRightSide(m_elementDescentPipe.next());
		m_elementDescentPipe.pop();
		m_elementDescentPipe.push(binOp);
		break;
	}
	case TK_GREATER_THAN:
	{
		auto binOp = std::make_shared<BinaryOperator>(BinaryOperator::BinOperand::GT, m_elementDescentPipe.next());
		m_elementDescentPipe.pop();

		NextToken();
		ShiftExpression();

		binOp->SetRightSide(m_elementDescentPipe.next());
		m_elementDescentPipe.pop();
		m_elementDescentPipe.push(binOp);
		break;
	}
	case TK_LE_OP:
	{
		auto binOp = std::make_shared<BinaryOperator>(BinaryOperator::BinOperand::LE, m_elementDescentPipe.next());
		m_elementDescentPipe.pop();

		NextToken();
		ShiftExpression();

		binOp->SetRightSide(m_elementDescentPipe.next());
		m_elementDescentPipe.pop();
		m_elementDescentPipe.push(binOp);
		break;
	}
	case TK_GE_OP:
	{
		auto binOp = std::make_shared<BinaryOperator>(BinaryOperator::BinOperand::GE, m_elementDescentPipe.next());
		m_elementDescentPipe.pop();

		NextToken();
		ShiftExpression();

		binOp->SetRightSide(m_elementDescentPipe.next());
		m_elementDescentPipe.pop();
		m_elementDescentPipe.push(binOp);
		break;
	}
	}
}

void Parser::EqualityExpression()
{
	RelationalExpression();

	switch (CURRENT_TOKEN()) {
	case TK_EQ_OP:
	{
		auto binOp = std::make_shared<BinaryOperator>(BinaryOperator::BinOperand::EQ, m_elementDescentPipe.next());
		m_elementDescentPipe.pop();

		NextToken();
		RelationalExpression();

		binOp->SetRightSide(m_elementDescentPipe.next());
		m_elementDescentPipe.pop();
		m_elementDescentPipe.push(binOp);
		break;
	}
	case TK_NE_OP:
	{
		auto binOp = std::make_shared<BinaryOperator>(BinaryOperator::BinOperand::NEQ, m_elementDescentPipe.next());
		m_elementDescentPipe.pop();

		NextToken();
		RelationalExpression();

		binOp->SetRightSide(m_elementDescentPipe.next());
		m_elementDescentPipe.pop();
		m_elementDescentPipe.push(binOp);
		break;
	}
	}
}

void Parser::AndExpression()
{
	EqualityExpression();

	if (MATCH_TOKEN(TK_AMPERSAND)) {
		auto binOp = std::make_shared<BinaryOperator>(BinaryOperator::BinOperand::AND, m_elementDescentPipe.next());
		m_elementDescentPipe.pop();

		NextToken();
		EqualityExpression();

		binOp->SetRightSide(m_elementDescentPipe.next());
		m_elementDescentPipe.pop();
		m_elementDescentPipe.push(binOp);
	}
}

void Parser::ExclusiveOrExpression()
{
	AndExpression();

	if (MATCH_TOKEN(TK_CARET)) {
		auto binOp = std::make_shared<BinaryOperator>(BinaryOperator::BinOperand::XOR, m_elementDescentPipe.next());
		m_elementDescentPipe.pop();

		NextToken();
		AndExpression();

		binOp->SetRightSide(m_elementDescentPipe.next());
		m_elementDescentPipe.pop();
		m_elementDescentPipe.push(binOp);
	}
}

void Parser::LogicalAndExpression()
{
	ExclusiveOrExpression();

	if (MATCH_TOKEN(TK_AND_OP)) {
		auto binOp = std::make_shared<BinaryOperator>(BinaryOperator::BinOperand::LAND, m_elementDescentPipe.next());
		m_elementDescentPipe.pop();

		NextToken();
		ExclusiveOrExpression();

		binOp->SetRightSide(m_elementDescentPipe.next());
		m_elementDescentPipe.pop();
		m_elementDescentPipe.push(binOp);
	}
}

void Parser::LogicalOrExpression()
{
	LogicalAndExpression();

	if (MATCH_TOKEN(TK_OR_OP)) {
		auto binOp = std::make_shared<BinaryOperator>(BinaryOperator::BinOperand::LOR, m_elementDescentPipe.next());
		m_elementDescentPipe.pop();

		NextToken();
		LogicalAndExpression();

		binOp->SetRightSide(m_elementDescentPipe.next());
		m_elementDescentPipe.pop();
		m_elementDescentPipe.push(binOp);
	}
}

void Parser::ConditionalExpression()
{
	LogicalOrExpression();

	if (MATCH_TOKEN(TK_QUESTION_MARK)) {
		auto conOp = std::make_shared<ConditionalOperator>(m_elementDescentPipe.next());
		m_elementDescentPipe.pop();

		NextToken();
		Expression();
		conOp->SetTruthCompound(m_elementDescentPipe.next());
		m_elementDescentPipe.pop();

		ExpectToken(TK_COLON);
		ConditionalExpression();

		conOp->SetAltCompound(m_elementDescentPipe.next());
		m_elementDescentPipe.pop();
		m_elementDescentPipe.push(conOp);
	}
}

void Parser::AssignmentExpression()
{
	ConditionalExpression();

	//UnaryExpression(); //XXX UnaryExpression is already run in the ConditionalExpression
	AssignmentOperator();
}

void Parser::Expression()
{
	for (;;) {
		AssignmentExpression();

		if (NOT_TOKEN(TK_COMMA)) {
			break;
		}

		NextToken();
	}
}

void Parser::ConstantExpression()
{
	ConditionalExpression();
}

bool Parser::JumpStatement()
{
	switch (CURRENT_TOKEN()) {
	case TK_GOTO:
		NextToken();

		//ExpectIdentifier();//XXX: possible optimization

		m_elementDescentPipe.push(std::make_shared<GotoStmt>(CURRENT_DATA()->As<std::string>()));
		NextToken();
		break;
	case TK_CONTINUE:
		NextToken();
		EMIT("ITR CONTINUE");
		break;
	case TK_BREAK:
		NextToken();
		m_elementDescentPipe.push(std::make_shared<BreakStmt>());
		break;
	case TK_RETURN:
		NextToken();
		if (MATCH_TOKEN(TK_COMMIT)) {
			m_elementDescentPipe.push(std::make_shared<ReturnStmt>());
		}
		else {
			Expression();

			auto returnStmt = std::make_shared<ReturnStmt>();
			while (!m_elementDescentPipe.empty()) {
				returnStmt->SetReturnNode(m_elementDescentPipe.next());
				m_elementDescentPipe.pop();
			}

			m_elementDescentPipe.push(returnStmt);
		}
		break;
	default:
		return false;
	}

	ExpectToken(TK_COMMIT);
	return true;
}

// For, do and while loop
bool Parser::IterationStatement()
{
	switch (CURRENT_TOKEN()) {
	case TK_WHILE:
	{
		NextToken();
		ExpectToken(TK_PARENTHESE_OPEN);
		Expression();
		auto whlstmt = std::make_shared<WhileStmt>(m_elementDescentPipe.next());
		ExpectToken(TK_PARENTHESE_CLOSE);
		m_elementDescentPipe.pop();

		Statement();

		if (!m_elementDescentPipe.empty()) {
			whlstmt->SetBody(m_elementDescentPipe.next());
			m_elementDescentPipe.pop();
		}

		m_elementDescentPipe.push(whlstmt);
		return true;
	}
	case TK_DO:
	{
		NextToken();
		Statement();

		auto dostmt = std::make_shared<DoStmt>(m_elementDescentPipe.next());
		m_elementDescentPipe.pop();

		ExpectToken(TK_WHILE);
		ExpectToken(TK_PARENTHESE_OPEN);
		Expression();
		ExpectToken(TK_PARENTHESE_CLOSE);
		ExpectToken(TK_COMMIT);

		dostmt->SetEval(m_elementDescentPipe.next());
		m_elementDescentPipe.pop();
		m_elementDescentPipe.push(dostmt);
		return true;
	}
	case TK_FOR:
		NextToken();
		ExpectToken(TK_PARENTHESE_OPEN);

		std::shared_ptr<ASTNode> node1;
		std::shared_ptr<ASTNode> node2;
		std::shared_ptr<ASTNode> node3;

		Declaration();
		if (!m_elementDescentPipe.empty()) {
			node1 = m_elementDescentPipe.next();
			m_elementDescentPipe.pop();

			ExpressionStatement();
			if (!m_elementDescentPipe.empty()) {
				node2 = m_elementDescentPipe.next();
				m_elementDescentPipe.pop();
			}
		}
		else {
			ExpressionStatement();
			if (!m_elementDescentPipe.empty()) {
				node1 = m_elementDescentPipe.next();
				m_elementDescentPipe.pop();
			}

			ExpressionStatement();
			if (!m_elementDescentPipe.empty()) {
				node2 = m_elementDescentPipe.next();
				m_elementDescentPipe.pop();
			}
		}

		Expression();
		if (!m_elementDescentPipe.empty()) {
			node3 = m_elementDescentPipe.next();
			m_elementDescentPipe.pop();
		}

		ExpectToken(TK_PARENTHESE_CLOSE);

		auto forstmt = std::make_shared<ForStmt>(node1, node2, node3);

		Statement();

		if (!m_elementDescentPipe.empty()) {
			forstmt->SetBody(m_elementDescentPipe.next());
			m_elementDescentPipe.pop();
		}

		m_elementDescentPipe.push(forstmt);
		return true;
	}

	return false;
}

// If and switch statements
bool Parser::SelectionStatement()
{
	switch (CURRENT_TOKEN()) {
	case TK_IF:
	{
		NextToken();
		ExpectToken(TK_PARENTHESE_OPEN);
		Expression();
		ExpectToken(TK_PARENTHESE_CLOSE);

		auto ifStmt = std::make_shared<IfStmt>(m_elementDescentPipe.next());
		m_elementDescentPipe.pop();

		Statement();

		// If the statement yields a body, save it and try
		// with alternative compound
		if (m_elementDescentPipe.size() > 0) {
			ifStmt->SetTruthCompound(m_elementDescentPipe.next());
			m_elementDescentPipe.pop();

			if (MATCH_TOKEN(TK_ELSE)) {
				NextToken();
				Statement();
			}
		}

		m_elementDescentPipe.push(ifStmt);
		return true;
	}
	case TK_SWITCH:
	{
		NextToken();
		ExpectToken(TK_PARENTHESE_OPEN);
		Expression();
		ExpectToken(TK_PARENTHESE_CLOSE);

		auto swStmt = std::make_shared<SwitchStmt>(m_elementDescentPipe.next());
		m_elementDescentPipe.pop();

		Statement();

		// If the statement yields a body, save it
		if (m_elementDescentPipe.size() > 0) {
			swStmt->SetBody(m_elementDescentPipe.next());
			m_elementDescentPipe.pop();
		}

		m_elementDescentPipe.push(swStmt);
		return true;
	}
	}

	return false;
}

void Parser::ExpressionStatement()
{
	Expression();

	if (MATCH_TOKEN(TK_COMMIT)) {
		NextToken();
		return;
	}
}

// Compound statements contain code block
bool Parser::CompoundStatement()
{
	if (MATCH_TOKEN(TK_BRACE_OPEN)) {
		NextToken();
		if (MATCH_TOKEN(TK_BRACE_CLOSE)) {
			NextToken();
		}
		else {
			auto startState = m_elementDescentPipe.state();
			BlockItems();
			m_elementDescentPipe.release_until(startState);
			ExpectToken(TK_BRACE_CLOSE);
		}

		// Squash all stack elements in compound statment
		// body and push compound on the stack
		auto compound = std::make_shared<CompoundStmt>();
		while (!m_elementDescentPipe.empty()) {
			compound->AppendChild(m_elementDescentPipe.next());
			m_elementDescentPipe.pop();
		}

		m_elementDescentPipe.push(compound);
		return true;
	}

	return false;
}

// Labeled statements
bool Parser::LabeledStatement()
{
	switch (CURRENT_TOKEN()) {
	case TK_IDENTIFIER:
	{
		// Snapshot current state in case of rollback
		m_comm.Snapshot();
		try {
			auto lblName = CURRENT_DATA()->As<std::string>();
			EMIT_IDENTIFIER();
			NextToken();
			ExpectToken(TK_COLON);

			// Remove snapshot since we can continue this path
			m_comm.DisposeSnapshot();
			Statement();

			if (m_elementDescentPipe.empty()) {
				throw ParseException{ "expected statement", 0, 0 };
			}

			auto lbl = std::make_shared<LabelStmt>(lblName, m_elementDescentPipe.next());
			m_elementDescentPipe.pop();
			m_elementDescentPipe.push(lbl);
		}
		// Not a label, rollback the command state
		catch (const UnexpectedTokenException&) {
			m_comm.Revert();
		}
		break;
	}
	case TK_CASE:
	{
		NextToken();
		ConstantExpression();

		auto lblLiteral = m_elementDescentPipe.next();
		m_elementDescentPipe.pop();

		ExpectToken(TK_COLON);

		Statement();

		if (m_elementDescentPipe.empty()) {
			throw ParseException{ "label at end of compound statement", 0, 0 };
		}

		auto cse = std::make_shared<CaseStmt>(lblLiteral, m_elementDescentPipe.next());
		m_elementDescentPipe.pop();
		m_elementDescentPipe.push(cse);
		return true;
	}
	case TK_DEFAULT:
	{
		NextToken();
		ExpectToken(TK_COLON);

		Statement();

		if (m_elementDescentPipe.empty()) {
			throw ParseException{ "label at end of compound statement", 0, 0 };
		}

		auto dflt = std::make_shared<DefaultStmt>(m_elementDescentPipe.next());
		m_elementDescentPipe.pop();
		m_elementDescentPipe.push(dflt);
		return true;
	}
	}

	return false;
}

void Parser::Statement()
{
	if (CompoundStatement()) { return; }
	if (SelectionStatement()) { return; }
	if (IterationStatement()) { return; }
	if (JumpStatement()) { return; }
	if (LabeledStatement()) { return; }

	// Yield no result, try statement as expression
	ExpressionStatement();
}

void Parser::BlockItems()
{
	do {
		auto itemState = m_elementDescentPipe.state();

		Statement();
		if (MATCH_TOKEN(TK_BRACE_CLOSE)) {
			break;
		}

		if (m_elementDescentPipe.is_changed(itemState)) {
			m_elementDescentPipe.lock();
			itemState = m_elementDescentPipe.state();
		}

		Declaration();
		if (m_elementDescentPipe.is_changed(itemState)) {
			m_elementDescentPipe.lock();
		}
	} while (NOT_TOKEN(TK_BRACE_CLOSE));
}

void Parser::Declaration()
{
	if (!DeclarationSpecifiers()) {
		return;
	}

	if (MATCH_TOKEN(TK_COMMIT)) {
		NextToken();
	}
	else {
		auto initState = m_elementDescentPipe.state();
		InitDeclaratorList();
		if (m_elementDescentPipe.is_changed(initState)) {
			m_elementDescentPipe.release_until(initState);

			auto decl = std::make_shared<DeclStmt>();
			while (!m_elementDescentPipe.empty()) {
				decl->AddDeclaration(std::dynamic_pointer_cast<VarDecl>(m_elementDescentPipe.next()));
				m_elementDescentPipe.pop();
			}

			m_elementDescentPipe.push(decl);
		}

		if (MATCH_TOKEN(TK_COMMIT)) {
			NextToken();
		}
	}
}

void Parser::InitDeclaratorList()
{
	auto cont = false;
	do {
		cont = false;
		if (!Declarator()) {
			return;
		}

		if (MATCH_TOKEN(TK_ASSIGN)) {
			NextToken();

			auto startState = m_elementDescentPipe.state();
			Initializer();
			m_elementDescentPipe.release_until(startState);

			auto var = std::make_shared<VarDecl>(m_identifierStack.top(), m_elementDescentPipe.next());
			m_identifierStack.pop();
			m_elementDescentPipe.pop();

			assert(var != nullptr);

			m_elementDescentPipe.push(var);
			m_elementDescentPipe.lock();
		}
		else {
			auto var = std::make_shared<VarDecl>(m_identifierStack.top());
			m_identifierStack.pop();
			m_elementDescentPipe.push(var);
			m_elementDescentPipe.lock();

			if (MATCH_TOKEN(TK_COMMA)) {
				cont = true;
				NextToken();
			}
		}
	} while (cont);
}

// Typenames are primitive types
// and used defined structures
void Parser::TypeName()
{
	SpecifierQualifierList();
	AbstractDeclarator();
}

void Parser::AbstractDeclarator()
{
	Pointer();
	DirectAbstractDeclarator();
}

void Parser::DirectAbstractDeclarator()
{
	bool cont = false;
	do {
		cont = false;
		switch (CURRENT_TOKEN()) {
		case TK_PARENTHESE_OPEN:
			NextToken();
			if (MATCH_TOKEN(TK_PARENTHESE_CLOSE)) {
				NextToken();
				EMIT("FUNC DELC EMPTY");//TODO
				cont = true;
			}
			else {
				AbstractDeclarator();
				if (ParameterTypeList()) {
					cont = true;
				}
				ExpectToken(TK_PARENTHESE_CLOSE);
			}
			break;
		case TK_BRACKET_OPEN:
			NextToken();
			if (MATCH_TOKEN(TK_BRACKET_CLOSE)) {
				NextToken();
				EMIT("UNINIT ARRAY?");//TODO
				cont = true;
			}
			else if (MATCH_TOKEN(TK_ASTERISK)) {
				NextToken();
				ExpectToken(TK_BRACKET_CLOSE);
				EMIT("UNINIT ARRAY VARIABLE SZ?");//TODO
				cont = true;
			}
			else {
				AssignmentExpression();
				ExpectToken(TK_BRACKET_CLOSE);
				cont = true;
			}
			break;
		default:
			break;
		}
	} while (cont);
}

void Parser::Initializer()
{
	if (MATCH_TOKEN(TK_BRACE_OPEN)) {
		auto startState = m_elementDescentPipe.state();

		do {
			NextToken();

			// Snapshot current state in case of rollback
			m_comm.Snapshot();
			try {
				Designation();
				m_comm.DisposeSnapshot();
			}
			// Cannot cast, rollback the command state
			catch (const UnexpectedTokenException&) {
				m_comm.Revert();
			}

			Initializer();
		} while (MATCH_TOKEN(TK_COMMA));
		ExpectToken(TK_BRACE_CLOSE);

		m_elementDescentPipe.release_until(startState);

		auto list = std::make_shared<InitListExpr>();
		while (!m_elementDescentPipe.empty()) {
			list->AddListItem(m_elementDescentPipe.next());
			m_elementDescentPipe.pop();
		}

		m_elementDescentPipe.push(list);
		m_elementDescentPipe.lock();
	}
	else {
		AssignmentExpression();
	}
}

void Parser::Designation()
{
	Designators();
	ExpectToken(TK_ASSIGN);
}

void Parser::Designators()
{
	bool cont = false;
	do { //TODO: rewrite loop
		switch (CURRENT_TOKEN()) {
		case TK_BRACKET_OPEN:
			NextToken();
			ConstantExpression();
			ExpectToken(TK_BRACKET_CLOSE);
			cont = true;
			break;
		case TK_DOT:
			NextToken();
			ExpectIdentifier();
			cont = true;
			break;
		}
	} while (cont);
}

void Parser::Pointer()
{
	while (MATCH_TOKEN(TK_ASTERISK)) {
		NextToken();
		TypeQualifierList();
	}
}

bool Parser::Declarator()
{
	Pointer();
	return DirectDeclarator();
}

bool Parser::DirectDeclarator()
{
	auto foundDecl = false;

	if (MATCH_TOKEN(TK_IDENTIFIER)) {
		auto name = CURRENT_DATA()->As<std::string>();
		m_identifierStack.push(name);
		EMIT_IDENTIFIER();
		foundDecl = true;
		NextToken();
	}
	else if (MATCH_TOKEN(TK_PARENTHESE_OPEN)) {
		NextToken();
		Declarator();
		ExpectToken(TK_PARENTHESE_CLOSE);
	}
	else {
		return false;
	}

	// Declarations following an identifier
	for (;;) {
		switch (CURRENT_TOKEN()) {
		case TK_PARENTHESE_OPEN:
			NextToken();
			if (MATCH_TOKEN(TK_PARENTHESE_CLOSE)) {
				NextToken();

				auto func = std::make_shared<FunctionDecl>(m_identifierStack.top());
				m_identifierStack.pop();

				m_elementDescentPipe.push(func);
				return true;
			}
			else {
				if (!ParameterTypeList()) {

					///TODO
					do {
						if (MATCH_TOKEN(TK_IDENTIFIER)) {
							EMIT_IDENTIFIER();
							NextToken();
						}
						else {
							return false;//TMP
						}
					} while (MATCH_TOKEN(TK_COMMA));
					///

				}

				auto func = std::make_shared<FunctionDecl>(m_identifierStack.top());
				func->SetParameterStatement(std::dynamic_pointer_cast<ParamStmt>(m_elementDescentPipe.next()));
				m_elementDescentPipe.pop();
				ExpectToken(TK_PARENTHESE_CLOSE);

				m_elementDescentPipe.push(func);
				return true;
			}
			break;
		case TK_BRACKET_OPEN:
			NextToken();
			switch (CURRENT_TOKEN()) {
			case TK_BRACKET_CLOSE:
				NextToken();
				EMIT("UNINIT ARRAY");
				return true;//TODO: return ptr
			case TK_ASTERISK:
				NextToken();
				EMIT("UNINIT ARRAY VARIABLE SZ");
				ExpectToken(TK_BRACKET_CLOSE);
				return true;//TODO: return ptr
			case TK_STATIC:
				NextToken();
				TypeQualifierList();
				AssignmentExpression();
				ExpectToken(TK_BRACKET_CLOSE);
			default:
				TypeQualifierList(); // optional

				if (MATCH_TOKEN(TK_ASTERISK)) {
					NextToken();
					return true;
				}

				if (MATCH_TOKEN(TK_STATIC)) { // optional
					NextToken();
				}

				AssignmentExpression(); // optional

				while (!m_elementDescentPipe.empty()) {
					m_elementDescentPipe.pop();
				}

				ExpectToken(TK_BRACKET_CLOSE);
			}
			break;
		default:
			goto break_loop;
		}
	}

break_loop:
	return foundDecl;
}

void Parser::TypeQualifierList()
{
	while (TypeQualifier() != Value::TypeQualifier::NONE);
}

bool Parser::ParameterTypeList()
{
	bool rs = false;

	auto startState = m_elementDescentPipe.state();
	for (;;) {
		rs = ParameterDeclaration();
		if (NOT_TOKEN(TK_COMMA)) {
			break;
		}

		NextToken();
		if (MATCH_TOKEN(TK_ELLIPSIS)) {
			NextToken();
			rs = true;
			//TODO: Change function signature
		}

		if (NOT_TOKEN(TK_COMMA)) {
			break;
		}
	}

	m_elementDescentPipe.release_until(startState);

	if (rs) {
		auto param = std::make_shared<ParamStmt>();
		while (!m_elementDescentPipe.empty()) {
			param->AppendParamter(m_elementDescentPipe.next());
			m_elementDescentPipe.pop();
		}

		m_elementDescentPipe.push(param);
	}

	return rs;
}

bool Parser::ParameterDeclaration()
{
	if (!DeclarationSpecifiers()) {
		return false;
	}

	if (Declarator()) {
		auto param = std::make_shared<ParamDecl>(m_identifierStack.top());
		m_identifierStack.pop();

		m_elementDescentPipe.push(param);
		m_elementDescentPipe.lock();
		return true;
	}

	AbstractDeclarator();
	return true; //?
}

bool Parser::FunctionDefinition()
{
	// Return type for function declaration
	DeclarationSpecifiers();

	auto startState = m_elementDescentPipe.state();

	// Must match at least one declarator to qualify as function declaration
	if (!Declarator()) {
		m_elementDescentPipe.release_until(startState);
		return false;
	}

	if (m_elementDescentPipe.empty()) {
		m_elementDescentPipe.release_until(startState);
		return false;
	}

	auto func = std::dynamic_pointer_cast<FunctionDecl>(m_elementDescentPipe.next());
	if (func == nullptr) {
		m_elementDescentPipe.release_until(startState);
		return false;
	}

	m_elementDescentPipe.lock();

	while (Declarator());
	auto res = CompoundStatement();
	if (res) {
		func->SetCompound(std::dynamic_pointer_cast<CompoundStmt>(m_elementDescentPipe.next()));
		m_elementDescentPipe.pop();
	}

	m_elementDescentPipe.release_until(startState);
	return true;
}

// Try as function; if that fails assume declaration
void Parser::ExternalDeclaration()
{
	// Snapshot current state in case of rollback
	m_comm.Snapshot();

	if (!FunctionDefinition()) {
		// Not a function, rollback the command state
		m_comm.Revert();

		// Clear all states since nothing should be kept moving forward
		m_elementDescentPipe.clear();
		ClearStack(m_identifierStack);
		Declaration();
	}
	else {
		// Remove snapshot since we can continue this path
		m_comm.DisposeSnapshot();
	}

	// At top level, release all locks
	m_elementDescentPipe.release_until(LockPipe<decltype(m_elementDescentPipe)>::begin);
	assert(!m_elementDescentPipe.empty());
}

void Parser::TranslationUnit()
{
	//TODO: For each translation unit run the parser loop

	//TODO: name returns file name of current lexer, not translation unit
	AST_ROOT() = std::make_shared<TranslationUnitDecl>(m_profile->MetaInfo()->name);

	do {
		ExternalDeclaration();

		while (!m_elementDescentPipe.empty()) {
			AST_ROOT()->AppendChild(m_elementDescentPipe.next());
			m_elementDescentPipe.pop();
		}

		// There should be no elements
		assert(m_elementDescentPipe.empty(true));

		// Clear all lists where possible before adding new items
		ClearStack(m_identifierStack);
		m_comm.TryClear();
	} while (!lex.IsDone());
}

Parser& Parser::Execute()
{
	NextToken();
	TranslationUnit();

	return (*this);
}
