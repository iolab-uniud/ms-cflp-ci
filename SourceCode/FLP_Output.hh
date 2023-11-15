// File FLP_Output.hh
#ifndef FLP_OUTPUT_HH
#define FLP_OUTPUT_HH
#include <iostream>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include "FLP_Input.hh"

using namespace std;

struct Suppliers { int w1 = -1, w2 = -1, q1 = 0, q2 = 0; };
ostream& operator<<(ostream& os, const Suppliers& sup);

enum class Position { FIRST, SECOND };

struct Transfer
{
  Transfer(int s, int fw, int tw, int q) 
  { store = s; from_w = fw; to_w = tw; quantity = q; }
  int store;
  int from_w, to_w;
  int quantity;
};

class FLP_Output 
{
  friend ostream& operator<<(ostream& os, const FLP_Output& out);
  friend istream& operator>>(istream& is, FLP_Output& out);
  friend bool operator==(const FLP_Output& out1, const FLP_Output& out2);
public:
  FLP_Output(const FLP_Input& i);
  FLP_Output& operator=(const FLP_Output& out);
  Suppliers Assignment(int s) const { return assignment[s]; }
  int FirstSupplier(int s) const { return assignment[s].w1; }
  int SecondSupplier(int s) const { return assignment[s].w2; }
  int FirstQuantity(int s) const { return assignment[s].q1; }
  int SecondQuantity(int s) const { return assignment[s].q2; }
  void FullAssign(int s, int w);
  void AssignFirst(int s, int w, int q);
  void AssignSecond(int s, int w, int q);
  void ChangeFirstSupplierAndQuantity(int s, int new_w, int new_q);
  void ChangeSecondSupplierAndQuantity(int s, int new_w, int new_q);
  int CheckAndComputeQuantity(int s, int new_w, Position pos) const; 
  int BestTransfer(int s, int w, int q, const vector<int>& assumed_open, const vector<Transfer>& planned_transfers) const; 
  int RevisedResidualCapacity(int w, const vector<Transfer>& transfers) const; 


  void ReplaceSupplier(int s, Position pos, int w, int q);
  int Load(int w) const { return load[w]; }
  bool Open(int w) const { return load[w] > 0; }
  bool Closed(int w) const { return load[w] == 0; }
  int ResidualCapacity(int w) const { return in.Capacity(w) - load[w]; }
  void Reset();
  void Dump(ostream& os) const;
  void PrettyPrint(ostream& os) const;
  bool Compatible(int s, int w) const { return incompatible[s][w] == 0; } 
  bool AlmostCompatible(int s, int w) const { return incompatible[s][w] == 1; } 
  int Clients(int w) const { return client_list[w].size(); }
  int Client(int w, int i) const { return client_list[w][i]; }

  CostType ComputeCost() const;
  int ComputeViolations() const;
  int NumberOfSigleSourceStores() const;
  int NumberOfOpenWarehouses() const;
private:
  const FLP_Input& in;
  vector<Suppliers> assignment;   // warehouses assigned to the store 
  vector<int> load; // load assigned to the warehouse
  vector<vector<int>> incompatible;  // store x warehouse: no. of stores incompatible with s assigned to w
  vector<vector<int>> client_list; // list of stores supplied by a warehouse
  void ReorderSuppliers(int s);
};
#endif
