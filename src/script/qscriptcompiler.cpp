/****************************************************************************
**
** Copyright (C) 1992-$THISYEAR$ $TROLLTECH$. All rights reserved.
**
** This file is part of the $MODULE$ of the Qt Toolkit.
**
** $TROLLTECH_DUAL_LICENSE$
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "qscriptengine_p.h"
#include "qscriptcompiler_p.h"
#include "qscriptast_p.h"

#include <QtCore/QtDebug>

namespace QScript {

class FetchName: protected AST::Visitor
{
public:
    inline FetchName(QScriptEnginePrivate *e):
        eng(e), name(0) {}

    QScriptNameIdImpl *operator() (AST::PropertyName *node)
    {
        name = 0;
        node->accept(this);
        return name;
    }

protected:
    virtual bool visit(AST::IdentifierPropertyName *node)
    {
        name = node->id;
        return false;
    }

    virtual bool visit(AST::StringLiteralPropertyName *node)
    {
        name = node->id;
        return false;
    }

    virtual bool visit(AST::NumericLiteralPropertyName *node)
    {
        name = eng->nameId(QString::number(node->id), /*persistent=*/false); // ### don't use QString::number
        name->persistent = true; // ### remove
        return false;
    }

private:
    QScriptEnginePrivate *eng;
    QScriptNameIdImpl *name;
};

class EmptySourceElements: protected AST::Visitor
{
public:
    EmptySourceElements(QScriptEngine *d):
        driver(d), empty(false) {}

    inline bool operator () (AST::Node *)
    {
        empty = false;
        return empty;
    }

private:
    QScriptEngine *driver;
    bool empty;
};

class DeclareLocals: protected AST::Visitor
{
public:
    DeclareLocals(Compiler *c):
        compiler(c),
        eng(c->engine())
    {
    }

    void operator () (AST::Node *node)
    {
        if (node)
            node->accept(this);
    }

protected:
    virtual bool visit(AST::FunctionDeclaration *node)
    {
        compiler->iDeclareLocal(node->name);
        return false;
    }

    virtual bool visit(AST::FunctionExpression *)
    { return false; }

    virtual bool visit(AST::VariableDeclaration *node)
    {
        compiler->iDeclareLocal(node->name);
        return false;
    }

private:
    Compiler *compiler;
    QScriptEnginePrivate *eng;
};

Compiler::Compiler(QScriptEngine *eng):
    m_eng(QScriptEnginePrivate::get(eng)),
    m_state(0),
    m_topLevelCompiler(false),
    m_activeLoop(0)
{
}

Compiler::~Compiler()
{
}

bool Compiler::topLevelCompiler() const
{
    return m_topLevelCompiler;
}

void Compiler::setTopLevelCompiler(bool b)
{
    m_topLevelCompiler = b;
}

 CompilationUnit Compiler::compile(AST::Node *node, const QList<QScriptNameIdImpl *> &formals)
{
    m_formals = formals;
    m_state = 0;
    m_instructions.clear();
    m_exceptionHandlers.clear();
    m_generateFastArgumentLookup = false; // ### !formals.isEmpty();  // ### disabled for now.. it's buggy :(

    m_compilationUnit = CompilationUnit();

    if (node)
        node->accept(this);

    // add a terminator
    if (topLevelCompiler()) {
        iHalt();
    } else if (m_instructions.isEmpty() || m_instructions.last().op != QScriptInstruction::OP_Ret) {
        iLoadUndefined();
        iRet();
    }

    m_compilationUnit.setInstructions(m_instructions);
    m_compilationUnit.setExceptionHandlers(m_exceptionHandlers);
    return m_compilationUnit;
}

bool Compiler::preVisit(AST::Node *)
{
    return m_compilationUnit.isValid();
}

bool Compiler::visit(AST::SourceElements *node)
{
    DeclareLocals declareLocals(this);
    declareLocals(node);

    bool was = changeParseStatements(false);

    for (AST::SourceElements *it = node; it != 0; it = it->next)
        it->element->accept(this);

    changeParseStatements(true);

    for (AST::SourceElements *it = node; it != 0; it = it->next)
        it->element->accept(this);

    changeParseStatements(was);

    return false;
}

bool Compiler::visit(AST::StatementList *)
{
    return true;
}

bool Compiler::visit(AST::FunctionSourceElement *)
{
    return m_parseStatements == 0;
}

bool Compiler::visit(AST::StatementSourceElement *)
{
    return m_parseStatements;
}

bool Compiler::visit(AST::ThisExpression *)
{
    iLoadThis();
    return false;
}

bool Compiler::visit(AST::NullExpression *)
{
    iLoadNull();
    return false;
}

bool Compiler::visit(AST::RegExpLiteral *node)
{
    Q_ASSERT(node->pattern != 0);

    if (node->flags)
        iNewRegExp(node->pattern, node->flags);
    else
        iNewRegExp(node->pattern);

    return false;
}

bool Compiler::visit(AST::NumericLiteral *node)
{
    iLoadNumber(node->value);
    return false;
}

bool Compiler::visit(AST::StringLiteral *node)
{
    iNewString(node->value);

    return false;
}

bool Compiler::visit(AST::ObjectLiteral *node)
{
    iNewObject();

    FetchName fetchName(m_eng);

    for (AST::PropertyNameAndValueList *it = node->properties; it != 0; it = it->next) {
        iDuplicate();

        QScriptNameIdImpl *name = fetchName(it->name);
        Q_ASSERT(name != 0);
        iLoadString(name);
        iMakeReference();

        it->value->accept(this);
        iPutField();
    }

    return false;
}

