#include "klee/Internal/Module/CilInfoTable.h"
#include <fstream>

using namespace klee;

class PTreeNode;

bool Point::operator== (const Point& _point) {
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

bool Definition::withSameVariableAs(const Definition& _def) {
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

void Definition::print() {
	std::cerr << "Definition: ";
	Point::print();
}

bool Definition::operator== (const Definition& _def) {
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


void Definition::readFromIfstream(std::ifstream& fin) {
	fin >> var_name >> var_id >> var_line >> file_name >> func_name >> func_id >> stmt_id >> cutpoint;
}

void Use::print() {
	std::cerr << "Use: " << "use_kind: " << kind << " ";
	Point::print();
}

bool Use::operator== (const Use& _use){
	if (this->file_name == _use.file_name &&
				this->func_id == _use.func_id &&
				this->func_name == _use.func_name &&
				this->stmt_id == _use.stmt_id &&
				this->var_id == _use.var_id &&
				this->var_line == _use.var_line &&
				this->var_name == _use.var_name &&
				this->kind == _use.kind)
			return true;
		else
			return false;
}

void Use::readFromIfstream(std::ifstream& fin){
	fin >> var_name >> var_id >> var_line >> file_name >> func_name >> func_id >> stmt_id >> cutpoint;
}

bool DefUsePair::updateStatus(const Definition& defPoint) {
	if(def == defPoint && (status == 0 || status == 3)) {
		status = 1;
		def.ptreeNode = defPoint.ptreeNode;
		return true;
	}
	if(status == 1 && def.withSameVariableAs(defPoint)) {
		redefineList.push_back(defPoint);
		return true;
	}
	return false;
}

bool DefUsePair::updateStatus(const Use& usePoint) {
	if(use == usePoint
			&& status == 1
			&& usePoint.ptreeNode->isPosterityOf(def.ptreeNode)
			&& checkRedefine(usePoint)) {
		status = 2;
		return true;
	}
	return false;
}

bool DefUsePair::checkRedefine(const Use& usePoint) {
	std::vector<klee::Definition>::iterator it;
	for(it = redefineList.begin(); it != redefineList.end(); ++it){
		if(usePoint.ptreeNode->isPosterityOf(it->ptreeNode)){
			status = 3;
			return false;
		}
	}
	return true;
}

bool DefUsePair::readFromIfstream(std::ifstream& fin){
	fin >> dua_id;
	if(fin.good()){
		fin >> use.kind;
		def.readFromIfstream(fin);
		use.readFromIfstream(fin);
		fin >> status;
		print();
		return true;
	}
	else{
		return false;
	}
}

void DefUsePair::print() {
	std::cerr << dua_id << "\n";
	def.print();
	use.print();
	std::cerr << status << "\n\n";
}

CilInfoTable::CilInfoTable(std::string cilInfoFile){
	std::cout << "Work with DU Searcher!";
	std::ifstream fin(cilInfoFile.c_str());
	//Read def-use pairs from the file defined by def-use-file command line option.
	while(1){
		DefUsePair dupair;
		if(dupair.readFromIfstream(fin)){
			defUseList.push_back(dupair);
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
	std::vector<klee::DefUsePair>::iterator it;
	for(it = defUseList.begin(); it != defUseList.end(); ++it){
		it->updateStatus(keyPoint);
	}
}

void CilInfoTable::update(const Use& keyPoint){
	std::vector<klee::DefUsePair>::iterator it;
	for(it = defUseList.begin(); it != defUseList.end(); ++it){
		it->updateStatus(keyPoint);
	}
}

void CilInfoTable::print() {
	std::vector<klee::DefUsePair>::iterator it;
	for(it = defUseList.begin(); it != defUseList.end(); ++it) {
		it->print();
	}
}
