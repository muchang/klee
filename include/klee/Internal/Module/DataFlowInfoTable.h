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

  struct Branch {
    llvm::Instruction *inst;
  };

  struct Definition  {
	  llvm::Instruction *inst;
    llvm::Argument *arg; 
    DefType type;
  };

  struct Use  {
	  llvm::Instruction *inst;
    std::vector<Branch> branchs;
    UseType type;
  };

  struct DefUseChain {
    Definition definition;
    std::vector<Use> uselist;

  public:
    void print();
    bool updateState(klee::KInstruction *kinstruction);
  };

  class DataFlowInfoTable {
  private:
	  std::vector<DefUseChain> defuseSet;
  public:
    DataFlowInfoTable(llvm::Module *m);
    ~DataFlowInfoTable();
    void printDefUseSet();
    bool updateState(klee::KInstruction *kinstruction)
  };

}