bool Compiler::visit(AST::IdentifierExpression *node)
{
    Q_ASSERT(node->name != 0);

    if (m_generateReferences)
        iResolve(node->name);
    else if (node->name == m_eng->idTable()->id_arguments)
        iFetchArguments();
    else
        iFetch(node->name);

    return false;
}

bool Compiler::visit(AST::FunctionDeclaration *node)
{
    iResolve(node->name);
    iNewClosure(node->formals, node->body);
    iPutField();
    return false;
}

bool Compiler::visit(AST::FunctionExpression *node)
{
    if (node->name) {
        iResolve(node->name);
        iNewClosure(node->formals, node->body);
        iAssign();
    } else {
        iNewClosure(node->formals, node->body);
    }
    return false;
}

bool Compiler::visit(AST::CallExpression *node)
{
    bool was = generateReferences(true);
    node->base->accept(this);
    generateReferences(false);

    int argc = 0;
    for (AST::ArgumentList *it = node->arguments; it != 0; it = it->next) {
        it->expression->accept(this);
        ++argc;
    }

    generateReferences(was);

    iCall(argc);
    return false;
}

bool Compiler::visit(AST::NewExpression *node)
{
    bool was = generateReferences(true);
    node->expression->accept(this);
    generateReferences(was);
    iNew(0);
    return false;
}

bool Compiler::visit(AST::NewMemberExpression *node)
{
    bool was = generateReferences(true);
    node->base->accept(this);
    generateReferences(false);

    int argc = 0;
    for (AST::ArgumentList *it = node->arguments; it != 0; it = it->next) {
        it->expression->accept(this);
        ++argc;
    }

    generateReferences(was);

    iNew(argc);
    return false;
}

bool Compiler::visit(AST::FieldMemberExpression *node)
{
    bool was = generateReferences(false);
    node->base->accept(this);
    generateReferences(was);

    iLoadString(node->name);

    if (! was)
        iFetchField();
    else
        iMakeReference();

    return false;
}

bool Compiler::visit(AST::ArrayMemberExpression *node)
{
    bool was = generateReferences(false);
    node->base->accept(this);
    node->expression->accept(this);
    generateReferences(was);

    if (! was)
        iFetchField();
    else
        iMakeReference();

    return false;
}

bool Compiler::visit(AST::PostIncrementExpression *node)
{
    bool was = generateReferences(true);
    node->base->accept(this);
    generateReferences(was);
    iPostIncr();

    return false;
}

bool Compiler::visit(AST::PostDecrementExpression *node)
{
    bool was = generateReferences(true);
    node->base->accept(this);
    generateReferences(was);
    iPostDecr();

    return false;
}

bool Compiler::visit(AST::PreIncrementExpression *node)
{
    bool was = generateReferences(true);
    node->expression->accept(this);
    generateReferences(was);
    iIncr();
    return false;
}

bool Compiler::visit(AST::PreDecrementExpression *node)
{
    bool was = generateReferences(true);
    node->expression->accept(this);
    generateReferences(was);
    iDecr();
    return false;
}

void Compiler::endVisit(AST::NotExpression *)
{
    iNot();
}

void Compiler::endVisit(AST::TildeExpression *)
{
    iBitNot();
}

bool Compiler::visit(AST::ThrowStatement *node)
{
    iLine(node);
    return true;
}

bool Compiler::visit(AST::TryStatement *node)
{
    int start = nextInstructionOffset();
    if (node->statement)
        node->statement->accept(this);
    int end = nextInstructionOffset();
    if (node->catchExpression) {
        iBranch(0); // skip the catch if no exception
        ExceptionHandlerDescriptor ehd(start, end, nextInstructionOffset());
        m_exceptionHandlers.append(ehd);
        iBeginCatch(node->catchExpression->name);
        node->catchExpression->statement->accept(this);
        iEndCatch();
        patchInstruction(end, nextInstructionOffset() - end);
    }
    if (node->finallyExpression) {
        if (!node->catchExpression) {
            ExceptionHandlerDescriptor ehd(start, end, nextInstructionOffset());
            m_exceptionHandlers.prepend(ehd);
        }
        node->finallyExpression->statement->accept(this);
    }
    return false;
}

void Compiler::endVisit(AST::ThrowStatement *node)
{
    if (! node->expression)
        iLoadUndefined();

    iThrow();
}

void Compiler::endVisit(AST::VoidExpression *)
{
    iPop();
    iLoadUndefined();
}

bool Compiler::visit(AST::TypeOfExpression *node)
{
    bool was = generateReferences(true);
    node->expression->accept(this);
    generateReferences(was);
    iTypeOf();
    return false;
}

bool Compiler::visit(AST::DeleteExpression *node)
{
    bool was = generateReferences(true);
    node->expression->accept(this);
    generateReferences(was);
    iDelete();
    return false;
}

bool Compiler::visit(AST::ReturnStatement *node)
{
    iLine(node);
    return true;
}

void Compiler::endVisit(AST::ReturnStatement *node)
{
    if (! node->expression)
        iLoadUndefined();

    iRet();
}

bool Compiler::visit(AST::VariableStatement *node)
{
    AST::VariableDeclarationList *lst = node->declarations;
    while (lst) {
        if (lst->declaration->expression) {
            iLine(node);
            break;
        }
        lst = lst->next;
    }
    return true;
}

