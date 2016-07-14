#include "klee/Internal/Module/DataFlowInfoTable.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Module.h"

#include "llvm/Support/InstIterator.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

using namespace llvm;
using namespace klee;

void DefUseChain::print() {
    // print definition
    if(definition.type == LocDef)
        errs() << "\e[34mI:" << *definition.inst << "\e[0m\n";
    else if(definition.type == ArgDef)
        errs() << "\e[34mI:" << *definition.arg << "\e[0m\n";

    // print use by iterator
    std::vector<Use>::iterator it;
    for(it = uselist.begin(); it != uselist.end(); ++it) {
        errs() << "\e[33mI:" << *(*it).inst << "\e[0m\n";
        // if the type of use is p-use 
        // then we print the first Instruction of branchs 
        if(it->type == Puse){
            std::vector<Branch>::iterator br;
            for(br = it->branchs.begin(); br != it->branchs.end(); ++br)
                errs() << "\e[35mBr:" << *(*br).inst << "\e[0m\n";
        }
    }
}

DataFlowInfoTable::DataFlowInfoTable(llvm::Module *m) {
  for (Module::iterator fnIt = m->begin(), fn_ie = m->end(); 
        fnIt != fn_ie; ++fnIt) {

    for (inst_iterator I = inst_begin(fnIt), ie = inst_end(fnIt); I != ie;++I) {
        errs() << "\e[34mI:" << *I << "\e[0m\n";

        // Analysis each variables of the Instruction.
        for (User::op_iterator oi = I->op_begin(), e = I->op_end(); oi != e; ++oi) {
            
            Value *v = *oi;
            DefUseChain duchain;
            bool pass = false;

            // Deal with the definition 
            // local definition record in member variable inst
            if(duchain.definition.inst = dyn_cast<Instruction>(*oi)) {
                duchain.definition.type = LocDef;
                pass = true;
                errs() << "\e[32mV:" << *v << "\e[0m\n";
            }
            // definition from argument record in member variable arg
            else if(duchain.definition.arg = dyn_cast<Argument>(*oi)){
                duchain.definition.type = ArgDef;
                pass = true;
                errs() << "\e[35mV:" << *v << "\e[0m\n";
            }

            // find the use of the definition
            if(pass == true){
                for (Value::use_iterator ui = v->use_begin(), e = v->use_end(); ui != e; ++ui){
                    if (Instruction *In = dyn_cast<Instruction>(*ui)) {
                        Use use;
                        use.inst = In;
                        // We should push the first instruction of the two branch into the branchs list
                        // when we meet p-use instruction use key word Br.
                        if(In->getOpcode() == Instruction::Br) {
                            use.type = Puse;
                            errs() << "  is used in instruction:\n";
                            errs() << "\e[40mB:" << *In << "\e[0m\n";
                            for (User::op_iterator opi = In->op_begin(), e = In->op_end(); opi != e; ++opi) {
                                if(BasicBlock *Bb = dyn_cast<BasicBlock>(*opi)) {
                                    Branch branch;
                                    errs() << "\e[40mB:" << *Bb << "\e[0m\n";
                                    BasicBlock::iterator inst = Bb->begin();
                                    branch.inst = inst;
                                    use.branchs.push_back(branch);
                                    errs() << "\e[33m" << *inst << "\e[0m\n";
                                }
                            }
                        }
                        // Otherwise we simplily push the c-use instruction
                        // into the uselist.
                        else {
                            use.type = Cuse;
                            errs() << "  is used in instruction:\n";
                            errs() << "\e[33m" << *In << "\e[0m\n";
                        }
                        duchain.uselist.push_back(use);
                    }
                }
                defuseSet.push_back(duchain); 
            }
        }  // User::op_iterator 
        errs() << "\n";
        errs() << "*********************************************\n";
    }  // inst_iterator
  } // Module::iterator
  printDefUseSet();
}

void DataFlowInfoTable::printDefUseSet(){
    std::vector<DefUseChain>::iterator it;
    for(it = defuseSet.begin(); it != defuseSet.end(); ++it) 
        it->print();
}