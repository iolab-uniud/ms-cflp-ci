// File FLP_Output.cc
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include "FLP_Output.hh"

FLP_Output::FLP_Output(const FLP_Input& my_in)
  : in(my_in), assignment(in.Stores()), load(in.Warehouses(),0), 
    incompatible(in.Stores(),vector<int>(in.Warehouses())), 
    client_list(in.Warehouses())
{}

FLP_Output& FLP_Output::operator=(const FLP_Output& out)
{
  assignment = out.assignment;
  load = out.load;
  incompatible = out.incompatible;
  client_list = out.client_list;
  return *this;
}

void FLP_Output::AssignFirst(int s, int w, int q)
{ // assign to w1, starting from empty solution
  assignment[s].w1 = w;
  assignment[s].q1 = q;
  client_list[w].push_back(s);
  load[w] += q;
  int s2, i;
  for (i = 0; i < in.StoreIncompatibilities(s); i++)
    {
      s2 = in.StoreIncompatibility(s,i);
      incompatible[s2][w]++;
    }
}
	
void FLP_Output::AssignSecond(int s, int w, int q)
{ // assign to w2, starting from empty solution
  assignment[s].w2 = w;  
  assignment[s].q2 = q;
  if (w != -1)
  {
    client_list[w].push_back(s);
    load[w] += q;
    int s2, i;
    for (i = 0; i < in.StoreIncompatibilities(s); i++)
      {
        s2 = in.StoreIncompatibility(s,i);
        incompatible[s2][w]++;
      }
  }
  ReorderSuppliers(s);
}

void FLP_Output::FullAssign(int s, int w)
{  // full assign to w1, starting from empty solution
  assignment[s].w1 = w;
  assignment[s].q1 = in.AmountOfGoods(s);
  client_list[w].push_back(s);
  load[w] += in.AmountOfGoods(s);
  int s2, i;
  for (i = 0; i < in.StoreIncompatibilities(s); i++)
    {
      s2 = in.StoreIncompatibility(s,i);
      incompatible[s2][w]++;
    }
  assignment[s].w2 = -1;
  assignment[s].q2 = 0;
}

void FLP_Output::ChangeFirstSupplierAndQuantity(int s, int new_w, int new_q)
{ // change the first supplier and the quantity, the quantity of the second
  // supplier is modified accordingly
  int old_w1 = assignment[s].w1, old_w2 = assignment[s].w2;
  int old_q1 = assignment[s].q1, old_q2 = assignment[s].q2;
  int new_q2 = in.AmountOfGoods(s) - new_q;	

  assignment[s].w1 = new_w;
  assignment[s].q1 = new_q;
  assignment[s].q2 = new_q2;

  client_list[new_w].push_back(s);
  RemoveElement(client_list[old_w1],s);

  load[new_w] += new_q;
  load[old_w1] -= old_q1;
  load[old_w2] += (new_q2 - old_q2);

  int s2, i;
  for (i = 0; i < in.StoreIncompatibilities(s); i++)
  {
     s2 = in.StoreIncompatibility(s,i);
     incompatible[s2][new_w]++;
     incompatible[s2][old_w1]--;
  }
  ReorderSuppliers(s);
}

void FLP_Output::ChangeSecondSupplierAndQuantity(int s, int new_w, int new_q)
{// change the second supplier and the quantity, the quantity of the first
  // supplier is modified accordingly
  // NOTE the second supplier might be -1 (the first never)
  int old_w1 = assignment[s].w1, old_w2 = assignment[s].w2;
  int old_q1 = assignment[s].q1, old_q2 = assignment[s].q2;
  int new_q1 = in.AmountOfGoods(s) - new_q;	

  if (new_q == 0) // do not assign to new_w if quantity new_q has been set to 0 by CheckAndComputeRedistribution
    new_w = -1;

  assignment[s].w2 = new_w;
  assignment[s].q2 = new_q;
  assignment[s].q1 = new_q1;

  if (new_w != -1)
    client_list[new_w].push_back(s);
  if (old_w2 != -1)
    RemoveElement(client_list[old_w2],s);

  load[new_w] += new_q;
  load[old_w2] -= old_q2;
  load[old_w1] += (new_q1 - old_q1);

  int s2, i;
  for (i = 0; i < in.StoreIncompatibilities(s); i++)
  {
     s2 = in.StoreIncompatibility(s,i);
     if (new_w != -1) incompatible[s2][new_w]++;
     if (old_w2 != -1) incompatible[s2][old_w2]--;
  }
  ReorderSuppliers(s);
}

