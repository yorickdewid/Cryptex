// Copyright (c) 2017 Quenza Inc. All rights reserved.
//
// This file is part of the Cryptox project.
//
// Use of this source code is governed by a private license
// that can be found in the LICENSE file. Content can not be
// copied and/or distributed without the express of the author.

#include <CryCC/AST/ASTNode.h>
#include <CryCC/AST/Factory.h>

namespace CryCC
{
namespace AST
{

namespace
{

template<typename NodeType, typename = typename std::enable_if<std::is_base_of<ASTNode, NodeType>::value>::type>
ASTNodeType ReturnNode(Serializable::VisitorInterface *visitor)
{
	std::shared_ptr<ASTNode> node = std::make_shared<NodeType>(*visitor);
	visitor->FireDependencies(node);
	return node;
}

} // namespace

std::shared_ptr<ASTNode> ASTFactory::MakeNode(Serializable::VisitorInterface *visitor)
{
	switch (visitor->GetNodeId())
	{
		// The invalid node is hit when there is no data left in the input stream.
		// In order to signal the end of input, throw an exception.
		// FUTURE: Inval can be touched when the input cannot be processed, but is
		//         not done. There must be a more reliable method to test of the
		//         end of input stream.
	case NodeID::INVAL:
		throw InvalidStreamException{};

	case NodeID::BINARY_OPERATOR_ID:
		return ReturnNode<BinaryOperator>(visitor);
	case NodeID::CONDITIONAL_OPERATOR_ID:
		return ReturnNode<ConditionalOperator>(visitor);
	case NodeID::UNARY_OPERATOR_ID:
		return ReturnNode<UnaryOperator>(visitor);
	case NodeID::COMPOUND_ASSIGN_OPERATOR_ID:
		return ReturnNode<CompoundAssignOperator>(visitor);

	case NodeID::CHARACTER_LITERAL_ID:
		return ReturnNode<CharacterLiteral>(visitor);
	case NodeID::STRING_LITERAL_ID:
		return ReturnNode<StringLiteral>(visitor);
	case NodeID::INTEGER_LITERAL_ID:
		return ReturnNode<IntegerLiteral>(visitor);
	case NodeID::FLOAT_LITERAL_ID:
		return ReturnNode<FloatingLiteral>(visitor);

	case NodeID::VAR_DECL_ID:
		return ReturnNode<VarDecl>(visitor);
	case NodeID::PARAM_DECL_ID:
		return ReturnNode<ParamDecl>(visitor);
	case NodeID::VARIADIC_DECL_ID:
		return ReturnNode<VariadicDecl>(visitor);
	case NodeID::TYPEDEF_DECL_ID:
		return ReturnNode<TypedefDecl>(visitor);
	case NodeID::FIELD_DECL_ID:
		return ReturnNode<FieldDecl>(visitor);
	case NodeID::RECORD_DECL_ID:
		return ReturnNode<RecordDecl>(visitor);
	case NodeID::ENUM_CONSTANT_DECL_ID:
		return ReturnNode<EnumConstantDecl>(visitor);
	case NodeID::ENUM_DECL_ID:
		return ReturnNode<EnumDecl>(visitor);
	case NodeID::FUNCTION_DECL_ID:
		return ReturnNode<FunctionDecl>(visitor);
	case NodeID::TRANSLATION_UNIT_DECL_ID:
		return ReturnNode<TranslationUnitDecl>(visitor);

	case NodeID::RESOLVE_REF_EXPR_ID:
		break;
	case NodeID::DECL_REF_EXPR_ID:
		return ReturnNode<DeclRefExpr>(visitor);
	case NodeID::CALL_EXPR_ID:
		return ReturnNode<CallExpr>(visitor);
	case NodeID::BUILTIN_EXPR_ID:
		//return ReturnNode<BuiltinExpr>(visitor);
	case NodeID::CAST_EXPR_ID:
		return ReturnNode<CastExpr>(visitor);
	case NodeID::IMPLICIT_CONVERTION_EXPR_ID:
		return ReturnNode<ImplicitConvertionExpr>(visitor);
	case NodeID::PAREN_EXPR_ID:
		return ReturnNode<ParenExpr>(visitor);
	case NodeID::INIT_LIST_EXPR_ID:
		return ReturnNode<InitListExpr>(visitor);
	case NodeID::COMPOUND_LITERAL_EXPR_ID:
		return ReturnNode<CompoundLiteralExpr>(visitor);
	case NodeID::ARRAY_SUBSCRIPT_EXPR_ID:
		return ReturnNode<ArraySubscriptExpr>(visitor);
	case NodeID::MEMBER_EXPR_ID:
		return ReturnNode<MemberExpr>(visitor);

	case NodeID::CONTINUE_STMT_ID:
		return ReturnNode<ContinueStmt>(visitor);
	case NodeID::RETURN_STMT_ID:
		return ReturnNode<ReturnStmt>(visitor);
	case NodeID::IF_STMT_ID:
		return ReturnNode<IfStmt>(visitor);
	case NodeID::SWITCH_STMT_ID:
		return ReturnNode<SwitchStmt>(visitor);
	case NodeID::WHILE_STMT_ID:
		return ReturnNode<WhileStmt>(visitor);
	case NodeID::DO_STMT_ID:
		return ReturnNode<DoStmt>(visitor);
	case NodeID::FOR_STMT_ID:
		return ReturnNode<ForStmt>(visitor);
	case NodeID::BREAK_STMT_ID:
		return ReturnNode<BreakStmt>(visitor);
	case NodeID::DEFAULT_STMT_ID:
		return ReturnNode<DefaultStmt>(visitor);
	case NodeID::CASE_STMT_ID:
		return ReturnNode<CaseStmt>(visitor);
	case NodeID::DECL_STMT_ID:
		return ReturnNode<DeclStmt>(visitor);
	case NodeID::ARGUMENT_STMT_ID:
		return ReturnNode<ArgumentStmt>(visitor);
	case NodeID::PARAM_STMT_ID:
		return ReturnNode<ParamStmt>(visitor);
	case NodeID::LABEL_STMT_ID:
		return ReturnNode<LabelStmt>(visitor);
	case NodeID::GOTO_STMT_ID:
		return ReturnNode<GotoStmt>(visitor);
	case NodeID::COMPOUND_STMT_ID:
		return ReturnNode<CompoundStmt>(visitor);

		// NOTE: The intermediate nodes cannot be instantiated
		//       and therefore indicates an erorr.
	case NodeID::AST_NODE_ID:
	case NodeID::OPERATOR_ID:
	case NodeID::LITERAL_ID:
	case NodeID::DECL_ID:
	case NodeID::EXPR_ID:
	case NodeID::STMT_ID:
	default:
		break;
	}

	CryImplExcept(); //TODO
}

} // namespace AST
} // namespace CryCC
