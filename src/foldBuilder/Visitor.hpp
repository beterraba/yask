/*****************************************************************************

YASK: Yet Another Stencil Kernel
Copyright (c) 2014-2016, Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to
deal in the Software without restriction, including without limitation the
rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

* The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

*****************************************************************************/

//////////// Generic Visitor class /////////////

#ifndef VISITOR_HPP
#define VISITOR_HPP

#include "Expr.hpp"

using namespace std;

// Base class for an Expr-tree visitor.
class ExprVisitor {
public:
    virtual ~ExprVisitor() { }

    // By default, leaf-node visitors do nothing.
    virtual void visit(ConstExpr* ce) { }
    virtual void visit(CodeExpr* ce) { }
    virtual void visit(GridPoint* gp) { }

    // By default, a unary visitor just visits its operand.
    virtual void visit(UnaryExpr* ue) {
        ue->getRhs()->accept(this);
    }

    // By default, a binary visitor just visits its operands.
    virtual void visit(BinaryExpr* be) {
        be->getLhs()->accept(this);
        be->getRhs()->accept(this);
    }

    // By default, an equality visitor just visits its operands.
    virtual void visit(EqualsExpr* be) {
        be->getLhs()->accept(this);
        be->getRhs()->accept(this);
    }

    // By default, a commutative visitor just visits its operands.
    virtual void visit(CommutativeExpr* ce) {
        ExprPtrVec& ops = ce->getOps();
        for (auto ep : ops) {
            ep->accept(this);
        }
    }
};

// A visitor that counts FP ops.
// TODO: count the type of each.
class FpOpCounterVisitor : public ExprVisitor {

    int _numOps;
    
public:
    FpOpCounterVisitor() : _numOps(0) { }

    int getNumOps() const { return _numOps; }

    // Count as one op and visit operand.
    virtual void visit(UnaryExpr* ue) {
        _numOps++;
        ue->getRhs()->accept(this);
    }

    // Count as one op and visit operands.
    virtual void visit(BinaryExpr* be) {
        _numOps++;
        be->getLhs()->accept(this);
        be->getRhs()->accept(this);
    }

    // Count as one op between each operand and visit operands.
    virtual void visit(CommutativeExpr* ce) {
        ExprPtrVec& ops = ce->getOps();
        _numOps += ops.size() - 1;
        for (auto ep : ops) {
            ep->accept(this);
        }
    }
};

#endif