bool Compiler::visit(AST::VariableDeclaration *node)
{
    if (node->expression != 0) {
        iResolve(node->name);
        node->expression->accept(this);
        iPutField();
    }

    return false;
}

bool Compiler::visit(AST::ConditionalExpression *node)
{
    node->expression->accept(this);

    int cond = nextInstructionOffset();
    iBranchFalse(0);

    node->ok->accept(this);

    if (! node->ko) {
        patchInstruction(cond, nextInstructionOffset() - cond);
    } else {
        int terminator = nextInstructionOffset();
        iBranch(0);
        node->ko->accept(this);

        patchInstruction(cond, terminator + 1 - cond);
        patchInstruction(terminator, nextInstructionOffset() - terminator);
    }

    return false;
}

bool Compiler::visit(AST::IfStatement *node)
{
    iLine(node);
    node->expression->accept(this);

    int cond = nextInstructionOffset();
    iBranchFalse(0);

    node->ok->accept(this);

    if (! node->ko) {
        patchInstruction(cond, nextInstructionOffset() - cond);
    } else {
        int terminator = nextInstructionOffset();
        iBranch(0);
        node->ko->accept(this);

        patchInstruction(cond, terminator + 1 - cond);
        patchInstruction(terminator, nextInstructionOffset() - terminator);
    }

    return false;
}

bool Compiler::visit(AST::Block *node)
{
    if (node->statements && m_loops.contains(node)) {
        Loop &loop = m_loops[node];

        node->statements->accept(this);

        loop.breakLabel.offset = nextInstructionOffset();

        foreach (int index, loop.breakLabel.uses) {
            patchInstruction(index, loop.breakLabel.offset - index);
        }

        return false;
    }

    return true;
}

bool Compiler::visit(AST::WhileStatement *node)
{
    Loop *previousLoop = changeActiveLoop(&m_loops[node]);
    m_activeLoop->continueLabel.offset = nextInstructionOffset();

    int again = nextInstructionOffset();
    iLine(node);
    node->expression->accept(this);

    int cond = nextInstructionOffset();
    iBranchFalse(0);

    bool was = iterationStatement(true);
    bool was2 = generateLeaveOnBreak(false);
    node->statement->accept(this);
    generateLeaveOnBreak(was2);
    iterationStatement(was);

    iBranch(again - nextInstructionOffset());
    patchInstruction(cond, nextInstructionOffset() - cond);

    m_activeLoop->breakLabel.offset = nextInstructionOffset();

    foreach (int index, m_activeLoop->breakLabel.uses) {
        patchInstruction(index, m_activeLoop->breakLabel.offset - index);
    }

    foreach (int index, m_activeLoop->continueLabel.uses) {
        patchInstruction(index, m_activeLoop->continueLabel.offset - index);
    }

    changeActiveLoop(previousLoop);
    m_loops.remove(node);

    return false;
}

bool Compiler::visit(AST::DoWhileStatement *node)
{
    Loop *previousLoop = changeActiveLoop(&m_loops[node]);
    int again = nextInstructionOffset();
    bool was = iterationStatement(true);
    node->statement->accept(this);
    iterationStatement(was);

    m_activeLoop->continueLabel.offset = nextInstructionOffset();

    iLine(node->expression);
    node->expression->accept(this);

    iBranchTrue(again - nextInstructionOffset());
    m_activeLoop->breakLabel.offset = nextInstructionOffset();

    foreach (int index, m_activeLoop->breakLabel.uses) {
        patchInstruction(index, m_activeLoop->breakLabel.offset - index);
    }

    foreach (int index, m_activeLoop->continueLabel.uses) {
        patchInstruction(index, m_activeLoop->continueLabel.offset - index);
    }

    changeActiveLoop(previousLoop);
    m_loops.remove(node);

    return false;
}

bool Compiler::visit(AST::ForEachStatement *node)
{
    Loop *previousLoop = changeActiveLoop(&m_loops[node]);

    node->expression->accept(this);
    iNewEnumeration();
    iDuplicate();
    iToFirstElement();

    int again = nextInstructionOffset();
    m_activeLoop->continueLabel.offset = again;
    iLine(node);
    iDuplicate();
    iHasNextElement();
    int cond = nextInstructionOffset();
    iBranchFalse(0);
    bool was = generateReferences(true);
    node->initialiser->accept(this);
    generateReferences(was);
    iNextElement();
    iAssign();
    iPop();
    was = iterationStatement(true);
    node->statement->accept(this);
    iterationStatement(was);
    iBranch(again - nextInstructionOffset());
    patchInstruction(cond, nextInstructionOffset() - cond);

    m_activeLoop->breakLabel.offset = nextInstructionOffset();
    iPop(); // pop the Enumeration

    foreach (int index, m_activeLoop->breakLabel.uses) {
        patchInstruction(index, m_activeLoop->breakLabel.offset - index);
    }

    foreach (int index, m_activeLoop->continueLabel.uses) {
        patchInstruction(index, m_activeLoop->continueLabel.offset - index);
    }

    changeActiveLoop(previousLoop);
    m_loops.remove(node);

    return false;
}

