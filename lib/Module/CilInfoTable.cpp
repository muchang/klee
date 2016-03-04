#include "klee/Internal/Module/CilInfoTable.h"
#include <fstream>

using namespace klee;

bool Point::operator== (const Point& _point){
	if (this->file_name == _point.file_name &&
			this->func_id == _point.func_id &&
			this->func_name == _point.func_name &&
			this->stmt_id == _point.stmt_id &&
			this->var_id == _point.var_id &&
			this->var_line == _point.var_line &&
			this->var_name == _point.var_name)
		return true;
	else
		return false;
}

void Point::print() {
	std::cerr << var_name << " " << var_id << " " << var_line << " " << file_name << " " << func_name << " " << func_id << " " << stmt_id << " " << cutpoint << "\n" ;
}

bool Definition::withSameVariableAs(const Definition& _def){
	if (this->file_name == _def.file_name &&
			this->func_id == _def.func_id &&
			this->func_name == _def.func_name &&
			this->var_id == _def.var_id &&
			this->var_name == _def.var_name)
		return true;
	else
		return false;
}

void Definition::print() {
	std::cerr << "Definition: ";
	Point::print();
}

bool Definition::operator== (const Definition& _def){
	if (this->file_name == _def.file_name &&
				this->func_id == _def.func_id &&
				this->func_name == _def.func_name &&
				this->stmt_id == _def.stmt_id &&
				this->var_id == _def.var_id &&
				this->var_line == _def.var_line &&
				this->var_name == _def.var_name)
			return true;
		else
			return false;
}


void Definition::readFromIfstream(std::ifstream& fin){
	fin >> var_name >> var_id >> var_line >> file_name >> func_name >> func_id >> stmt_id >> cutpoint;
}

void Use::print() {
	std::cerr << "Use: ";
	Point::print();
}

bool Use::operator== (const Use& _use){
	if (this->file_name == _use.file_name &&
				this->func_id == _use.func_id &&
				this->func_name == _use.func_name &&
				this->stmt_id == _use.stmt_id &&
				this->var_id == _use.var_id &&
				this->var_line == _use.var_line &&
				this->var_name == _use.var_name)
			return true;
		else
			return false;
}

void Use::readFromIfstream(std::ifstream& fin){
	fin >> var_name >> var_id >> var_line >> file_name >> func_name >> func_id >> stmt_id >> cutpoint;
}

bool DefUsePair::updateStatus(const Definition& defPoint) {
	if(def == defPoint && status == UnReach) {
		status = ReachDef;
		return true;
	}
	else if(def.withSameVariableAs(defPoint) && status == ReachDef) {
		status = UnReach;
		return true;
	}

	return false;
}

bool DefUsePair::updateStatus(const Use& usePoint) {
	if(use == usePoint && status == ReachDef) {
		status = Covered;
		return true;
	}

	return false;

}

void DefUsePair::print() {
	std::cerr << dua_id << " " << dua_kind << "\n";
	def.print();
	use.print();
	std::cerr << status << "\n\n";
}

CilInfoTable::CilInfoTable(std::string cilInfoFile){
	std::cout << "Work with DU Searcher!";
	std::ifstream fin(cilInfoFile.c_str());
	int temp;

	//Read def-use pairs from the file defined by def-use-file command line option.
	while(1){
		DefUsePair dupair;
		fin >> dupair.dua_id;
		if(fin.good()){
			fin >> dupair.dua_kind;
			dupair.def.readFromIfstream(fin);
			dupair.use.readFromIfstream(fin);
			fin>>temp;
			dupair.print();
			defUseList.push_front(dupair);
		}
		else{
			std::cout << "**************************************" << std::endl;
			break;
		}
	}


	fin.close();
}

CilInfoTable::~CilInfoTable() {

}

void CilInfoTable::update(const Definition& keyPoint){
	std::list<klee::DefUsePair>::iterator it;
	for(it = defUseList.begin(); it != defUseList.end(); ++it){
		it->updateStatus(keyPoint);
	}
}

void CilInfoTable::update(const Use& keyPoint){
	std::list<klee::DefUsePair>::iterator it;
	for(it = defUseList.begin(); it != defUseList.end(); ++it){
		it->updateStatus(keyPoint);
	}
}

void CilInfoTable::print() {
	std::list<klee::DefUsePair>::iterator it;
	for(it = defUseList.begin(); it != defUseList.end(); ++it) {
		std::cerr << it->dua_id << it->dua_kind;
		std::cerr << it->def.var_name << it->def.var_id << it->def.var_line << it->def.file_name << it->def.func_name << it->def.func_id << it->def.stmt_id << it->def.cutpoint ;
		std::cerr << it->use.var_name << it->use.var_id << it->use.var_line << it->use.file_name << it->use.func_name << it->use.func_id << it->use.stmt_id << it->use.cutpoint << "\n";
	}
}
