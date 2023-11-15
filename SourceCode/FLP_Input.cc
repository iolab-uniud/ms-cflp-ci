// File FLP_Input.cc
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include "FLP_Input.hh"

FLP_Input::FLP_Input(string file_name, double sqrt_ratio_preferred, int cost_diff_threshold)
{  
  const int MAX_DIM = 100;
  int w, s, i, preferred;
  char ch, buffer[MAX_DIM];

  ifstream is(file_name);
  if(!is)
  {
    cerr << "Cannot open input file " <<  file_name << endl;
    exit(1);
  }
  
  is >> buffer >> ch >> warehouses >> ch;
  is >> buffer >> ch >> stores >> ch;
  
  capacity.resize(warehouses);
  fixed_cost.resize(warehouses);
  amount_of_goods.resize(stores);
  supply_cost.resize(stores,vector<CostType>(warehouses));
  
  // read capacity
  is.ignore(MAX_DIM,'['); // read "... Capacity = ["
  for (w = 0; w < warehouses; w++)
    is >> capacity[w] >> ch;
  
  // read fixed costs  
  is.ignore(MAX_DIM,'['); // read "... FixedCosts = ["
  for (w = 0; w < warehouses; w++)
    is >> fixed_cost[w] >> ch;

  // read goods
  is.ignore(MAX_DIM,'['); // read "... Goods = ["
  for (s = 0; s < stores; s++)
    {
      is >> amount_of_goods[s] >> ch;
      if (amount_of_goods[s] == 1)
        throw invalid_argument("The amount of goods cannot be equal to 1 (it cannot be split into two suppliers)");
    }
  // read supply costs
  is.ignore(MAX_DIM,'['); // read "... SupplyCost = ["
  is >> ch; // read first '|'
  for (s = 0; s < stores; s++)
  {	 
    for (w = 0; w < warehouses; w++)
      is >> supply_cost[s][w] >> ch;
  }
  is >> ch >> ch;
  
  // read incompatibilities
  int s2, num_incompatibilities;
  is >> buffer >> ch >> num_incompatibilities >> ch;
  incompatibilities.resize(num_incompatibilities);
  incompatibility_list.resize(stores);
  is.ignore(MAX_DIM,'['); // read "... IncompatiblePairs = ["
  for (i = 0; i < num_incompatibilities; i++)
  {
    is >> ch >> s >> ch >> s2;	  
    incompatibilities[i].first = s - 1;
    incompatibilities[i].second = s2 - 1;
    incompatibility_list[s-1].push_back(s2-1);
    incompatibility_list[s2-1].push_back(s-1);    
  }
  is >> ch >> ch;

  // compute preferred facilities
  preferred_suppliers.resize(stores);
  preferred_clients.resize(warehouses);
  preference.resize(stores,vector<bool>(warehouses,false));
  vector<pair<int,CostType>> suppliers;
  // preferred can't be more than the number of facilities
  preferred = min(static_cast<int>(sqrt_ratio_preferred * sqrt(warehouses) + 0.5), warehouses);                   

  int best_cost;
  for (s = 0; s < stores; s++)
    {
      suppliers.clear();
      for (w = 0; w < warehouses; w++)
        suppliers.push_back(make_pair(w,supply_cost[s][w]));
      sort(suppliers.begin(), suppliers.end(), 
           [](const pair<int,CostType>& left, const pair<int,CostType>& right) {
             return left.second < right.second; });
      for (i = 0; i < preferred; i++)
        {
          w = suppliers[i].first;
          preferred_suppliers[s].push_back(w);
          InsertClient(w,s);   // preferred_clients[w].push_back(s);
          preference[s][w] = true;
        }
      best_cost = suppliers[0].second;
      for (; i < warehouses; i++)
        if (suppliers[i].second <= best_cost + cost_diff_threshold)
          {
            w = suppliers[i].first;
            preferred_suppliers[s].push_back(w);
            InsertClient(w,s);  // preferred_clients[w].push_back(s);
            preference[s][w] = true;
          }
        else
          break;
    }	

  // cerr << "Computing neighbor warehouses...";
  vector<vector<bool>> neighbors(warehouses,vector<bool>(warehouses,false));
  int w1, w2;
  unsigned i2;
  for (s = 0; s < stores; s++)
    for (i = 0; static_cast<unsigned>(i) < preferred_suppliers[s].size()-1; i++)
      {
        w1 = preferred_suppliers[s][i];
        for (i2 = i+1; i2 < preferred_suppliers[s].size(); i2++)
          {
            w2 = preferred_suppliers[s][i2];            
            if (w1 < w2)
              {
                if (!neighbors[w1][w2])
                  {
                    neighbor_warehouses.push_back(make_pair(w1,w2));
                    neighbors[w1][w2] = true;
                  }
              }
            else if (w2 < w1)
              {
                if (!neighbors[w2][w1])
                  {
                    neighbor_warehouses.push_back(make_pair(w2,w1));
                    neighbors[w2][w1] = true;
                  }
              }
          } 
      }
}

