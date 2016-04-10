//===-- CilInfoTable.h ----------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include "../../lib/Core/PTree.h"
#include "../../lib/Core/Common.h"

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
	  std::string cutpoints;

    PTreeNode* ptreeNode;

	  bool operator == (const Point& );
	  void print();
    void read(std::ifstream& fin);
  };

  struct Definition : public Point {
	  bool withSameVariableAs(const Definition& );
	  bool operator == (const Definition& );
	  void print();
  };

  struct Use : public Point {
    std::string kind;
	  bool operator == (const Use& );
	  void print();
  };

  struct Cutpoint : public Point {
    /*
      0:UnReach
      1:Reach
    */
    int status;
    std::string kind;
    bool operator == (const Point& );
    Cutpoint(std::string);
    void print();
  };

  struct DefUsePair {
    std::string dua_id;
    Definition def;
    Use use;
    std::vector<Definition> redefines;
    std::vector<Cutpoint> cutpoints;
    /*
    status:
      0:UnReach,
      1:ReachDef,
      2:Covered,
      3:Redefine,
    */
    int status;
    bool initCutpoints();
    bool checkRedefine(const Use&);
    bool read(std::ifstream& );
    bool updateStatus(const Definition& );
    bool updateStatus(const Use& );
    bool updateCutpointsStatus(const Use& );
    void print();
    void printCutpoints();
    bool equalKind(const Use&, const Cutpoint&);
  };

  struct updateResult{
    bool update;
    bool hitDef;
    bool hitUse;
    bool hitCutpoint;
    updateResult(){
      update = false;
      hitDef = false;
      hitUse = false;
      hitCutpoint = false;
    }
  };

  class CilInfoTable {

  private:
	  std::vector<DefUsePair> defUseList;
    unsigned cutpointLevel;

  public:
	  CilInfoTable(std::string cilInfoFile);

    unsigned getSize() const;
    bool clearCurrentCutpointLevel();
    bool clearAllPair();
    /*Print the content of the CilInfoTable*/
    void print();
    /*Update defUsePair in CilInfoTable when meet klee_cil_info function*/
    bool update(const Definition& );
    bool update(const Use& );
  };

}