bool Compiler::visit(AST::LocalForEachStatement *node)
{
    Loop *previousLoop = changeActiveLoop(&m_loops[node]);

    node->declaration->accept(this);
    node->expression->accept(this);
    iNewEnumeration();
    iDuplicate();
    iToFirstElement();

    int again = nextInstructionOffset();
    m_activeLoop->continueLabel.offset = again;
    iLine(node);
    iDuplicate();
    iHasNextElement();
    int cond = nextInstructionOffset();
    iBranchFalse(0);
    iResolve(node->declaration->name);
    iNextElement();
    iAssign();
    iPop();
    bool was = iterationStatement(true);
    node->statement->accept(this);
    iterationStatement(was);
    iBranch(again - nextInstructionOffset());
    patchInstruction(cond, nextInstructionOffset() - cond);

    m_activeLoop->breakLabel.offset = nextInstructionOffset();
    iPop(); // pop the Enumeration

    foreach (int index, m_activeLoop->breakLabel.uses) {
        patchInstruction(index, m_activeLoop->breakLabel.offset - index);
    }

    foreach (int index, m_activeLoop->continueLabel.uses) {
        patchInstruction(index, m_activeLoop->continueLabel.offset - index);
    }

    changeActiveLoop(previousLoop);
    m_loops.remove(node);

    return false;
}

void Compiler::visitForInternal(AST::Statement *node, AST::ExpressionNode *condition, AST::Statement *statement, AST::ExpressionNode *expression)
{
    Q_ASSERT(statement != 0);

    int again = nextInstructionOffset();
    if (condition != 0) {
        iLine(condition);
        condition->accept(this);
    } else {
        iLine(node);
        iLoadNumber(1);
    }

    int cond = nextInstructionOffset();
    iBranchFalse(0);

    Loop *previousLoop = changeActiveLoop(&m_loops[node]);

    bool was = iterationStatement(true);
    statement->accept(this);
    iterationStatement(was);

    m_activeLoop->continueLabel.offset = nextInstructionOffset();

    if (expression != 0) {
        expression->accept(this);
        iPop();
    }

    iBranch(again - nextInstructionOffset());
    patchInstruction(cond, nextInstructionOffset() - cond);

    m_activeLoop->breakLabel.offset = nextInstructionOffset();

    foreach (int index, m_activeLoop->breakLabel.uses) {
        patchInstruction(index, m_activeLoop->breakLabel.offset - index);
    }

    foreach (int index, m_activeLoop->continueLabel.uses) {
        patchInstruction(index, m_activeLoop->continueLabel.offset - index);
    }

    changeActiveLoop(previousLoop);
    m_loops.remove(node);
}

bool Compiler::visit(AST::ForStatement *node)
{
    iLine(node);

    if (node->initialiser != 0) {
        node->initialiser->accept(this);
        iPop();
    }

    visitForInternal(node, node->condition, node->statement, node->expression);
    return false;
}

bool Compiler::visit(AST::LocalForStatement *node)
{
    iLine(node);

    if (node->declarations)
        node->declarations->accept(this);

    visitForInternal(node, node->condition, node->statement, node->expression);
    return false;
}

bool Compiler::isAssignmentOperator(int op) const
{
    switch (op) {

    case QSOperator::Assign:
    case QSOperator::InplaceAnd:
    case QSOperator::InplaceSub:
    case QSOperator::InplaceDiv:
    case QSOperator::InplaceAdd:
    case QSOperator::InplaceLeftShift:
    case QSOperator::InplaceMod:
    case QSOperator::InplaceMul:
    case QSOperator::InplaceOr:
    case QSOperator::InplaceRightShift:
    case QSOperator::InplaceURightShift:
    case QSOperator::InplaceXor:
        return true;

    default:
        break;

    }

    return false;
}

bool Compiler::visit(AST::Expression *node)
{
    node->left->accept(this);
    iPop(); // ### or iSync?
    node->right->accept(this);
    return false;
}

bool Compiler::visit(AST::BinaryExpression *node)
{
    if (isAssignmentOperator(node->op)) {
        bool was = generateReferences(true);
        node->left->accept(this);
        generateReferences(was);
    } else {
        node->left->accept(this);
    }

    int address = 0;
    if (node->op == QSOperator::Or || node->op == QSOperator::And) {
        iDuplicate();
        address = nextInstructionOffset();
        if (node->op == QSOperator::Or)
            iBranchTrue(0);
        else
            iBranchFalse(0);
        iPop();
    }

    node->right->accept(this);

    switch (node->op) {

    case QSOperator::Assign:
        iAssign();
        break;

    case QSOperator::InplaceAnd:
        iInplaceAnd();
        break;

    case QSOperator::InplaceSub:
        iInplaceSub();
        break;

    case QSOperator::InplaceDiv:
        iInplaceDiv();
        break;

    case QSOperator::InplaceAdd:
        iInplaceAdd();
        break;

    case QSOperator::InplaceLeftShift:
        iInplaceLeftShift();
        break;

    case QSOperator::InplaceMod:
        iInplaceMod();
        break;

    case QSOperator::InplaceMul:
        iInplaceMul();
        break;

    case QSOperator::InplaceOr:
        iInplaceOr();
        break;

    case QSOperator::InplaceRightShift:
        iInplaceRightShift();
        break;

    case QSOperator::InplaceURightShift:
        iInplaceURightShift();
        break;

    case QSOperator::InplaceXor:
        iInplaceXor();
        break;

    case QSOperator::BitAnd:
        iBitAnd();
        break;

    case QSOperator::BitOr:
        iBitOr();
        break;

    case QSOperator::BitXor:
        iBitXor();
        break;

    case QSOperator::LShift:
        iLeftShift();
        break;

    case QSOperator::Mod:
        iMod();
        break;

    case QSOperator::RShift:
        iRightShift();
        break;

    case QSOperator::URShift:
        iURightShift();
        break;

    case QSOperator::InstanceOf:
        iInstanceOf();
        break;

    case QSOperator::Add:
        iAdd();
        break;

    case QSOperator::And:
        patchInstruction(address, nextInstructionOffset() - address);
        break;

    case QSOperator::Div:
        iDiv();
        break;

    case QSOperator::Equal:
        iEqual();
        break;

    case QSOperator::Ge:
        iGreatOrEqual();
        break;

    case QSOperator::Gt:
        iGreatThan();
        break;

    case QSOperator::Le:
        iLessOrEqual();
        break;

    case QSOperator::Lt:
        iLessThan();
        break;

    case QSOperator::Mul:
        iMul();
        break;

    case QSOperator::NotEqual:
        iNotEqual();
        break;

    case QSOperator::Or:
        patchInstruction(address, nextInstructionOffset() - address);
        break;

    case QSOperator::Sub:
        iSub();
        break;

    case QSOperator::StrictEqual:
        iStrictEqual();
        break;

    case QSOperator::StrictNotEqual:
        iStrictNotEqual();
        break;

    case QSOperator::In:
        iIn();
        break;
    }

    return false;
}