void FLP_Output::ReorderSuppliers(int s)
{
  if (assignment[s].w2 != -1 && in.SupplyCost(s, assignment[s].w1) > in.SupplyCost(s, assignment[s].w2))
    {
      swap(assignment[s].w1,assignment[s].w2);
      swap(assignment[s].q1,assignment[s].q2);
    }
}

void FLP_Output::ReplaceSupplier(int s, Position pos, int new_w, int q)
{ // NOTE: quantity q is passed and not computed, because in the Swap move it might 
  // be changed by the first call to the second one
  int old_w, other_old_w;
    
  if (pos == Position::FIRST)
    {
      old_w = assignment[s].w1;
      other_old_w = assignment[s].w2;
      assignment[s].w1 = new_w;
    }
  else
    {
      old_w = assignment[s].w2;
      other_old_w = assignment[s].w1;
      assignment[s].w2 = new_w;		
    }
  load[new_w] += q;
  RemoveElement(client_list[old_w],s);
  load[old_w] -= q;

  int i, s2;
  for (i = 0; i < in.StoreIncompatibilities(s); i++)
    {
      s2 = in.StoreIncompatibility(s,i);
      if (new_w != other_old_w)
        incompatible[s2][new_w]++;
      incompatible[s2][old_w]--;
    }

  if (new_w != other_old_w)
    {
      client_list[new_w].push_back(s);
      ReorderSuppliers(s);
    }
  else // compact suppliers in w1 (independently if the new supplier is entered as w1 or w2)
    {
      assignment[s].q1 += assignment[s].q2;
      assignment[s].q2 = 0;
      assignment[s].w2 = -1;
    }
}

void FLP_Output::Reset()
{
  int s, w;
  for (s = 0; s < in.Stores(); s++)
    {
      assignment[s].w1 = -1;
      assignment[s].w2 = -1;
      assignment[s].q1 = 0;
      assignment[s].q2 = 0;
      for (w = 0; w < in.Warehouses(); w++)
        incompatible[s][w] = 0;
    }
  for (w = 0; w < in.Warehouses(); w++)
    {
      client_list[w].clear();
      load[w] = 0;
    }
}

int FLP_Output::CheckAndComputeQuantity(int s, int new_w, Position pos) const
{ 
  // computes the best quantity to assign to new_w to be inserted in position pos
  // (returns -1 if the assignment is infeasible)
  int w1 = assignment[s].w1, w2 = assignment[s].w2;
  int q1 = assignment[s].q1, q2 = assignment[s].q2;

  if (pos == Position::SECOND && w2 != -1) 
    {
      if (ResidualCapacity(w1) + ResidualCapacity(new_w) < q2)
        return -1; // there is no room for the current load of w2 in w1 and new_w
      if (in.SupplyCost(s,w1) < in.SupplyCost(s,new_w))
        { // give as much as possible of q2 (possibly all) to old_w1
          if (q2 <= ResidualCapacity(w1))// give everything to w1, set new_q to 0
            return 0;
          else
            return q2 - ResidualCapacity(w1);
        }
      else
        { // give as much as possible to new_w 
          if (q2 <= ResidualCapacity(new_w))
            return q2;
          else
            return ResidualCapacity(new_w);
        }
    }
  else if (pos == Position::FIRST && w2 == -1) // replace the unique supplier (no rebalance here)
    {
      if (ResidualCapacity(new_w) < q1)
        return -1; // there is no room for the total load which is in old_w1 new_w
      else
        return in.AmountOfGoods(s);
    }
  else // mv.pos == Position::SECOND && w2 == -1  // introduce the second supplier: rebalance the load of the first
  {
    // NOTE 1: old_w1 surely has the capacity (as it splits its own current load)
    // NOTE 2: the quantity redistributed is decreased by one to provide against the possibility that everything is given to new_w
    // (we want to keep at least one unit to old_w1 as the first supplier is never eliminated)
    // we could swap suppliers in this case
    // NOTE 3: if the new supplier costs more than old_w1, the move would be null, and so it it forbidden (returns -1)
    if (in.SupplyCost(s,w1) < in.SupplyCost(s,new_w))
      return -1; // see NOTE 3 above
    else
      { // one unit remains to old_w1 (see note 2 above)
        if (in.AmountOfGoods(s) - 1 <= ResidualCapacity(new_w))
          return  in.AmountOfGoods(s) - 1;
        else
          return ResidualCapacity(new_w);
      }
  }
}

