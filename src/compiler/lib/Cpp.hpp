/*****************************************************************************

YASK: Yet Another Stencil Kernel
Copyright (c) 2014-2017, Intel Corporation

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

////////// Support for YASK C++ scalar and vector-code generation //////////////

// NB: This file does *not* support vector-code with intrinsics; see
// CppIntrin.hpp for that.

#ifndef CPP_HPP
#define CPP_HPP

#include "Vec.hpp"
#include "Grid.hpp"

namespace yask {

    /////////// Scalar code /////////////

    // Outputs C++ scalar code for YASK.
    class CppPrintHelper : public PrintHelper {

    public:
        CppPrintHelper(const CounterVisitor* cv,
                       const string& varPrefix,
                       const string& varType,
                       const string& linePrefix,
                       const string& lineSuffix) :
            PrintHelper(cv, varPrefix, varType,
                        linePrefix, lineSuffix) { }
        virtual ~CppPrintHelper() { }

        // Format a real, preserving precision.
        static string formatReal(double v);
    
        // Return a constant expression.
        // This is overloaded to preserve precision.
        virtual string addConstExpr(ostream& os, double v) {
            return formatReal(v);
        }

        // Make call for a point.
        // This is a utility function used for both reads and writes.
        virtual string makePointCall(const GridPoint& gp,
                                     const string& fname,
                                     string optArg = "") const;
    
        // Return a grid-point reference.
        virtual string readFromPoint(ostream& os, const GridPoint& gp);

        // Return code to update a grid point.
        virtual string writeToPoint(ostream& os, const GridPoint& gp,
                                    const string& val);
    };

    /////////// Vector code /////////////

    // Output generic C++ vector code for YASK.
    class CppVecPrintHelper : public VecPrintHelper {

    public:
        CppVecPrintHelper(VecInfoVisitor& vv,
                          bool allowUnalignedLoads,
                          Dimensions& dims,
                          const CounterVisitor* cv,
                          const string& varPrefix,
                          const string& varType,
                          const string& linePrefix,
                          const string& lineSuffix) :
            VecPrintHelper(vv, allowUnalignedLoads, dims, cv,
                           varPrefix, varType, linePrefix, lineSuffix) { }

    protected:

        // Vars for tracking pointers to grid values.
        map<GridPoint, string> _vecPtrs; // pointers to grid vecs. value: ptr-var name.
        map<string, int> _ptrOfsLo; // lowest read offset from _vecPtrs in inner dim.
        map<string, int> _ptrOfsHi; // highest read offset from _vecPtrs in inner dim.

        // Element indices.
        string _elemSuffix = "_elem";
        VarMap _varMap; // maps vector indices to elem indices; filled by printElemIndices.
        
        // A simple constant.
        virtual string addConstExpr(ostream& os, double v) {
            return CppPrintHelper::formatReal(v);
        }

        // Any code.
        virtual string addCodeExpr(ostream& os, const string& code) {
            return code;
        }

        // Print a comment about a point.
        // This is a utility function used for both reads and writes.
        virtual void printPointComment(ostream& os, const GridPoint& gp,
                                       const string& verb) const {

            os << endl << " // " << verb << " vector starting at " <<
                gp.makeStr() << "." << endl;
        }

        // Print call for a vectorized point.
        // This is a utility function used for both reads and writes.
        virtual void printVecPointCall(ostream& os,
                                       const GridPoint& gp,
                                       const string& funcName,
                                       const string& firstArg,
                                       const string& lastArg,
                                       bool isNorm) const;
    
        // Print aligned memory read.
        virtual string printAlignedVecRead(ostream& os, const GridPoint& gp);

        // Print unaliged memory read.
        // Assumes this results in same values as printUnalignedVec().
        virtual string printUnalignedVecRead(ostream& os, const GridPoint& gp);

        // Print aligned memory write.
        virtual string printAlignedVecWrite(ostream& os, const GridPoint& gp,
                                            const string& val);
    
        // Print conversion from memory vars to point var gp if needed.
        // This calls printUnalignedVecCtor(), which can be overloaded
        // by derived classes.
        virtual string printUnalignedVec(ostream& os, const GridPoint& gp);

        // Print per-element construction for one point var pvName from elems.
        virtual void printUnalignedVecSimple(ostream& os, const GridPoint& gp,
                                             const string& pvName, string linePrefix,
                                             const set<size_t>* doneElems = 0);

        // Read from a single point to be broadcast to a vector.
        // Return code for read.
        virtual string readFromScalarPoint(ostream& os, const GridPoint& gp,
                                           const VarMap* vMap=0);

        // Read from multiple points that are not vectorizable.
        // Return var name.
        virtual string printNonVecRead(ostream& os, const GridPoint& gp);
        
        // Print construction for one point var pvName from elems.
        // This version prints inefficient element-by-element assignment.
        // Override this in derived classes for more efficient implementations.
        virtual void printUnalignedVecCtor(ostream& os, const GridPoint& gp, const string& pvName) {
            printUnalignedVecSimple(os, gp, pvName, _linePrefix);
        }
        
    public:

        // Print code to set pointers of aligned reads.
        virtual void printBasePtrs(ostream& os);

        // Make base point (inner-dim index = 0).
        virtual GridPointPtr makeBasePoint(const GridPoint& gp) {
            GridPointPtr bgp = gp.cloneGridPoint();
            IntScalar idi(getDims()._innerDim, 0); // set inner-dim index to 0.
            bgp->setArgConst(idi);
            return bgp;
        }

        // Print prefetches for each base pointer.
        // Print only 'ptrVar' if provided.
        virtual void printPrefetches(ostream& os, bool ahead, string ptrVar = "");

        // Print any needed memory reads and/or constructions to 'os'.
        // Return code containing a vector of grid points.
        virtual string readFromPoint(ostream& os, const GridPoint& gp);
        
        // Print any immediate memory writes to 'os'.
        // Return code to update a vector of grid points or null string
        // if all writes were printed.
        virtual string writeToPoint(ostream& os, const GridPoint& gp,
                                    const string& val);
        
        // print init of un-normalized indices.
        virtual void printElemIndices(ostream& os);

        // Print code to set ptrName to gp.
        virtual void printPointPtr(ostream& os, const string& ptrName, const GridPoint& gp);
        
        // Access cached values.
        virtual void savePointPtr(const GridPoint& gp, string var) {
            _vecPtrs[gp] = var;
        }
        virtual string* lookupPointPtr(const GridPoint& gp) {
            if (_vecPtrs.count(gp))
                return &_vecPtrs.at(gp);
            return 0;
        }
    };

    // Outputs the variables needed for an inner loop.
    class CppLoopVarPrintVisitor : public PrintVisitorBase {
    protected:
        CppVecPrintHelper& _cvph;
        
    public:
        CppLoopVarPrintVisitor(ostream& os,
                               CppVecPrintHelper& ph,
                               CompilerSettings& settings,
                               const VarMap* varMap = 0) :
            PrintVisitorBase(os, ph, settings, varMap),
            _cvph(ph) { }

        // A grid access.
        virtual void visit(GridPoint* gp);
    };
    
    // Print out a stencil in C++ form for YASK.
    class YASKCppPrinter : public PrinterBase {
    protected:
        EqGroups& _clusterEqGroups;
        Dimensions& _dims;
        string _context, _context_base;

        // Print an expression as a one-line C++ comment.
        void addComment(ostream& os, EqGroup& eq);

        // A factory method to create a new PrintHelper.
        // This can be overridden in derived classes to provide
        // alternative PrintHelpers.
        virtual CppVecPrintHelper* newCppVecPrintHelper(VecInfoVisitor& vv,
                                                        CounterVisitor& cv) {
            return new CppVecPrintHelper(vv, _settings._allowUnalignedLoads, _dims, &cv,
                                         "temp", "real_vec_t", " ", ";\n");
        }

        // Print extraction of indices.
        virtual void printIndices(ostream& os) const;
        
        // Print a shim function to map hard-coded YASK vars to actual dims.
        virtual void printShim(ostream& os,
                               const string& fname,
                               bool use_template = false);

        // Print pieces of YASK output.
        virtual void printMacros(ostream& os);
        virtual void printData(ostream& os);
        virtual void printEqGroups(ostream& os);
        virtual void printContext(ostream& os);
        
        
    public:
        YASKCppPrinter(StencilSolution& stencil,
                       EqGroups& eqGroups,
                       EqGroups& clusterEqGroups,
                       Dimensions& dims) :
            PrinterBase(stencil, eqGroups),
            _clusterEqGroups(clusterEqGroups),
            _dims(dims)
        {
            // name of C++ struct.
            _context = "StencilContext_" + _stencil.getName();
            _context_base = _context + "_data";
        }
        virtual ~YASKCppPrinter() { }

        // Output all code for YASK.
        virtual void print(ostream& os);
    };

} // namespace yask.

#endif
