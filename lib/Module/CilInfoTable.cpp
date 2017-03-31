#include "klee/Internal/Module/CilInfoTable.h"
#include "llvm/Support/raw_ostream.h"
#include <fstream>
#include <sstream>

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
std::string int2str (int i) {
	std::stringstream ss;
	ss << i;
	return ss.str();
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
		cp_index = cutpoints.begin();
	}
	else {
		cp_index = cutpoints.end();
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
		if(tmp_status == "0")
			status = UnReach;
		else if(tmp_status == "1")
			status = ReachDef;
		else if(tmp_status == "2")
			status = Covered;
		return true;

	}
	else{
		return false;
	}
}

void Point::write (std::ofstream& fout) {
	fout << var_name << " " << var_id << " " << var_line << " " << file_name << " " << func_name << " " << func_id << " " << stmt_id << " ";
    std::vector<Cutpoint>::iterator it;
	it = cutpoints.begin();
	if(it == cutpoints.end()){
		fout << "no ";
		return ;
	}	
	else
		fout << it->func_id << ":" <<  it->stmt_id << ":" << it->branch_choice << ":" << it->var_line;
	for ( ++it; it != cutpoints.end(); ++it){
		fout << ";" << it->func_id << ":" <<  it->stmt_id << ":" << it->branch_choice << ":" << it->var_line ;
	}
	fout << "# ";
}

void DefUsePair::write (std::ofstream& fout){
	fout << dua_id << " ";
	if(this->type == Cuse)
		fout << "0 ";
	else if(this->type == PTuse)
		fout << "1 ";
	else if(this->type ==PFuse)
		fout << "2 ";
	
	def.write(fout);
	use.write(fout);

	if(status == UnReach)
		fout << "0\n";
	else if(status == ReachDef)
		fout << "1\n";
	else if(status == Covered)
		fout << "2\n";
}
//**************************************************************************************************





//Print Functions
//**************************************************************************************************
void Point::print() {
	std::cerr << var_name << " " << var_id << " " << var_line << " " << file_name << " " << func_name << " " << func_id << " " << stmt_id << " " ;
	if(inst) llvm::errs() << *inst;
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
	std::cerr << "\nCutpoints: func_id:" << func_id << " " << "stmt_id:" << stmt_id << " "<< "var_line:" << var_line;
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
Node::Node() {
	this->inst = 0;
}

Cutpoint::Cutpoint (std::string sequence) {
	std::vector<std::string> sequenceList = split(sequence,':');
	//need assert
	assert( sequenceList.size() == 4 );
	func_id = sequenceList[0];
	stmt_id = sequenceList[1];
	branch_choice = sequenceList[2];
	var_line = sequenceList[3];
}
CilInfoTable::~CilInfoTable() {
	std::cout << target->dua_id << ",";
	switch(target->status) {
		case UnReach: std::cout << "0,";break; 
		case ReachDef: std::cout << "1,";break;
		case Covered: std::cout << "2,";break;
		default: std::cout << "-1,";break;
	}

	std::ofstream fout((cilinfofile+".new").c_str());
	std::vector<klee::DefUsePair>::iterator it;
	for(it = defUseList.begin(); it != defUseList.end(); ++it) {
		it->write(fout);
	}	
	fout.close();
}



CilInfoTable::CilInfoTable (std::string cilInfoFile, llvm::Module* module) {
	std::ifstream fin(cilInfoFile.c_str());
	cilinfofile = cilInfoFile;
	//Read def-use pairs from the file defined by def-use-file command line option.
	while(1){
		DefUsePair dupair;
		if(dupair.read(fin)){
			defUseList.push_back(dupair);
		}
		else{
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
					if (0 != cIn->getCalledFunction() && "df_stmt_monitor" == cIn->getCalledFunction()->getName().str()) {
						assert(4 == cIn->getNumArgOperands());
						int func_id = cast<llvm::ConstantInt>(cIn->getArgOperand(0))->getSExtValue(),
						    stmt_id = cast<llvm::ConstantInt>(cIn->getArgOperand(1))->getSExtValue(),
							branch_choice = cast<llvm::ConstantInt>(cIn->getArgOperand(2))->getSExtValue(),
							stmt_line = cast<llvm::ConstantInt>(cIn->getArgOperand(3))->getSExtValue();
						setNodeInstruction(func_id, stmt_id, branch_choice, stmt_line, cIn);
					}
				}
			}
		}
	}
}
bool CilInfoTable::setNodeInstruction(int func_id, int stmt_id, int branch_choice, int stmt_line, llvm::Instruction *inst) {
	// todo: maybe could add index for nodes to speed up
	for (std::vector<klee::DefUsePair>::iterator dfIt = defUseList.begin(); dfIt != defUseList.end(); ++dfIt) {
		if(dfIt->def.equals(func_id, stmt_id, stmt_line)) dfIt->def.inst = inst;
		if(
			(-1 == branch_choice && klee::Cuse == dfIt->type) ||
			(1 == branch_choice && klee::PTuse == dfIt->type) ||
			(0 == branch_choice && klee::PFuse == dfIt->type) // see mail @ Fri 11/4/2016 2:18 PM
		) {
			if(dfIt->use.equals(func_id, stmt_id, stmt_line)) dfIt->use.inst = inst;
		}
		for(std::vector<Cutpoint>::iterator dcIt = dfIt->def.cutpoints.begin(); dcIt != dfIt->def.cutpoints.end(); ++dcIt) {
			if(
				("0" == dcIt->branch_choice && 1 == branch_choice) ||
				("1" == dcIt->branch_choice && 0 == branch_choice) // see mail @ Mon 11/21/2016 7:44 PM
			) {
				if(dcIt->equals(func_id, stmt_id, stmt_line)) dcIt->inst = inst;
			}
		}
		for(std::vector<Cutpoint>::iterator ucIt = dfIt->use.cutpoints.begin(); ucIt != dfIt->use.cutpoints.end(); ++ucIt) {
			if(
				("0" == ucIt->branch_choice && 1 == branch_choice) ||
				("1" == ucIt->branch_choice && 0 == branch_choice)
			) {
				if(ucIt->equals(func_id, stmt_id, stmt_line)) ucIt->inst = inst;
			}	
		}
	}
	return true;
}