int FLP_Output::BestTransfer(int s, int old_w, int q, const vector<int>& assumed_open, const vector<Transfer>& planned_transfers) const
{ // compute the best warehouse to transfer the quantity q of store s from w,
  // considering that the warehouses in vector assumed_open are open and the transfers  
  // in planned_transfers are already included
  int i, new_w, best_new_w = -1;
  CostType cost, best_cost = -1;
  for (i = 0; i < in.PreferredSuppliers(s); i++)
    {
      new_w = in.PreferredSupplier(s,i);
      if (RevisedResidualCapacity(new_w,planned_transfers) >= q 
          && Compatible(s,new_w) 
          && new_w != old_w
          ) 
        {
          if (Open(new_w) || Member(assumed_open, new_w)) // if either open or already scheduled to be opened
            {
              best_new_w = new_w; // the first open is surely the best (they are ordered by cost)
              break;
            }
          else
            {
            cost = in.FixedCost(new_w) + q * in.SupplyCost(s, new_w);
            if (best_new_w == -1 || cost < best_cost)
              {
                best_new_w = new_w;
                best_cost = cost;
              }
            }
        }
    }
  return best_new_w;
}

int FLP_Output::RevisedResidualCapacity(int w, const vector<Transfer>& transfers) const
{
  int residual_capacity = ResidualCapacity(w);

  unsigned i;
  for (i = 0; i < transfers.size(); i++)
  {
    if (transfers[i].to_w == w)
      residual_capacity -= transfers[i].quantity;
    else if (transfers[i].from_w == w)
      residual_capacity += transfers[i].quantity;
  }
  return residual_capacity;
}

CostType FLP_Output::ComputeCost() const
{
  int s, w;
  CostType cost = 0;
  int w1, w2;
  for (s = 0; s < in.Stores(); s++)
    {
      w1 = assignment[s].w1;
      cost += assignment[s].q1 * in.SupplyCost(s,w1);
      w2 = assignment[s].w2;
      if (w2 != -1)
        cost += assignment[s].q2 * in.SupplyCost(s,w2);
    }
  for (w = 0; w < in.Warehouses(); w++)
    if (load[w] > 0)
      cost += in.FixedCost(w);
  return cost;
}

int FLP_Output::ComputeViolations() const
{
  int w, violations = 0;
  for (w = 0; w < in.Warehouses(); w++)
    if (load[w] > in.Capacity(w))
      violations++;
  return violations;
}

int FLP_Output::NumberOfSigleSourceStores() const
{
  int s, count = 0;
  for (s = 0; s < in.Stores(); s++)
    if (assignment[s].w2 == -1)
      count++;
  return count;
}

int FLP_Output::NumberOfOpenWarehouses() const
{
  int w, count = 0;
  for (w = 0; w < in.Warehouses(); w++)
    if (load[w] > 0)
      count++;
  return count;
}

ostream& operator<<(ostream& os, const Suppliers& sup)
{
  return os << '(' << sup.w1 << '/' << sup.q1 << ',' <<  sup.w2 << '/' << sup.q2 << ')';
}

