#include "klee/Internal/Module/CilInfoTable.h"
#include <fstream>

using namespace klee;

class PTreeNode;


/* A tool function split the string by the delim. */
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

/* Point struct. */
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
/* End of Point struct. */

/* Definition struct. */
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
/* End of Definition struct. */

/* Use struct. */
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
/* End of Use struct. */

/* Cutpoint struct. */
Cutpoint::Cutpoint(std::string sequence) {
	std::vector<std::string> sequenceList = split(sequence,':');
	//need assert
	assert( sequenceList.size() == 4 );
	func_id = sequenceList[0];
	stmt_id = sequenceList[1];
	kind = sequenceList[2];
	var_line = sequenceList[3];
	status = 0;
}

void Cutpoint::print() {
	std::cerr << "func_id:" << func_id << "\n";
	std::cerr << "stmt_id:" << stmt_id << "\n";
	std::cerr << "var_line:" << var_line << "\n";
}
/* End of Cutpoint struct. */

/* DefUsePair struct. */
bool DefUsePair::updateStatus(const Definition& defPoint) {
	if(def == defPoint && (status == 0 || status == 3)) {
		status = 1;
		def.ptreeNode = defPoint.ptreeNode;
	}
	if(status == 1 && def.withSameVariableAs(defPoint)) {
		redefines.push_back(defPoint);
	}
	return false;
}

bool DefUsePair::updateStatus(const Use& usePoint) {
	if(use == usePoint
			&& status == 1
			&& usePoint.ptreeNode->isPosterityOf(def.ptreeNode)
			&& checkRedefine(usePoint)) {
		status = 2;
	}
  return updateCutpointsStatus(usePoint);
}

bool DefUsePair::updateCutpointsStatus(const Use& use) {
	bool flag = false;
	std::vector<Cutpoint>::iterator it;
	for (it = cutpoints.begin() ; it != cutpoints.end(); ++it){
		if(it->func_id == use.func_id &&
			 it->stmt_id == use.stmt_id &&
		 	 it->var_line == use.var_line &&
		 	 equalKind(use,*it) ){
			 it->status = 1;
			 flag = true;
		}
	}
	return flag;
}

bool DefUsePair::equalKind(const Use& use, const Cutpoint& cutpoint) {
	if(use.kind == "1" && cutpoint.kind == "0")
		return true;
	else if(use.kind == "2" && cutpoint.kind == "1")
		return true;
	else
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
		initCutpoints();
		print();
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

void DefUsePair::printCutpoints() {
	std::vector<Cutpoint>::iterator it;
	for (it = cutpoints.begin() ; it != cutpoints.end(); ++it){
		it->print();
	}
}

void DefUsePair::print() {
	std::cerr << dua_id << "\n";
	def.print();
	use.print();
	//printCutpoints();
	std::cerr << status << "\n\n";
}
/* End of DefUsePair struct. */

/* CilInfoTable struct. */
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
	cutpointLevel = 0;
	fin.close();
}

bool CilInfoTable::clearCurrentCutpointLevel() {
	bool cutpointClear = true;
	std::vector<DefUsePair>::iterator it;
	for (it = defUseList.begin() ; it != defUseList.end(); ++it){
		if(cutpointLevel >= it->cutpoints.size()){
			cutpointClear = false;
		}
		else if(it->cutpoints[cutpointLevel].status == 0){
			std::cerr << "cutpointLevel:" << cutpointLevel << "\n";
			it->print();
			return false;
		}
	}
	cutpointLevel++;
	std::cerr << "cutpointLevel:" << cutpointLevel << "\n";
	return cutpointClear;
}

bool CilInfoTable::clearAllPair() {
	std::vector<DefUsePair>::iterator it;
	for (it = defUseList.begin(); it != defUseList.end(); ++it){
		if(it->status != 2){       //Covered
			return false;
		}
	}
	return true;
}

bool CilInfoTable::update(const Definition& keyPoint) {
	bool flag = false;
	std::vector<klee::DefUsePair>::iterator it;
	for(it = defUseList.begin(); it != defUseList.end(); ++it){
		if(it->updateStatus(keyPoint)){
			flag = true;
		}
	}
	return flag;
}

bool CilInfoTable::update(const Use& keyPoint) {
	bool flag = false;
	std::vector<klee::DefUsePair>::iterator it;
	for(it = defUseList.begin(); it != defUseList.end(); ++it){
		if(it->updateStatus(keyPoint)){
			flag = true;
		}
	}
	return flag;
}

void CilInfoTable::print() {
	std::vector<klee::DefUsePair>::iterator it;
	for(it = defUseList.begin(); it != defUseList.end(); ++it) {
		it->print();
	}
}
/* End of CilInfoTable. */