//Updata Functions
//**************************************************************************************************
bool Node::equals (const KInstruction *kinstruction) {
	return inst == kinstruction->inst;
}
bool Node::equals (int func_id, int stmt_id, int stmt_line) {
	return int2str(func_id) == this->func_id && int2str(stmt_id) == this->stmt_id;
}

DupairStatus DefUsePair::update(const ExecutionState &state, const KInstruction *kinstruction) {
	if(def.cp_index != def.cutpoints.end() && def.cp_index->equals(state.pc)) {
		llvm::errs() << "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n";
		llvm::errs()<<*(kinstruction->inst)<< "\n"; 
		def.cp_index++;
	}
	if(use.cp_index != use.cutpoints.end() && use.cp_index->equals(state.pc)) {
		llvm::errs() << "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n";
		llvm::errs()<<*(kinstruction->inst)<< "\n"; 
		use.cp_index++;
	}
    if(def.equals(kinstruction) && status != Covered){
        def.ptreeNodes.push_back(state.ptreeNode);
		def.inst = kinstruction->inst;
		status = ReachDef;
		return ReachDef;
    }
	for(std::vector<PTreeNode*>::iterator it = def.ptreeNodes.begin(); it != def.ptreeNodes.end(); ++it) {
		if( status == ReachDef && state.ptreeNode->isPosterityOf(*it) && use.equals(kinstruction)){
			//check redefine
			bool haveRedef = false;
			for(std::vector<PTreeNode*>::iterator redef = redefine_candidates.begin(); redef != redefine_candidates.end(); ++redef) {
				if(state.ptreeNode->isPosterityOf(*redef) && (*redef)->isPosterityOf(*it) && it!=redef)
					haveRedef = true;
			}
			if(haveRedef == false){
				status = Covered;
				return Covered;
			}
		}
	}
	return UnReach;
}

void DefUsePair::addRedefineCandidate(const ExecutionState &state) {
	redefine_candidates.push_back(state.ptreeNode);
}