bool Compiler::visit(AST::TrueLiteral *)
{
    iLoadTrue();
    return false;
}

bool Compiler::visit(AST::FalseLiteral *)
{
    iLoadFalse();
    return false;
}

bool Compiler::visit(AST::SwitchStatement *node)
{
    iLine(node);
    Loop *previousLoop = changeActiveLoop(&m_loops[node]);

    node->expression->accept(this);

    bool was = switchStatement(true);

    AST::CaseClauses *clauses;
    int skipIndex = -1;
    int fallthroughIndex = -1;
    // ### make a function for this
    for (clauses = node->block->clauses; clauses != 0; clauses = clauses->next) {
        AST::CaseClause *clause = clauses->clause;
        if (skipIndex != -1)
            patchInstruction(skipIndex, nextInstructionOffset() - skipIndex);

        iDuplicate(); // expression
        clause->expression->accept(this);
        iStrictEqual();
        skipIndex = nextInstructionOffset();
        iBranchFalse(0); // next case

        if (fallthroughIndex != -1) // previous case falls through to here
            patchInstruction(fallthroughIndex, nextInstructionOffset() - fallthroughIndex);

        int breaksBefore = m_activeLoop->breakLabel.uses.count();
        if (clause->statements)
            clause->statements->accept(this);
        int breaksAfter = m_activeLoop->breakLabel.uses.count();
        if (breaksAfter == breaksBefore) { // fallthrough
            fallthroughIndex = nextInstructionOffset();
            iBranch(0);
        } else { // no fallthrough (break)
            fallthroughIndex = -1;
        }
    }

    if (fallthroughIndex != -1) {
        patchInstruction(fallthroughIndex, nextInstructionOffset() - fallthroughIndex);
        fallthroughIndex = -1;
    }

    int defaultIndex = -1;
    if (node->block->defaultClause) {
        int skipDefaultIndex = -1;
        if (!node->block->clauses && node->block->moreClauses) {
            skipDefaultIndex = nextInstructionOffset();
            iBranch(0);
        }
        defaultIndex = nextInstructionOffset();
        int breaksBefore = m_activeLoop->breakLabel.uses.count();
        if (node->block->defaultClause->statements)
            node->block->defaultClause->statements->accept(this);
        int breaksAfter = m_activeLoop->breakLabel.uses.count();
        if (breaksAfter == breaksBefore) { // fallthrough
            fallthroughIndex = nextInstructionOffset();
            iBranch(0);
        } else { // no fallthrough (break)
            fallthroughIndex = -1;
        }
        if (skipDefaultIndex != -1)
            patchInstruction(skipDefaultIndex, nextInstructionOffset() - skipDefaultIndex);
    }

    for (clauses = node->block->moreClauses; clauses != 0; clauses = clauses->next) {
        AST::CaseClause *clause = clauses->clause;
        if (skipIndex != -1)
            patchInstruction(skipIndex, nextInstructionOffset() - skipIndex);

        iDuplicate(); // expression
        clause->expression->accept(this);
        iStrictEqual();
        skipIndex = nextInstructionOffset();
        iBranchFalse(0); // next case

        if (fallthroughIndex != -1) // previous case falls through to here
            patchInstruction(fallthroughIndex, nextInstructionOffset() - fallthroughIndex);

        int breaksBefore = m_activeLoop->breakLabel.uses.count();
        if (clause->statements)
            clause->statements->accept(this);
        int breaksAfter = m_activeLoop->breakLabel.uses.count();
        if (breaksAfter == breaksBefore) { // fallthrough
            fallthroughIndex = nextInstructionOffset();
            iBranch(0);
        } else { // no fallthrough (break)
            fallthroughIndex = -1;
        }
    }

    if (skipIndex != -1) {
        patchInstruction(skipIndex, nextInstructionOffset() - skipIndex);
        if (defaultIndex != -1)
            iBranch(defaultIndex - nextInstructionOffset()); // goto default
    }

    if (fallthroughIndex != -1)
        patchInstruction(fallthroughIndex, nextInstructionOffset() - fallthroughIndex);

    // backpatch the breaks
    int term = nextInstructionOffset();
    foreach (int index, m_activeLoop->breakLabel.uses) {
        patchInstruction(index, term - index);
    }

    iPop(); // expression

    switchStatement(was);
    changeActiveLoop(previousLoop);
    m_loops.remove(node);
    return false;
}

