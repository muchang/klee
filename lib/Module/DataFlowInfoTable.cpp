#include "klee/Internal/Module/DataFlowInfoTable.h"

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Module.h"

#include "llvm/Analysis/Dominators.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

using namespace llvm;
using namespace klee;

void CutPoint::print() {
    errs() << "\e[35mcutpoint:" << inst << "\e[0m\n";
}

int CutPoint::evaluate(KInstruction *kinstruction) {
    if(kinstruction->inst == inst)
        return 10;
    else
        return 0;
}

void DataFlowInstruction::dominatorAnalysis(llvm::Module* m) {
     for (Module::iterator fnIt = m->begin(), fn_ie = m->end(); 
        fnIt != fn_ie; ++fnIt) {
            llvm::Function* F = fnIt;
            DominatorTree* DT=new DominatorTree();
            DT->runOnFunction(*F);

            BasicBlock* BB = inst->getParent();
            DomTreeNodeBase<BasicBlock> *IDomA = DT->getNode(BB);
            while(IDomA) {
                CutPoint cp;
                cp.basicblock = IDomA->getBlock();
                errs() << "\e[34mBB:" << *cp.basicblock << "\e[0m\n";
                cp.inst = cp.basicblock->begin();
                errs() << "\e[35mFirst Inst:" << *cp.inst << "\e[0m\n\n";
                IDomA = IDomA->getIDom();
                cutpoints.push_back(cp);
            }
    }
}

void Branch::print() {
    errs() << "\e[36mBr:" << inst;
    if(pass == true)
        errs() << "   Pass\e[0m\n";
    else    
        errs() << "   Failed\e[0m\n";
}

void klee::Use::print() {
    errs() << "\e[33mI:" << inst;
    if(pass == true)
        errs() << "   pass\e[0m\n";
    else    
        errs() << "   Failed\e[0m\n";
    // if the type of use is p-use 
    // then we print the first Instruction of branchs 
    if(type == Puse){
        std::vector<Branch>::iterator br;
        for(br = branchs.begin(); br != branchs.end(); ++br) {
            br->print();
        }
                
    }
    std::vector<CutPoint>::iterator cp;
    for(cp = cutpoints.begin(); cp != cutpoints.end(); ++cp) {
        cp->print();
    }
}

int klee::Use::evaluate(KInstruction *kinstruction) {
    int value = 0;
    if(kinstruction->inst == inst)
        value += 50;
    // if the type of use is p-use 
    // then we print the first Instruction of branchs 
    if(type == Puse){
        std::vector<Branch>::iterator br;
        for(std::vector<Branch>::iterator br = branchs.begin(); br != branchs.end(); ++br) {
            if(kinstruction->inst == inst)
                value += 100;
        }
                
    }
    std::vector<CutPoint>::iterator cp;
    for(cp = cutpoints.begin(); cp != cutpoints.end(); ++cp) {
        value += cp->evaluate(kinstruction);
    }

    return value;
}

void Definition::print() {
    // print definition
    if(type == LocDef)
        errs() << "\e[34mI:" << inst << "\e[0m\n";
    else if(type == ArgDef)
        errs() << "\e[34mI:" << arg << "\e[0m\n";
    
    std::vector<CutPoint>::iterator cp;
    for(cp = cutpoints.begin(); cp != cutpoints.end(); ++cp) {
       cp->print();
    }
}

int Definition::evaluate(KInstruction *kinstruction) {
    int value = 0;
    if(type == LocDef && kinstruction->inst == inst)
        value += 50;
    
    std::vector<CutPoint>::iterator cp;
    for(cp = cutpoints.begin(); cp != cutpoints.end(); ++cp) {
       value += cp->evaluate(kinstruction);
    }
    return value;
}

void DefUseChain::print() {
    definition.print();

    // print use by iterator
    std::vector<Use>::iterator use;
    for(use = uselist.begin(); use != uselist.end(); ++use) {
        use->print();
    }
}

// I don't split update function into definition.update & use.update
// because use.update should use the ptreeNode of definition on the defuse chain.
void DefUseChain::update(ExecutionState &state, KInstruction *kinstruction) {

    if(kinstruction->inst == definition.inst){
        definition.ptreeNode = state.ptreeNode;
    }

    std::vector<Use>::iterator use;
    for(use = uselist.begin(); use != uselist.end(); ++use) {
        // if the type of use is p-use 
        // then we print the first Instruction of branchs 
        if(use->type == Puse) {
            if(use->inst == kinstruction->inst)
                use->ptreeNode = state.ptreeNode;
            std::vector<Branch>::iterator br;
            for(br = use->branchs.begin(); br != use->branchs.end(); ++br){
                if(kinstruction->inst == br->inst && state.ptreeNode->isPosterityOf(use->ptreeNode)){
                    br->pass = true; 
                }
            }
            for(br = use->branchs.begin(); br != use->branchs.end(); ++br){
                if(br->pass == true){
                    use->pass = true;
                }
                else{
                    use->pass = false;
                    break;
                }
                    
            }
        }
        else if(use->type == Cuse && 
                use->inst == kinstruction->inst && 
                (definition.type == ArgDef || state.ptreeNode->isPosterityOf(definition.ptreeNode)))
            use->pass = true;
    }
}

int DefUseChain::evaluate(KInstruction *kinstruction) {
    int value = 0;
    value += definition.evaluate(kinstruction);

    // print use by iterator
    // std::vector<Use>::iterator use;
    // for(use = uselist.begin(); use != uselist.end(); ++use) {
    //     value += use->evaluate(kinstruction);
    // }
    value += target->evaluate(kinstruction);
    return value;
}

bool DefUseChain::stepTarget() {
    target++;
    if(target == uselist.end()) {
        errs() << "?????????????????\n";
        return false;
    }
    else {
        return true;
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
                duchain.definition.dominatorAnalysis(m);
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
                        use.dominatorAnalysis(m);
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
  target = defuseSet.begin();
  target->target = target->uselist.begin();
}

void DataFlowInfoTable::printDefUseSet(){
    int total = 0;
    int pass = 0;
    for(std::vector<DefUseChain>::iterator it = defuseSet.begin(); it != defuseSet.end(); ++it) {
        //it->print();
        std::vector<Use>::iterator use;
        for(use = it->uselist.begin(); use != it->uselist.end(); ++use) {
            total = total + 1;
            if(use->pass == true)
                pass = pass + 1;
        }
    }
    errs() << "Cover Rate:" << pass << "/" << total << "\n";
       
}

void DataFlowInfoTable::update(ExecutionState &state, KInstruction *kinstruction) {
    for(std::vector<DefUseChain>::iterator it = defuseSet.begin(); it != defuseSet.end(); ++it)
        it->update(state, kinstruction);
}

int DataFlowInfoTable::evaluate(KInstruction *kinstruction) {
    int value = 0;
    // for(std::vector<DefUseChain>::iterator it = defuseSet.begin(); it != defuseSet.end(); ++it) {
    //     value += it->evaluate(kinstruction);
    // }
    target->evaluate(kinstruction);
    return value; 
}

bool DataFlowInfoTable::targetPass() {
    if(target->target->pass == true){
        errs() << target->target->inst << "\n";
        return true;
    }    
    else {
        return false;
    }   
}

bool DataFlowInfoTable::stepTarget() {
    if(target->stepTarget()) {
        return true;
    }
    else {
        target++;
        if(target == defuseSet.end())
            return false;
        else {
            target->target = target->uselist.begin();
            return true;
        }
    }
}