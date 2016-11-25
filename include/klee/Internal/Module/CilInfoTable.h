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
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"

#include "llvm/Support/CallSite.h"

#include <string>
#include <vector>
#include <iostream>


namespace klee {

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

  struct Node {
    std::string func_id;
	  std::string stmt_id;
    std::string var_line;

    PTreeNode* ptreeNode;
    llvm::Instruction *inst;

    Node();
    bool equals (const KInstruction *kinstruction);
    bool blockEquals(const KInstruction *kinstruction);
    bool equals (int func_id, int stmt_id, int stmt_line);
  };

  struct Cutpoint : public Node{
    std::string branch_choice;

    Cutpoint(std::string);
    void print();
    int evaluate(const KInstruction *);
  };

  struct Point: public Node {
	  std::string var_name;
	  std::string var_id;
	  std::string file_name;
	  std::string func_name;
    std::vector<Cutpoint> cutpoints;

	  void print();
    void read(std::ifstream& fin);
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
    int evaluate(const ExecutionState *);
    bool update(ExecutionState &state, KInstruction *kinstruction);
    bool setTarget(unsigned int dupairID);
    bool coveredTarget();
    bool isTarget(const llvm::Instruction* inst); 
    bool setNodeInstruction(int func_id, int stmt_id, int branch_choice, int stmt_line, llvm::Instruction *inst);
  };

}
