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
    ReachCp,
    Covered
  };

  struct Node {
    std::string func_id;
	  std::string stmt_id;
    std::string var_line;

    std::vector<PTreeNode*> ptreeNodes;
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
    int evaluate(const ExecutionState *es);
  };

  struct Point: public Node {
	  std::string var_name;
	  std::string var_id;
	  std::string file_name;
	  std::string func_name;
    std::vector<Cutpoint> cutpoints;
    std::vector<Cutpoint>::iterator cp_index;

	  void print();
    void read(std::ifstream& fin);
    void write(std::ofstream& fout);
  };

  struct Definition : public Point {
	  bool withSameVariableAs(const Definition& );
	  void print();
    int evaluate(const ExecutionState *es);
  };

  struct Use : public Point {
	  void print();
    int evaluate(const ExecutionState *es);
  };

  struct DefUsePair {
    std::string dua_id;
    Definition def;
    Use use;
    UseType type;
    std::vector<PTreeNode*> redefine_candidates;

    DupairStatus status;
    bool checkRedefine(const Use&);
    bool read(std::ifstream&);
    void write(std::ofstream&);
    void print();
    bool equalKind(const Use&, const Cutpoint&);
    int evaluate(const ExecutionState *es);
    DupairStatus update(const ExecutionState &state, const KInstruction *kinstruction);
    void addRedefineCandidate(const ExecutionState &state);
  };

  class CilInfoTable {

  private:
	  std::vector<DefUsePair> defUseList;
    std::vector<DefUsePair>::iterator target;
    std::string cilinfofile;
    bool needCompute;

  public:
	  CilInfoTable(std::string, llvm::Module *);
    ~CilInfoTable();

    unsigned getSize() const;
    /*Print the content of the CilInfoTable*/
    void print();
    /*Update defUsePair in CilInfoTable when meet klee_cil_info function*/
    int evaluate(const ExecutionState *es);
    bool update(ExecutionState &state, KInstruction *kinstruction);
    bool setTarget(unsigned int dupairID);
    bool coveredTarget();
    int isCutpoint(const llvm::Instruction* inst); 
    int isDefUse(const llvm::Instruction* inst);
    bool setNodeInstruction(int func_id, int stmt_id, int branch_choice, int stmt_line, llvm::Instruction *inst);
    bool shouldCompute();  
  };

}