bool Compiler::visit(AST::LabelledStatement *node)
{
    Loop *loop = findLoop(node->label);
    if (loop != 0) {
        m_compilationUnit.setValid(false);
        QString str = m_eng->toString(node->label);
        m_compilationUnit.setErrorMessage(QString::fromUtf8("duplicate label `%1'").arg(str));
        return false;
    }

    m_loops[node->statement].name = node->label;
    node->statement->accept(this);
    m_loops.remove(node->statement);
    return false;
}

bool Compiler::visit(AST::ExpressionStatement *node)
{
    if (node->expression)
        iLine(node->expression);
    return true;
}

void Compiler::endVisit(AST::ExpressionStatement *)
{
    if (topLevelCompiler())
        iSync();
    else
        iPop();
}

void Compiler::endVisit(AST::UnaryPlusExpression *)
{
    iUnaryPlus();
}

void Compiler::endVisit(AST::UnaryMinusExpression *)
{
    iUnaryMinus();
}

bool Compiler::visit(AST::ContinueStatement *node)
{
    iLine(node);
    return true;
}

void Compiler::endVisit(AST::ContinueStatement *node)
{
    int offset = nextInstructionOffset();
    iBranch(0);

    Loop *loop = findLoop(node->label);
    if (! loop) {
        m_compilationUnit.setErrorMessage(QString::fromUtf8("label not found"));
        m_compilationUnit.setValid(false);
        return;
    }

    loop->continueLabel.uses.append(offset);
}

bool Compiler::visit(AST::BreakStatement *node)
{
    iLine(node);
    return true;
}

void Compiler::endVisit(AST::BreakStatement *node)
{
    Loop *loop = findLoop(node->label);
    if (! loop) {
        m_compilationUnit.setErrorMessage(QString::fromUtf8("label not found"));
        m_compilationUnit.setValid(false);
        return;
    }

    if (m_generateLeaveWithOnBreak)
        iLeaveWith();
    int offset = nextInstructionOffset();
    iBranch(0);
    loop->breakLabel.uses.append(offset);
}

void Compiler::patchInstruction(int index, int offset)
{
    QScriptInstruction &i = m_instructions[index];

    switch (i.op) {
        case QScriptInstruction::OP_Branch:
        case QScriptInstruction::OP_BranchFalse:
        case QScriptInstruction::OP_BranchTrue:
            m_eng->newInteger(&i.operand[0], offset);
            break;

        default:
            Q_ASSERT_X(0, "Compiler::patchInstruction()", "expected a branch instruction");
            break;
    }
}

bool Compiler::visit(AST::WithStatement *node)
{
    iLine(node);
    node->expression->accept(this);
    iEnterWith();
    bool was = withStatement(true);
    bool was2 = generateLeaveOnBreak(m_iterationStatement);
    node->statement->accept(this);
    generateLeaveOnBreak(was2);
    withStatement(was);
    iLeaveWith();
    return false;
}

bool Compiler::visit(AST::ArrayLiteral *node)
{
    iNewArray();

    int length = 0;

    for (AST::ElementList *it = node->elements; it != 0; it = it->next) {
        for (AST::Elision *eit = it->elision; eit != 0; eit = eit->next) {
            iDuplicate();
            iLoadNumber(length);
            iMakeReference();
            iLoadUndefined();
            iAssign();
            iPop();
            ++length;
        }

        if (it->expression) {
            iDuplicate();
            iLoadNumber(length);
            iMakeReference();
            it->expression->accept(this);
            iAssign();
            iPop();
            ++length;
        }
    }

    for (AST::Elision *eit = node->elision; eit != 0; eit = eit->next) {
        iDuplicate();
        iLoadNumber(length);
        iMakeReference();
        iLoadUndefined();
        iAssign();
        iPop();
        ++length;
    }

    return false;
}

void Compiler::iLoadUndefined()
{
    pushInstruction(QScriptInstruction::OP_LoadUndefined);
}

void Compiler::iLoadThis()
{
    QScriptValue arg0;
    m_eng->newNameId(&arg0, m_eng->idTable()->id_this);
    pushInstruction(QScriptInstruction::OP_LoadThis, arg0);
}

void Compiler::iLoadNull()
{
    pushInstruction(QScriptInstruction::OP_LoadNull);
}

void Compiler::iLoadNumber(double number)
{
    QScriptValue arg0;
    m_eng->newNumber(&arg0, number);
    pushInstruction(QScriptInstruction::OP_LoadNumber, arg0);
}

void Compiler::iLoadString(QScriptNameIdImpl *id)
{
    QScriptValue arg0;
    id->persistent = true;
    m_eng->newNameId(&arg0, id);
    pushInstruction(QScriptInstruction::OP_LoadString, arg0);
}

void Compiler::iDuplicate()
{
    pushInstruction(QScriptInstruction::OP_Duplicate);
}

void Compiler::iResolve(QScriptNameIdImpl *id)
{
    QScriptValue arg0;
    id->persistent = true;
    m_eng->newNameId(&arg0, id);
    pushInstruction(QScriptInstruction::OP_Resolve, arg0);
}

void Compiler::iPutField()
{
    pushInstruction(QScriptInstruction::OP_PutField);
}

void Compiler::iCall(int argc)
{
    QScriptValue arg0;
    m_eng->newInteger(&arg0, argc);
    pushInstruction(QScriptInstruction::OP_Call, arg0);
}

