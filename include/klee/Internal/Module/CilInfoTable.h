//===-- CilInfoTable.h ----------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include "../../lib/Core/PTree.h"
#include "../../lib/Core/Common.h"

#include "klee/Internal/Module/KInstruction.h"
#include "klee/Internal/Module/InstructionInfoTable.h"
#include "klee/ExecutionState.h"

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instructions.h"

#include <string>
#include <vector>
#include <iostream>


namespace klee {
  struct Cutpoint;

  enum UseType {
    Cuse,     //C-use
    PTuse,    //P-true-use
    PFuse,    //P-false-use
    Puse
  };

  enum DupairStatus {
    UnReach,
    ReachDef,
    Covered
  };

  struct Point {
	  std::string var_name;
	  std::string var_id;
	  std::string var_line;
	  std::string file_name;
	  std::string func_name;
	  std::string func_id;
	  std::string stmt_id;
    std::vector<Cutpoint> cutpoints;

    PTreeNode* ptreeNode;
    llvm::Instruction *inst;

	  void print();
    void read(std::ifstream& fin);
    bool equals (const KInstruction *kinstruction);
  };

  struct Definition : public Point {
	  bool withSameVariableAs(const Definition& );
	  void print();
    int evaluate(const KInstruction *);
  };

  struct Use : public Point {
	  void print();
    int evaluate(const KInstruction *);
  };

  struct Cutpoint : public Point {
    std::string kind;

    Cutpoint(std::string);
    void print();
    int evaluate(const KInstruction *);
  };

  struct DefUsePair {
    std::string dua_id;
    Definition def;
    Use use;
    UseType type;

    DupairStatus status;
    bool checkRedefine(const Use&);
    bool read(std::ifstream& );
    void print();
    bool equalKind(const Use&, const Cutpoint&);
    int evaluate(const KInstruction *);
    void update(ExecutionState &state, KInstruction *kinstruction);
  };

  class CilInfoTable {

  private:
	  std::vector<DefUsePair> defUseList;
    std::vector<DefUsePair>::iterator target;

  public:
	  CilInfoTable(std::string, llvm::Module *);

    unsigned getSize() const;
    /*Print the content of the CilInfoTable*/
    void print();
    /*Update defUsePair in CilInfoTable when meet klee_cil_info function*/
    int evaluate(const ExecutionState &, const KInstruction *);
    void update(ExecutionState &state, KInstruction *kinstruction);
    bool stepTarget();
  };

}
