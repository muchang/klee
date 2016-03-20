#include "klee/Internal/Module/CilInfoTable.h"
#include <fstream>

using namespace klee;

class PTreeNode;

std::vector<std::string> split(const std::string &s, char delim) {
	  std::vector<std::string> elems;
    std::stringstream ss(s);
		std::string item;
    while (std::getline(ss, item, delim)) {
				std::cerr << item << std::endl;
        elems.push_back(item);
    }
    return elems;
}


bool Point::operator== (const Point& _point) {
	if (this->file_name == _point.file_name &&
			this->func_id == _point.func_id &&
			this->stmt_id == _point.stmt_id &&
			this->var_id == _point.var_id &&
			this->var_line == _point.var_line &&
			this->var_name == _point.var_name)
		return true;
	else
		return false;
}

void Point::print() {
	std::cerr << var_name << " " << var_id << " " << var_line << " " << file_name << " " << func_name << " " << func_id << " " << stmt_id << " " << cutpoints << "\n" ;
}

void Point::read(std::ifstream& fin) {
	fin >> var_name >> var_id >> var_line >> file_name >> func_name >> func_id >> stmt_id;
	std::string tmp_cutpoints;
	fin >> tmp_cutpoints;
	if(tmp_cutpoints == "no")
		cutpoints = "";
	else
		cutpoints = tmp_cutpoints.substr(0,tmp_cutpoints.length()-1);
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

Cutpoint::Cutpoint(std::string sequence) {
	std::vector<std::string> sequenceList = split(sequence,':');
	//need assert
	func_id = sequenceList[0];
	stmt_id = sequenceList[1];
	var_line = sequenceList[3];
	print();
}

void Cutpoint::print() {
	std::cerr << "func_id:" << func_id << "\n";
	std::cerr << "stmt_id:" << stmt_id << "\n";
	std::cerr << "var_line:" << var_line << "\n";
}

bool DefUsePair::updateStatus(const Definition& defPoint) {
	if(def == defPoint && (status == 0 || status == 3)) {
		status = 1;
		def.ptreeNode = defPoint.ptreeNode;
		return true;
	}
	if(status == 1 && def.withSameVariableAs(defPoint)) {
		redefines.push_back(defPoint);
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
	for(it = redefines.begin(); it != redefines.end(); ++it){
		if(usePoint.ptreeNode->isPosterityOf(it->ptreeNode)){
			status = 3;
			return false;
		}
	}
	return true;
}

bool DefUsePair::read(std::ifstream& fin){
	fin >> dua_id;
	if(fin.good()){
		fin >> use.kind;
		def.read(fin);
		use.read(fin);
		fin >> status;
		print();
		initCutpoints();
		return true;
	}
	else{
		return false;
	}
}

bool DefUsePair::initCutpoints() {
	std::string sequence;
	if(def.cutpoints == "")
		 sequence = use.cutpoints;
	else
		sequence = def.cutpoints + ";" + use.cutpoints;
	std::vector<std::string> sequenceList = split(sequence,';');
	std::vector<std::string>::iterator it;
	for (it = sequenceList.begin() ; it != sequenceList.end(); ++it){
		Cutpoint cutpoint(*it);
		cutpoints.push_back(cutpoint);
	}
	return true;
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
		if(dupair.read(fin)){
			defUseList.push_back(dupair);
		}
		else{
			std::cout << "**************************************" << std::endl;
			break;
		}
	}
	fin.close();
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