ostream& operator<<(ostream& os, const FLP_Output& out)
{ 
  int s;
  os << "[";
  for (s = 0; s < out.in.Stores(); s++)
    {
      os << out.assignment[s];
      if (s < out.in.Stores() - 1)
        os << ", ";
    }
  os << "]";

  return os;
}

istream& operator>>(istream& is, FLP_Output& out)
{// reads both formats, relying on the firse character: '[' = two-source, '{' = multi-source
  int s, prev_s;
  int w, w1, w2, q, q1, q2, count;
  char ch;

  out.Reset();
  is >> ch;
  if (ch == '[')
  {
    for (s = 0; s < out.in.Stores(); s++)
      {
        is >> ch >> w1 >> ch >> q1 >> ch >> w2 >> ch >> q2 >> ch >> ch;
        out.AssignFirst(s,w1,q1);
        out.AssignSecond(s,w2,q2);
      }
    is >> ch;
  }
  else if (ch == '{')
  {
    count = 1;
    prev_s = -1;
    do
      {
        is >> ch >> s >> ch >> w >> ch >> q >> ch >> ch;
        if (s == prev_s)
          {
            count++;
            if (count == 2)
              out.AssignSecond(s-1, w-1, q);
            else
              throw invalid_argument("More than two suppliers for one store");
          }
        else
          {
            count = 1;
            prev_s = s;
            out.AssignFirst(s-1, w-1, q);
          }
      }
    while (ch == ',');
  }
  else
    throw invalid_argument("Unknown solution format");
  return is;
}

bool operator==(const FLP_Output& out1, const FLP_Output& out2)
{
  int s;
  for (s = 0; s < out1.in.Stores(); s++)
    if (out1.assignment[s].w1 != out2.assignment[s].w1
     || out1.assignment[s].q1 != out2.assignment[s].q1
     || out1.assignment[s].w2 != out2.assignment[s].w2
     || out1.assignment[s].q2 != out2.assignment[s].q2)
      return false;
  return true;	
}

void FLP_Output::Dump(ostream& os) const
{
  int s, w, i;
  os << "[";
  for (s = 0; s < in.Stores(); s++)
    {
      os << '(' << assignment[s].w1 << '/' << assignment[s].q1 << ','
         <<  assignment[s].w2 << '/' << assignment[s].q2 << ')';
      if (s < in.Stores() - 1)
        os << ", ";
    }
  os << "]" << endl;
  
  os << "Load (";
  for (w = 0; w < in.Warehouses(); w++)
    { 
      os << load[w] << '/' << in.Capacity(w);
      if (w < in.Warehouses() - 1)
        os << ", ";
    }
  os << ")" << endl;
  
  int count_open = 0;
  os << endl;
  os << "Client lists:" << endl;
  for (w = 0; w < in.Warehouses(); w++)
    { 
      if (client_list[w].size() > 0)
        count_open++;
      os << w << "(" << client_list[w].size() << "): ";
      for (i = 0; i < static_cast<int>(client_list[w].size()); i++)
        {
          s = client_list[w][i];
          os << s << ' ';
        }
      os << endl;
    }
  os << count_open << " open warehouses (out of " << in.Warehouses() << ")" << endl;

  os << "Incompatibilities: " << endl;
  for (s = 0; s < in.Stores(); s++)
    { 
      for (w = 0; w < in.Warehouses(); w++)
        os << incompatible[s][w] << ' ' ;
      os << endl;
    }
  os << endl;	
}

void FLP_Output::PrettyPrint(ostream& os) const
{
  int s, w;
  os << "{";
  for (s = 0; s < in.Stores(); s++)
    {
	   w = assignment[s].w1;
       os << "(" << s+1 << ", " << w+1 << ", " << assignment[s].q1 << ")";
	   w = assignment[s].w2;	   
       if (w != -1) 
		 os << ",(" << s+1 << ", " << w+1 << ", " << assignment[s].q2 << ")";
       if (s < in.Stores() - 1) os << ", ";
    } 
  os << "}";
}

