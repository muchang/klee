//===-- CilInfoTable.h ----------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include <string>
#include <list>
#include <iostream>

namespace klee {

  struct Point {
	  std::string var_name;
	  int var_id;
	  int var_line;
	  std::string file_name;
	  std::string func_name;
	  int func_id;
	  int stmt_id;
	  std::string cutpoint;

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
	  bool operator == (const Use& );
	  void print();
	  void readFromIfstream(std::ifstream& fin);
  };

  struct DefUsePair {
	  enum Status{
		  UnReach,
		  ReachDef,
		  Covered
	  };
      int dua_id;
      int dua_kind;
      Definition def;
      Use use;
      Status status;

      DefUsePair()
      	  : dua_id(0),
			dua_kind(0),
			status(UnReach) {};
      bool updateStatus(const Definition& );
      bool updateStatus(const Use& );
      void print();
  };

  class CilInfoTable {

  private:
	  std::list<DefUsePair> defUseList;

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