void FLP_Input::InsertClient(int w, int s)
{ // insert client s in the preferred_clients of w (ordered by cost)
  unsigned i = 0;
  while (i < preferred_clients[w].size() && supply_cost[s][w] > supply_cost[preferred_clients[w][i]][w])
    i++;
  preferred_clients[w].insert(preferred_clients[w].begin() + i, s);
}

ostream& operator<<(ostream& os, const FLP_Input& in)
{
  int w, s, i;
  os << "Warehouses = " << in.warehouses << ";" << endl;
  os << "Stores = " << in.stores << ";" << endl;
  os << endl;
  
  os << "Capacity = [";
  for (w = 0; w < in.warehouses; w++)
    {
      os << in.capacity[w];
      if (w < in.warehouses - 1)
        os << ", ";
      else
        os << "];" << endl;
    }
  
  os << "FixedCost = [";
  for (w = 0; w < in.warehouses; w++)
    {
      os << in.fixed_cost[w];
      if (w < in.warehouses - 1)
        os << ", ";
      else
        os << "];" << endl;
    }
  
  os << "Goods = [";
  for (s = 0; s < in.stores; s++)
    {
      os << in.amount_of_goods[s];
      if (s < in.stores - 1)
        os << ", ";
      else
        os << "];" << endl;
    }
  
  os << "SupplyCost = [|";
  for (s = 0; s < in.stores; s++)
    {
      for (w = 0; w < in.warehouses; w++)
        {
          os << in.supply_cost[s][w];
          if (w < in.warehouses - 1)
            os << ",";
          else
            os << "|" << endl;
        }
    }
  os << "];" << endl;
  
  os << "Incompatibilities = " << in.incompatibilities.size() << ";" << endl;
  os << "IncompatiblePairs = [|";
  for (i = 0; i < static_cast<int>(in.incompatibilities.size()); i++)
    os << " " << in.incompatibilities[i].first+1 << ", " << in.incompatibilities[i].second+1 << " |";
  os << "];" << endl;
  
  os << "Preferred suppliers:" << endl;
  for (s = 0; s < in.stores; s++)
    {
      os << s << ": ";
      for (i = 0; i < static_cast<int>(in.preferred_suppliers[s].size()); i++)
        {
          w = in.preferred_suppliers[s][i];
          os << w << '/' << in.supply_cost[s][w] << " ";
        }
      os << endl;
    }
  os << "Preferred clients:" << endl;
  for (w = 0; w < in.warehouses; w++)
    {
      os << w << ": ";
      for (i = 0; i < static_cast<int>(in.preferred_clients[w].size()); i++)
        {
          s = in.preferred_clients[w][i];
          os << s << '/' << in.supply_cost[s][w] << " ";
        }
      os << endl;
    }

  in.PrintStatistics(os); 
  return os;
}

void FLP_Input:: PrintStatistics(ostream& os) const
{
  int s, w;
  float total_demand = 0, total_capacity = 0, avg_opening_cost = 0, avg_supply_cost = 0;

  cerr << "warehouses; stores; number of incompatibilities; average opening cost; average supply cost; ratio between the total demand and the total capacity;" << endl;
  
  for (w = 0; w < warehouses; w++)
    avg_opening_cost += fixed_cost[w];

  for (s = 0; s < stores; s++)
    for (w = 0; w < warehouses; w++)
      avg_supply_cost += supply_cost[s][w];

  for (s = 0; s < stores; s++)
    total_demand += amount_of_goods[s];

  for (w = 0; w < warehouses; w++)
    total_capacity += capacity[w];

  os << warehouses << "; " << stores << "; ";
  os << incompatibilities.size() << "; ";
  os << avg_opening_cost/warehouses << "; " << avg_supply_cost/(stores*warehouses) 
     << "; " << total_demand/total_capacity << ";" << endl;
    
}
