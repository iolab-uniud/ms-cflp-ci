// File FLP_Input.hh
#ifndef FLP_INPUT_HH
#define FLP_INPUT_HH
#include <iostream>
#include <cstdlib>
#include <vector>
#include <algorithm>

using namespace std;
typedef int CostType;

inline void RemoveElement(vector<int>& v, int e)
{ v.erase(remove(v.begin(), v.end(), e), v.end()); }

inline bool Member(const vector<int>& v, int e)
{ return find(v.begin(), v.end(), e) != v.end(); }

inline bool Member(const vector<pair<int,int> >& v, pair<int,int> e)
{ return find(v.begin(), v.end(), e) != v.end(); }

class FLP_Input 
{
  friend ostream& operator<<(ostream& os, const FLP_Input& in);
public:
  FLP_Input(string file_name, double sqrt_ratio_preferred, int cost_diff_threshold);
  int Stores() const { return stores; }
  int Warehouses() const { return warehouses; }
  int Capacity(int w) const { return capacity[w]; }
  int FixedCost(int w) const { return fixed_cost[w]; }
  int AmountOfGoods(int s) const { return amount_of_goods[s]; }
  CostType SupplyCost(int s, int w) const { return supply_cost[s][w]; }
  int Incompatibilities() const { return incompatibilities.size(); }
  pair<int, int> Incompatibility(int i) const { return incompatibilities[i]; }
  int StoreIncompatibilities(int s) const { return incompatibility_list[s].size(); }
  int StoreIncompatibility(int s, int i) const { return incompatibility_list[s][i]; }
  bool Incompatible(int s1, int s2) const { return Member(incompatibility_list[s1],s2); }
  int PreferredSuppliers(int s) const { return preferred_suppliers[s].size(); }
  int PreferredSupplier(int s, int i) const { return preferred_suppliers[s][i]; }
  int PreferredClients(int w) const { return preferred_clients[w].size(); }
  int PreferredClient(int w, int i) const { return preferred_clients[w][i]; }
  bool Preference(int s, int w) const { return preference[s][w]; }
  int NeighborWarehousePairs() const { return neighbor_warehouses.size(); }
  pair<int,int> NeighborWarehouses(int i) const { return neighbor_warehouses[i]; }
  void PrintStatistics(ostream& os) const;
 private:
  int stores, warehouses;
  vector<int> capacity;
  vector<int> fixed_cost;
  vector<int> amount_of_goods;
  vector<vector<CostType>> supply_cost;
  vector<pair<int,int>> incompatibilities; // list of incompatible pairs of stores
  vector<vector<int>> incompatibility_list;
  vector<vector<int>> preferred_suppliers; // list of preferred suppliers for each store
  vector<vector<int>> preferred_clients; // list of preferred clients for each warehouse
  vector<vector<bool>> preference; // store X warehouse matrix of preferred suppliers
  vector<pair<int,int>> neighbor_warehouses; // store the pairs of "neighbor" warehouses (i.e. with at least one client in common)
  void InsertClient(int w, int s);
};
#endif
