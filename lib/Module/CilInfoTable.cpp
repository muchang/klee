#include "klee/Internal/Module/CilInfoTable.h"
#include "llvm/Support/raw_ostream.h"
#include <fstream>

using namespace klee;

class PTreeNode;

//**************************************************************************************************
/* A tool function split the string by the delim. */
std::vector<std::string> split(const std::string &s, char delim) {
	std::vector<std::string> elems;
    std::stringstream ss(s);
	std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

void Point::read (std::ifstream& fin) {
	fin >> var_name >> var_id >> var_line >> file_name >> func_name >> func_id >> stmt_id;
	std::string tmp_cutpoints;
	fin >> tmp_cutpoints;
	if(tmp_cutpoints != "no"){
		tmp_cutpoints = tmp_cutpoints.substr(0, tmp_cutpoints.length()-1);
		std::vector<std::string> sequenceList = split(tmp_cutpoints, ';');
		std::vector<std::string>::iterator it;
		for (it = sequenceList.begin() ; it != sequenceList.end(); ++it){
			Cutpoint cutpoint(*it);
			cutpoints.push_back(cutpoint);
		}
	}
}

bool DefUsePair::read (std::ifstream& fin){
	fin >> dua_id;
	if(fin.good()){

		std::string useType;
		fin >> useType;
		if(useType == "0")
			this->type = Cuse;
		else if(useType == "1")
			this->type = PTuse;
		else if(useType == "2")
			this->type = PFuse;
		
		def.read(fin);
		use.read(fin);

		std::string tmp_status;
		fin >> tmp_status;
		status = UnReach;
		print();
		return true;

	}
	else{
		return false;
	}
}
//**************************************************************************************************





//Print Functions
//**************************************************************************************************
void Point::print() {
	std::cerr << var_name << " " << var_id << " " << var_line << " " << file_name << " " << func_name << " " << func_id << " " << stmt_id << " " ;
	std::vector<Cutpoint>::iterator it;
	for (it = cutpoints.begin() ; it != cutpoints.end(); ++it){
		it->print();
	}
	std::cerr << "\n";
}

void Definition::print() {
	std::cerr << "Definition: ";
	Point::print();
}

void Use::print() {
	std::cerr << "Use: ";
	Point::print();
}

void Cutpoint::print() {
	std::cerr << "\nCutpoints: func_id:" << func_id << " " << "stmt_id:" << stmt_id << " "<< "var_line:" << var_line ;
}

void DefUsePair::print() {
	std::cerr << "\ndua_id: " << dua_id << "\n";
	std::cerr << "type: " << type << "\n";
	def.print();
	use.print();

	std::cerr << "status: ";
	switch(status) {
		case UnReach: std::cerr << "UnReach\n";break;
		case ReachDef: std::cerr << "ReachDef\n";break;
		case Covered: std::cerr << "Covered\n";break;
		default: std::cerr << "UnKnow\n";break;
	}
	 
	std::cerr << "\n";
}

void CilInfoTable::print () {
	std::vector<klee::DefUsePair>::iterator it;
	for(it = defUseList.begin(); it != defUseList.end(); ++it) {
		it->print();
	}
}
//**************************************************************************************************






//Initialize Functions
//**************************************************************************************************

Cutpoint::Cutpoint (std::string sequence) {
	std::vector<std::string> sequenceList = split(sequence,':');
	//need assert
	assert( sequenceList.size() == 4 );
	func_id = sequenceList[0];
	stmt_id = sequenceList[1];
	branch_choice = sequenceList[2];
	var_line = sequenceList[3];
}




CilInfoTable::CilInfoTable (std::string cilInfoFile, llvm::Module* module) {
	std::cout << "Work with DU Searcher!";
	std::ifstream fin(cilInfoFile.c_str());
	//Read def-use pairs from the file defined by def-use-file command line option.
	while(1){
		DefUsePair dupair;
		if(dupair.read(fin)){
			defUseList.push_back(dupair);
		}
		else{
			std::cout << "**************************************" << std::endl;
			break;
		}
	}
	fin.close();
	target = defUseList.begin();

	// init def-use map ith module
	for (llvm::Module::iterator fnIt = module->begin(); fnIt != module->end(); ++fnIt) {
		for (llvm::Function::iterator bbIt = fnIt->begin(); bbIt != fnIt->end(); ++bbIt) {
			for (llvm::BasicBlock::iterator it = bbIt->begin(); it != bbIt->end(); ++it) {
				if (isa<llvm::CallInst>(it)) {
					llvm::CallInst* cIn = cast<llvm::CallInst>(it);
					if ("df_stmt_monitor" == cIn->getCalledFunction()->getName().str()) {
						assert(4 == cIn->getNumArgOperands());
						int func_id = cast<llvm::ConstantInt>(cIn->getArgOperand(0))->getSExtValue(),
						    stmt_id = cast<llvm::ConstantInt>(cIn->getArgOperand(1))->getSExtValue(),
							// branch_choice = cast<llvm::ConstantInt>(cIn->getArgOperand(2))->getSExtValue(),
							stmt_line = cast<llvm::ConstantInt>(cIn->getArgOperand(3))->getSExtValue();
						setNodeInstruction(func_id, stmt_id, stmt_line, cIn);
					}
				}
			}
		}
	}
}
bool CilInfoTable::setNodeInstruction(int func_id, int stmt_id, int stmt_line, llvm::Instruction *inst) {
	// todo: maybe could add index for nodes
	for (std::vector<klee::DefUsePair>::iterator dfIt = defUseList.begin(); dfIt != defUseList.end(); ++dfIt) {
		if(dfIt->def.equals(func_id, stmt_id, stmt_line)) dfIt->def.inst = inst;
		if(dfIt->use.equals(func_id, stmt_id, stmt_line)) dfIt->use.inst = inst;
		for(std::vector<Cutpoint>::iterator dcIt = dfIt->def.cutpoints.begin(); dcIt != dfIt->def.cutpoints.end(); ++dcIt) {
			if(dcIt->equals(func_id, stmt_id, stmt_line)) dcIt->inst = inst;
		}
		for(std::vector<Cutpoint>::iterator ucIt = dfIt->use.cutpoints.begin(); ucIt != dfIt->use.cutpoints.end(); ++ucIt) {
			if(ucIt->equals(func_id, stmt_id, stmt_line)) ucIt->inst = inst;
		}
	}
	return true;
}
//**************************************************************************************************






//Updata Functions
//**************************************************************************************************
bool Node::equals (const KInstruction *kinstruction) {
	const char *line = var_line.c_str();
	if (std::strtoul (line, NULL, 0) == kinstruction->info->line)
		return true;
	else
		return false;
}
bool Node::equals (int func_id, int stmt_id, int stmt_line) {
	std::string s_func_id, s_stmt_id, s_stmt_line;
	// simple cast from int to std::string
	s_func_id += func_id;
	s_stmt_id += stmt_id;
	s_stmt_line += stmt_line;
	return this->func_id == s_func_id && this->stmt_id == s_stmt_id && this->var_line == s_stmt_line;
}

void DefUsePair::update(ExecutionState &state, KInstruction *kinstruction) {
    if(def.equals(kinstruction) && status != Covered){
        def.ptreeNode = state.ptreeNode;
		def.inst = kinstruction->inst;
		status = ReachDef;
    }
        
	// if the type of use is p-use 
    // then we print the first Instruction of branchs
	if(status == ReachDef && state.ptreeNode->isPosterityOf(def.ptreeNode)){
		if(type == Cuse && use.equals(kinstruction)){
			use.ptreeNode = state.ptreeNode;
			use.inst = kinstruction->inst;
			status = Covered;
		}
		else if(type == PTuse && use.equals(kinstruction) && kinstruction->inst->getOpcode() == llvm::Instruction::Br){
            llvm::User::op_iterator opi = kinstruction->inst->op_begin();
			if(llvm::BasicBlock *Bb = dyn_cast<llvm::BasicBlock>(*opi)) {
                llvm::BasicBlock::iterator inst = Bb->begin();
                use.inst = inst;
				type = Puse;
            }
        }
		else if(type == PFuse && use.equals(kinstruction) && kinstruction->inst->getOpcode() == llvm::Instruction::Br){
            llvm::User::op_iterator opi = kinstruction->inst->op_end();
			opi--;
			if(llvm::BasicBlock *Bb = dyn_cast<llvm::BasicBlock>(*opi)) {
                llvm::BasicBlock::iterator inst = Bb->begin();
				use.inst = inst;
				type = Puse;
            }
        }
		else if(type == Puse && use.inst == kinstruction->inst) {
			use.ptreeNode = state.ptreeNode;
			status = Covered;
		}
	} 
}

void CilInfoTable::update(ExecutionState &state, KInstruction *kinstruction) {
    for(std::vector<DefUsePair>::iterator it = defUseList.begin(); it != defUseList.end(); ++it)
        it->update(state, kinstruction);
}
//**************************************************************************************************









//Evaluate Functions
//**************************************************************************************************
bool Definition::withSameVariableAs (const Definition& _def) {
	if (this->file_name == _def.file_name &&
			this->func_id == _def.func_id &&
			this->func_name == _def.func_name &&
			this->var_id == _def.var_id &&
			this->var_name == _def.var_name &&
			this->stmt_id != _def.stmt_id)
		return true;
	else
		return false;
}

int Cutpoint::evaluate(const KInstruction *kinstruction) {
    if(equals(kinstruction))
        return 1;
    else
        return 0;
}


int klee::Use::evaluate(const KInstruction *kinstruction) {
    int value = 0;
    if(equals(kinstruction))
        value += 10;
    // if the type of use is p-use 
    // then we print the first Instruction of branchs 
    // if(type == Puse){
    //     std::vector<Branch>::iterator br;
    //     for(std::vector<Branch>::iterator br = branchs.begin(); br != branchs.end(); ++br) {
    //         if(kinstruction->inst == inst)
    //             value += 100;
    //     }
                
    // }
    std::vector<Cutpoint>::iterator cp;
    for(cp = cutpoints.begin(); cp != cutpoints.end(); ++cp) {
        value += cp->evaluate(kinstruction);
    }

    return value;
}

int Definition::evaluate(const KInstruction *kinstruction) {
    int value = 0;
    if(equals(kinstruction))
        value += 5;
    
    std::vector<Cutpoint>::iterator cp;
    for(cp = cutpoints.begin(); cp != cutpoints.end(); ++cp) {
       value += cp->evaluate(kinstruction);
    }
    return value;
}

int DefUsePair::evaluate (const KInstruction *kinstruction) {
	int value = 0;
    value = def.evaluate(kinstruction);

    // print use by iterator
    // std::vector<Use>::iterator use;
    // for(use = uselist.begin(); use != uselist.end(); ++use) {
    //     value += use->evaluate(kinstruction);
    // }
    value += use.evaluate(kinstruction);
    return value;
}

int CilInfoTable::evaluate (const ExecutionState *es) {
    for(std::vector<DefUsePair>::iterator it = defUseList.begin(); it != defUseList.end(); ++it) {
        if (it->def.equals(es->pc) && 
			it->def.withSameVariableAs(target->def) &&
			es->ptreeNode->isPosterityOf(target->def.ptreeNode)) {
			//Is redefine
			return -1;
		}
    }
    return target->evaluate(es->pc);
}
//**************************************************************************************************



bool CilInfoTable::setTarget(unsigned int dupairID) {
	for(target = defUseList.begin();target != defUseList.end();target++) {
		if (std::strtoul (target->dua_id.c_str(), NULL, 0) == dupairID) {
			return true;
		}
	}
	target = defUseList.begin();
    return false;
}
