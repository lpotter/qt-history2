/****************************************************************************
**
** Copyright (C) 2004-$THISYEAR$ Trolltech AS. All rights reserved.
** Copyright (C) 2001-2004 Roberto Raggi
**
** This file is part of the $MODULE$ of the Qt Toolkit.
**
** $LICENSE$
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#ifndef __treewalker_h
#define __treewalker_h

#include "ast.h"

class TreeWalker
{
public:
    TreeWalker() {}
    virtual ~TreeWalker() {}

    virtual void parseNode(AST *node);
    virtual void parseTemplateArgumentList(TemplateArgumentListAST *node);
    virtual void parseClassOrNamespaceName(ClassOrNamespaceNameAST *node);
    virtual void parseName(NameAST *node);
    virtual void parseTypeParameter(TypeParameterAST *node);
    virtual void parseDeclaration(DeclarationAST *node);
    virtual void parseAccessDeclaration(AccessDeclarationAST *node);
    virtual void parseTypeSpecifier(TypeSpecifierAST *node);
    virtual void parseBaseSpecifier(BaseSpecifierAST *node);
    virtual void parseBaseClause(BaseClauseAST *node);
    virtual void parseClassSpecifier(ClassSpecifierAST *node);
    virtual void parseEnumerator(EnumeratorAST *node);
    virtual void parseEnumSpecifier(EnumSpecifierAST *node);
    virtual void parseElaboratedTypeSpecifier(ElaboratedTypeSpecifierAST *node);
    virtual void parseLinkageBody(LinkageBodyAST *node);
    virtual void parseLinkageSpecification(LinkageSpecificationAST *node);
    virtual void parseNamespace(NamespaceAST *node);
    virtual void parseNamespaceAlias(NamespaceAliasAST *node);
    virtual void parseUsing(UsingAST *node);
    virtual void parseUsingDirective(UsingDirectiveAST *node);
    virtual void parseDeclarator(DeclaratorAST *node);
    virtual void parseParameterDeclaration(ParameterDeclarationAST *node);
    virtual void parseParameterDeclarationList(ParameterDeclarationListAST *node);
    virtual void parseParameterDeclarationClause(ParameterDeclarationClauseAST *node);
    virtual void parseInitDeclarator(InitDeclaratorAST *node);
    virtual void parseInitDeclaratorList(InitDeclaratorListAST *node);
    virtual void parseTypedef(TypedefAST *node);
    virtual void parseTemplateParameter(TemplateParameterAST *node);
    virtual void parseTemplateParameterList(TemplateParameterListAST *node);
    virtual void parseTemplateDeclaration(TemplateDeclarationAST *node);
    virtual void parseSimpleDeclaration(SimpleDeclarationAST *node);
    virtual void parseStatement(StatementAST *node);
    virtual void parseExpressionStatement(ExpressionStatementAST *node);
    virtual void parseCondition(ConditionAST *node);
    virtual void parseIfStatement(IfStatementAST *node);
    virtual void parseWhileStatement(WhileStatementAST *node);
    virtual void parseDoStatement(DoStatementAST *node);
    virtual void parseForStatement(ForStatementAST *node);
    virtual void parseSwitchStatement(SwitchStatementAST *node);
    virtual void parseReturnStatement(ReturnStatementAST *node);
    virtual void parseStatementList(StatementListAST *node);
    virtual void parseDeclarationStatement(DeclarationStatementAST *node);
    virtual void parseFunctionDefinition(FunctionDefinitionAST *node);
    virtual void parseTranslationUnit(TranslationUnitAST *node);
    virtual void parseExpression(AbstractExpressionAST *node);
    virtual void parseBinaryExpression(BinaryExpressionAST *node);
};

inline void TreeWalker::parseNode(AST *node)
{
    if (!node)
        return;

    switch(node->nodeType()) {

    case NodeType_Declaration:
    case NodeType_AccessDeclaration:
    case NodeType_LinkageSpecification:
    case NodeType_Namespace:
    case NodeType_NamespaceAlias:
    case NodeType_Using:
    case NodeType_UsingDirective:
    case NodeType_Typedef:
    case NodeType_TemplateDeclaration:
    case NodeType_SimpleDeclaration:
    case NodeType_FunctionDefinition:
        parseDeclaration(static_cast<DeclarationAST*>(node));
        break;

    case NodeType_Statement:
    case NodeType_ExpressionStatement:
    case NodeType_IfStatement:
    case NodeType_WhileStatement:
    case NodeType_DoStatement:
    case NodeType_ForStatement:
    case NodeType_SwitchStatement:
    case NodeType_StatementList:
    case NodeType_DeclarationStatement:
    case NodeType_ReturnStatement:
        parseStatement(static_cast<StatementAST*>(node));
        break;

    case NodeType_TypeSpecifier:
    case NodeType_ClassSpecifier:
    case NodeType_EnumSpecifier:
    case NodeType_ElaboratedTypeSpecifier:
        parseTypeSpecifier(static_cast<TypeSpecifierAST*>(node));
        break;

    case NodeType_TemplateArgumentList:
        parseTemplateArgumentList(static_cast<TemplateArgumentListAST*>(node));
        break;
    case NodeType_ClassOrNamespaceName:
        parseClassOrNamespaceName(static_cast<ClassOrNamespaceNameAST*>(node));
        break;
    case NodeType_Name:
        parseName(static_cast<NameAST*>(node));
        break;
    case NodeType_TypeParameter:
        parseTypeParameter(static_cast<TypeParameterAST*>(node));
        break;
    case NodeType_BaseSpecifier:
        parseBaseSpecifier(static_cast<BaseSpecifierAST*>(node));
        break;
    case NodeType_BaseClause:
        parseBaseClause(static_cast<BaseClauseAST*>(node));
        break;
    case NodeType_Enumerator:
        parseEnumerator(static_cast<EnumeratorAST*>(node));
        break;
    case NodeType_LinkageBody:
        parseLinkageBody(static_cast<LinkageBodyAST*>(node));
        break;
    case NodeType_Declarator:
        parseDeclarator(static_cast<DeclaratorAST*>(node));
        break;
    case NodeType_ParameterDeclaration:
        parseParameterDeclaration(static_cast<ParameterDeclarationAST*>(node));
        break;
    case NodeType_ParameterDeclarationList:
        parseParameterDeclarationList(static_cast<ParameterDeclarationListAST*>(node));
        break;
    case NodeType_ParameterDeclarationClause:
        parseParameterDeclarationClause(static_cast<ParameterDeclarationClauseAST*>(node));
        break;
    case NodeType_InitDeclarator:
        parseInitDeclarator(static_cast<InitDeclaratorAST*>(node));
        break;
    case NodeType_InitDeclaratorList:
        parseInitDeclaratorList(static_cast<InitDeclaratorListAST*>(node));
        break;
    case NodeType_TemplateParameter:
        parseTemplateParameter(static_cast<TemplateParameterAST*>(node));
        break;
    case NodeType_TemplateParameterList:
        parseTemplateParameterList(static_cast<TemplateParameterListAST*>(node));
        break;
    case NodeType_Condition:
        parseCondition(static_cast<ConditionAST*>(node));
        break;
    case NodeType_TranslationUnit:
        parseTranslationUnit(static_cast<TranslationUnitAST*>(node));
        break;

    case NodeType_BinaryExpression:
        parseBinaryExpression(static_cast<BinaryExpressionAST*>(node));
        break;

    case NodeType_Expression:
    case NodeType_PrimaryExpression:

    case NodeType_PostfixExpression:
    case NodeType_Subscripting:
    case NodeType_FunctionCall:
    case NodeType_ExplicitTypeConversion:
    case NodeType_PseudoConstructorCall:
    case NodeType_ClassMemberAccess:
    case NodeType_IncrDecr:
    case NodeType_CppCastExpression:
    case NodeType_TypeIdentification:

    case NodeType_UnaryExpression:
    case NodeType_NewExpression:
    case NodeType_NewTypeId:
    case NodeType_NewDeclarator:
    case NodeType_NewInitializer:
    case NodeType_DeleteExpression:
    case NodeType_CastExpression:
    case NodeType_ConditionalExpression:
    case NodeType_ThrowExpression:
        parseExpression(static_cast<AbstractExpressionAST*>(node));
        break;
    }
}

#endif // __treewalker_h