void Compiler::iNew(int argc)
{
    QScriptValue arg0;
    m_eng->newInteger(&arg0, argc);
    pushInstruction(QScriptInstruction::OP_New, arg0);
}

void Compiler::iFetchField()
{
    pushInstruction(QScriptInstruction::OP_FetchField);
}

void Compiler::iFetchArguments()
{
    pushInstruction(QScriptInstruction::OP_FetchArguments);
}

void Compiler::iRet()
{
    pushInstruction(QScriptInstruction::OP_Ret);
}

void Compiler::iDeclareLocal(QScriptNameIdImpl *id)
{
    QScriptValue arg0;
    id->persistent = true;
    m_eng->newNameId(&arg0, id);
    pushInstruction(QScriptInstruction::OP_DeclareLocal, arg0);
}

void Compiler::iAssign()
{
    pushInstruction(QScriptInstruction::OP_Assign);
}

void Compiler::iBitAnd()
{
    pushInstruction(QScriptInstruction::OP_BitAnd);
}

void Compiler::iBitOr()
{
    pushInstruction(QScriptInstruction::OP_BitOr);
}

void Compiler::iBitXor()
{
    pushInstruction(QScriptInstruction::OP_BitXor);
}

void Compiler::iLeftShift()
{
    pushInstruction(QScriptInstruction::OP_LeftShift);
}

void Compiler::iMod()
{
    pushInstruction(QScriptInstruction::OP_Mod);
}

void Compiler::iRightShift()
{
    pushInstruction(QScriptInstruction::OP_RightShift);
}

void Compiler::iURightShift()
{
    pushInstruction(QScriptInstruction::OP_URightShift);
}

void Compiler::iAdd()
{
    pushInstruction(QScriptInstruction::OP_Add);
}

void Compiler::iDiv()
{
    pushInstruction(QScriptInstruction::OP_Div);
}

void Compiler::iEqual()
{
    pushInstruction(QScriptInstruction::OP_Equal);
}

void Compiler::iGreatOrEqual()
{
    pushInstruction(QScriptInstruction::OP_GreatOrEqual);
}

void Compiler::iGreatThan()
{
    pushInstruction(QScriptInstruction::OP_GreatThan);
}

void Compiler::iLessOrEqual()
{
    pushInstruction(QScriptInstruction::OP_LessOrEqual);
}

void Compiler::iLessThan()
{
    pushInstruction(QScriptInstruction::OP_LessThan);
}

void Compiler::iMul()
{
    pushInstruction(QScriptInstruction::OP_Mul);
}

void Compiler::iNotEqual()
{
    pushInstruction(QScriptInstruction::OP_NotEqual);
}

void Compiler::iSub()
{
    pushInstruction(QScriptInstruction::OP_Sub);
}

void Compiler::iStrictEqual()
{
    pushInstruction(QScriptInstruction::OP_StrictEqual);
}

void Compiler::iStrictNotEqual()
{
    pushInstruction(QScriptInstruction::OP_StrictNotEqual);
}

void Compiler::iBranch(int index)
{
    QScriptValue arg0;
    m_eng->newInteger(&arg0, index);
    pushInstruction(QScriptInstruction::OP_Branch, arg0);
}

void Compiler::iBranchFalse(int index)
{
    QScriptValue arg0;
    m_eng->newInteger(&arg0, index);
    pushInstruction(QScriptInstruction::OP_BranchFalse, arg0);
}

void Compiler::iBranchTrue(int index)
{
    QScriptValue arg0;
    m_eng->newInteger(&arg0, index);
    pushInstruction(QScriptInstruction::OP_BranchTrue, arg0);
}

void Compiler::iNewClosure(AST::FormalParameterList *formals, AST::Node *body)
{
    QScriptValue arg0;
    m_eng->newPointer(&arg0, formals);

    QScriptValue arg1;
    m_eng->newPointer(&arg1, body);

    pushInstruction(QScriptInstruction::OP_NewClosure, arg0, arg1);
}

void Compiler::iIncr()
{
    pushInstruction(QScriptInstruction::OP_Incr);
}

void Compiler::iDecr()
{
    pushInstruction(QScriptInstruction::OP_Decr);
}

void Compiler::iPop()
{
    pushInstruction(QScriptInstruction::OP_Pop);
}

void Compiler::iFetch(QScriptNameIdImpl *id)
{
    if (m_generateFastArgumentLookup) {
        int index = m_formals.indexOf(id);

        if (index != -1) {
            QScriptValue arg0;
            m_eng->newInteger(&arg0, index);
            pushInstruction(QScriptInstruction::OP_Receive, arg0);
            return;
        }
    }

    QScriptValue arg0;
    id->persistent = true;
    m_eng->newNameId(&arg0, id);
    pushInstruction(QScriptInstruction::OP_Fetch, arg0);
}

void Compiler::iLoadTrue()
{
    pushInstruction(QScriptInstruction::OP_LoadTrue);
}

void Compiler::iLoadFalse()
{
    pushInstruction(QScriptInstruction::OP_LoadFalse);
}

void Compiler::iUnaryMinus()
{
    pushInstruction(QScriptInstruction::OP_UnaryMinus);
}

void Compiler::iUnaryPlus()
{
    pushInstruction(QScriptInstruction::OP_UnaryPlus);
}

void Compiler::iPostIncr()
{
    pushInstruction(QScriptInstruction::OP_PostIncr);
}

