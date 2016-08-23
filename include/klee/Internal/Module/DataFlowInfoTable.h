//===-- CilInfoTable.h ----------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include <string>
#include <vector>
#include <iostream>

#include "llvm/IR/Argument.h"
#include "klee/Internal/Module/KInstruction.h"
#include "../../lib/Core/PTree.h"
#include "klee/ExecutionState.h"

namespace llvm {
  class Function;
  class Instruction;
  class Module; 
}

namespace klee {
  enum UseType {
    Cuse,
    Puse
  };

  enum DefType {
    ArgDef,
    LocDef
  };

  struct CutPoint {
    llvm::BasicBlock *basicblock;
    llvm::Instruction *inst;
    void print();
    int evaluate(KInstruction *kinstruction);
  };

  struct DataFlowInstruction {
    llvm::Instruction *inst;
    std::vector<CutPoint> cutpoints;
    PTreeNode* ptreeNode;
    bool pass;

    DataFlowInstruction(){pass = false;};
    void dominatorAnalysis(llvm::Module* m);
    void print();
  };

  struct Branch : DataFlowInstruction {
    void print();
  };

  struct Definition : DataFlowInstruction  {
    llvm::Argument *arg; 
    DefType type;

    void print();
    int evaluate(KInstruction *kinstruction);
  };

  struct Use : DataFlowInstruction  {
    std::vector<Branch> branchs;
    UseType type;
    
    void print();
    int evaluate(KInstruction *kinstruction);
  };

  struct DefUseChain {
    Definition definition;
    std::vector<Use> uselist;
    std::vector<Use>::iterator target;
    bool pass;

    DefUseChain(){pass = false;}
    void print();
    void update(ExecutionState &state, KInstruction *kinstruction);
    int evaluate(KInstruction *kinstruction);
    bool stepTarget();
  };

  class DataFlowInfoTable {
  private:
	  std::vector<DefUseChain> defuseSet;
    std::vector<DefUseChain>::iterator target;

  public:
    DataFlowInfoTable(llvm::Module *m);
    ~DataFlowInfoTable();
    void printDefUseSet();
    void update(ExecutionState &state, KInstruction *kinstruction);

    // Evaluate the instruction value
    // (the more def-use pair instruction could guide to the high value it will get)
    int evaluate(KInstruction *kinstruction);
    bool stepTarget();
    bool targetPass();
  };

}
