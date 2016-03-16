//===-- CilInfoTable.h ----------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include "../../lib/Core/PTree.h"

#include <string>
#include <vector>
#include <iostream>

namespace klee {

  struct Point {
	  std::string var_name;
	  std::string var_id;
	  std::string var_line;
	  std::string file_name;
	  std::string func_name;
	  std::string func_id;
	  std::string stmt_id;
	  std::string cutpoint;

    PTreeNode* ptreeNode;

	  bool operator == (const Point& );
	  void print();
  };

  struct Definition : public Point {
	  bool withSameVariableAs(const Definition& );
	  bool operator == (const Definition& );
	  void print();
	  void readFromIfstream(std::ifstream& fin);
  };

  struct Use : public Point {
    std::string kind;
	  bool operator == (const Use& );
	  void print();
	  void readFromIfstream(std::ifstream& fin);
  };

  struct DefUsePair {
    std::string dua_id;
    Definition def;
    Use use;
    std::vector<Definition> redefineList;
    /*
    status:
      0:UnReach,
      1:ReachDef,
      2:Covered,
      3:Redefine,
    */
    int status;

    bool checkRedefine(const Use&);
    bool readFromIfstream(std::ifstream& fin);
    bool updateStatus(const Definition& );
    bool updateStatus(const Use& );
    void print();
  };

  class CilInfoTable {

  private:
	  std::vector<DefUsePair> defUseList;

  public:
	 CilInfoTable(std::string cilInfoFile);
    ~CilInfoTable();

    unsigned getSize() const;
    /*Print the content of the CilInfoTable*/
    void print();
    /*Update defUsePair in CilInfoTable when meet klee_cil_info function*/
    void update(const Definition& );
    void update(const Use& );
  };

}