void Compiler::iPostDecr()
{
    pushInstruction(QScriptInstruction::OP_PostDecr);
}

void Compiler::iNewArray()
{
    pushInstruction(QScriptInstruction::OP_NewArray);
}

void Compiler::iNewObject()
{
    pushInstruction(QScriptInstruction::OP_NewObject);
}

void Compiler::iTypeOf()
{
    pushInstruction(QScriptInstruction::OP_TypeOf);
}

void Compiler::iDelete()
{
    pushInstruction(QScriptInstruction::OP_Delete);
}

void Compiler::iInstanceOf()
{
    pushInstruction(QScriptInstruction::OP_InstanceOf);
}

void Compiler::iInplaceAnd()
{
    pushInstruction(QScriptInstruction::OP_InplaceAnd);
}

void Compiler::iInplaceSub()
{
    pushInstruction(QScriptInstruction::OP_InplaceSub);
}

void Compiler::iInplaceDiv()
{
    pushInstruction(QScriptInstruction::OP_InplaceDiv);
}

void Compiler::iInplaceAdd()
{
    pushInstruction(QScriptInstruction::OP_InplaceAdd);
}

void Compiler::iInplaceLeftShift()
{
    pushInstruction(QScriptInstruction::OP_InplaceLeftShift);
}

void Compiler::iInplaceMod()
{
    pushInstruction(QScriptInstruction::OP_InplaceMod);
}

void Compiler::iInplaceMul()
{
    pushInstruction(QScriptInstruction::OP_InplaceMul);
}

void Compiler::iInplaceOr()
{
    pushInstruction(QScriptInstruction::OP_InplaceOr);
}

void Compiler::iInplaceRightShift()
{
    pushInstruction(QScriptInstruction::OP_InplaceRightShift);
}

void Compiler::iInplaceURightShift()
{
    pushInstruction(QScriptInstruction::OP_InplaceURightShift);
}

void Compiler::iInplaceXor()
{
    pushInstruction(QScriptInstruction::OP_InplaceXor);
}

void Compiler::iThrow()
{
    pushInstruction(QScriptInstruction::OP_Throw);
}

void Compiler::iLine(AST::Node *node)
{
    if (! node)
        return;

    QScriptValue arg0;
    m_eng->newInteger(&arg0, node->startLine);

    QScriptValue arg1;
    m_eng->newInteger(&arg1, node->startColumn);

    pushInstruction(QScriptInstruction::OP_Line, arg0, arg1);
}

void Compiler::iBitNot()
{
    pushInstruction(QScriptInstruction::OP_BitNot);
}

void Compiler::iNot()
{
    pushInstruction(QScriptInstruction::OP_Not);
}

void Compiler::iNewRegExp(QScriptNameIdImpl *pattern)
{
    QScriptValue arg0;
    pattern->persistent = true;
    m_eng->newNameId(&arg0, pattern);
    pushInstruction(QScriptInstruction::OP_NewRegExp, arg0);
}

void Compiler::iNewRegExp(QScriptNameIdImpl *pattern, QScriptNameIdImpl *flags)
{
    QScriptValue arg0;
    pattern->persistent = true;
    m_eng->newNameId(&arg0, pattern);

    QScriptValue arg1;
    flags->persistent = true;
    m_eng->newNameId(&arg1, flags);

    pushInstruction(QScriptInstruction::OP_NewRegExp, arg0, arg1);
}

void Compiler::iNewEnumeration()
{
    pushInstruction(QScriptInstruction::OP_NewEnumeration);
}

void Compiler::iToFirstElement()
{
    pushInstruction(QScriptInstruction::OP_ToFirstElement);
}

void Compiler::iHasNextElement()
{
    pushInstruction(QScriptInstruction::OP_HasNextElement);
}

void Compiler::iNextElement()
{
    pushInstruction(QScriptInstruction::OP_NextElement);
}

void Compiler::iEnterWith()
{
    pushInstruction(QScriptInstruction::OP_EnterWith);
}

void Compiler::iLeaveWith()
{
    pushInstruction(QScriptInstruction::OP_LeaveWith);
}

void Compiler::iBeginCatch(QScriptNameIdImpl *id)
{
    QScriptValue arg0;
    id->persistent = true;
    m_eng->newNameId(&arg0, id);
    pushInstruction(QScriptInstruction::OP_BeginCatch, arg0);
}

void Compiler::iEndCatch()
{
    pushInstruction(QScriptInstruction::OP_EndCatch);
}

void Compiler::iSync()
{
    pushInstruction(QScriptInstruction::OP_Sync);
}

void Compiler::iHalt()
{
    pushInstruction(QScriptInstruction::OP_Halt);
}

void Compiler::iMakeReference()
{
    pushInstruction(QScriptInstruction::OP_MakeReference);
}

void Compiler::iIn()
{
    pushInstruction(QScriptInstruction::OP_In);
}

void Compiler::iNewString(QScriptNameIdImpl *id)
{
    QScriptValue arg0;
    id->persistent = true;
    m_eng->newNameId(&arg0, id);
    pushInstruction(QScriptInstruction::OP_NewString, arg0);
}

Compiler::Loop *Compiler::findLoop(QScriptNameIdImpl *name)
{
    if (! name)
        return m_activeLoop;

    QMap<AST::Statement*, Loop>::iterator it = m_loops.begin();

    for (; it != m_loops.end(); ++it) {
        Loop &loop = *it;

        if (loop.name == name)
            return &loop;
    }

    return 0;
}


} // namespace QScript