bool CilInfoTable::update(ExecutionState &state, KInstruction *kinstruction) {
	for(std::vector<DefUsePair>::iterator it = defUseList.begin(); it != defUseList.end(); ++it){
		it->update(state, kinstruction);
	}
    for(std::vector<DefUsePair>::iterator it = defUseList.begin(); it != defUseList.end(); ++it)
		if(it->def.equals(kinstruction)) 
			 for(std::vector<DefUsePair>::iterator it2 = defUseList.begin(); it2 != defUseList.end(); ++it2)
				 if(it->def.withSameVariableAs(it2->def) && it!=it2)
					for(std::vector<PTreeNode*>::iterator defNode = it2->def.ptreeNodes.begin(); defNode != it2->def.ptreeNodes.end(); ++defNode)
						if(state.ptreeNode->isPosterityOf(*defNode) && !it2->def.equals(kinstruction))
							it2->addRedefineCandidate(state);

	for(std::vector<DefUsePair>::iterator it = defUseList.begin(); it != defUseList.end(); ++it) {
        if (it != target &&
		    target->status == ReachDef &&
			it->def.equals(kinstruction) && 
			it->def.withSameVariableAs(target->def)) {
			for(std::vector<PTreeNode*>::iterator it = target->def.ptreeNodes.begin(); it != target->def.ptreeNodes.end(); ++it) {
				if(state.ptreeNode->isPosterityOf(*it))
					return false;
			}
		}
    }
	return true;
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

bool Node::blockEquals(const KInstruction *kinstruction) {
	return inst->getParent() == kinstruction->inst->getParent();
}

int Cutpoint::evaluate(const ExecutionState *es){
    if(equals(es->pc)) {
		for(std::vector<PTreeNode*>::iterator it = ptreeNodes.begin(); it != ptreeNodes.end(); ++it)
			if(es->ptreeNode->isPosterityOf(*it)){
				return -2;
			}	
	    ptreeNodes.push_back(es->ptreeNode);
		return 10;
	}
    return 0;
}


int klee::Use::evaluate(const ExecutionState *es){
    int value = 0;
    if(equals(es->pc))
        value += 10;
    // std::vector<Cutpoint>::iterator cp;
    // for(cp = cutpoints.begin(); cp != cutpoints.end(); ++cp) {
    //     value += cp->evaluate(es);
    // }
	if(cp_index != cutpoints.end())
		value += cp_index->evaluate(es);
    return value;
}

int Definition::evaluate(const ExecutionState *es){
    int value = 0;
    if(equals(es->pc))
        value += 5;
    
    // std::vector<Cutpoint>::iterator cp;
    // for(cp = cutpoints.begin(); cp != cutpoints.end(); ++cp) {
    //    value += cp->evaluate(es);
    // }
	if(cp_index != cutpoints.end())
		value += cp_index->evaluate(es);
    return value;
}

int DefUsePair::evaluate(const ExecutionState *es){
    if (es == 0) return 0;
	int value = 0;
	if(status == UnReach)
    	value = def.evaluate(es);
	if(status == ReachDef)
    	value += use.evaluate(es);
    return value;
}

int CilInfoTable::evaluate (const ExecutionState *es) {
    return target->evaluate(es);
}
//**************************************************************************************************



bool CilInfoTable::setTarget(unsigned int dupairID) {
	for(target = defUseList.begin();target != defUseList.end();target++) {
		if (std::strtoul (target->dua_id.c_str(), NULL, 0) == dupairID){
			if(target->status == Covered) 
				return false;
			else
				return true;
		}
	}
	target = defUseList.begin();
    return false;
}

bool CilInfoTable::coveredTarget(){
	return target->status == Covered;
}

int CilInfoTable::isCutpoint(const llvm::Instruction* inst) {
    if (target->status == UnReach) {
		if (target->def.inst == inst) return 1;
		// for(std::vector<Cutpoint>::iterator cp = target->def.cutpoints.begin(); cp != target->def.cutpoints.end(); ++cp) {
        // 	if (cp->inst == inst) return 1;
    	// }
		if((target->def.cp_index != target->def.cutpoints.end()) && (target->def.cp_index->inst == inst)) return 1;
	}
	else if (target->status == ReachDef) {
		if (target->use.inst == inst) return 1;
		// for(std::vector<Cutpoint>::iterator cp = target->use.cutpoints.begin(); cp != target->use.cutpoints.end(); ++cp) {
		// 	if (cp->inst == inst) return 1;
		// }
		if((target->use.cp_index != target->use.cutpoints.end()) && (target->use.cp_index->inst == inst)) return 1;
	}
	return INT_MAX;
}


int CilInfoTable::isDefUse(const llvm::Instruction* inst) {
	if (target->status == UnReach) {
		if (target->def.inst == inst)
			return 1;
	}	
	else if (target->status == ReachDef) {
		if(target->use.inst == inst)
			return 1;
	}
	return INT_MAX;
}
