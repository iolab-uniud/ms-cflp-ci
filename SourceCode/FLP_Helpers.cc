// File FLP_Helpers.cc
#include "FLP_Helpers.hh"

FLP_SolutionManager::FLP_SolutionManager(const FLP_Input & pin) 
  : SolutionManager<FLP_Input,FLP_Output,DefaultCostStructure<CostType>>(pin, "FLPSolutionManager")  {} 

void FLP_SolutionManager::DumpState(const FLP_Output& out, ostream& os) const
{
  out.Dump(os);
}

void FLP_SolutionManager::RandomState(FLP_Output& out) 
{
  int s, w1, w2, i, q1, q2;
  bool single_source;
  out.Reset();
  for (s = 0; s < in.Stores(); s++)
    {
      single_source = static_cast<bool>(Random::Uniform<int>(1,100) <= 80); // 80% single source
      do 
        {
          i = Random::Uniform<int>(0, in.PreferredSuppliers(s) - 1);
          w1 = in.PreferredSupplier(s,i);
          if (single_source)
            q1 = in.AmountOfGoods(s);
          else
            q1 = Random::Uniform<int>(1,in.AmountOfGoods(s)-1);
        } 
      while (!out.Compatible(s,w1) || out.Load(w1) + q1 > in.Capacity(w1));
      out.AssignFirst(s,w1,q1);
      if (!single_source)
        {
          do 
            {
              i = Random::Uniform<int>(0, in.PreferredSuppliers(s) - 1);
              w2 = in.PreferredSupplier(s,i);
              q2 = in.AmountOfGoods(s) - q1;
            } 
          while (w1 == w2 || !out.Compatible(s,w2) || out.Load(w2) + q2 > in.Capacity(w2));
          out.AssignSecond(s,w2,q2);
        }
    }
} 

void FLP_SolutionManager::GreedyState(FLP_Output& out) 
{
  bool found_first;
  int i, j, s, w, best_s = -1, best_i = -1, best_w = -1;
  double cost, best_cost, amortized_fixed_cost;

  int equal_bests; // used by the random tie break
  const double equal_tolerance = 0.288;
  const double amortization_factor = 0.25;
  
  vector<int> unserved_stores;

  int count = 0;
  do // repeat the full procedure until an initial feasible solution is found
  {
    out.Reset();
    unserved_stores.resize(in.Stores());
    iota(unserved_stores.begin(), unserved_stores.end(),0);
    count++;
    while(unserved_stores.size() > 0)
      {
        found_first = false;
        for (i = 0; i < static_cast<int>(unserved_stores.size()); i++)
          {
            s = unserved_stores[i];
            for (j = 0; j < in.PreferredSuppliers(s); j++)
              {
                w = in.PreferredSupplier(s,j);
                if (out.Compatible(s,w) && out.ResidualCapacity(w) > 0 
                    && (out.FirstSupplier(s) == -1 || out.ResidualCapacity(w) >= in.AmountOfGoods(s) - out.FirstQuantity(s)))
                  {
                    if (out.Closed(w)) 
                      amortized_fixed_cost = amortization_factor * (in.FixedCost(w) * 
                                                                    in.AmountOfGoods(s))/static_cast<double>(in.Capacity(w));
                    else
                      amortized_fixed_cost = 0.0;
                    if (!found_first)
                      {
                        found_first = true;
                        best_w = w;
                        best_s = s;
                        best_i = i;
                        best_cost = in.SupplyCost(s,w) + amortized_fixed_cost;
                        equal_bests = 1;
                      }
                    else
                      {
                        cost = in.SupplyCost(s,w) + amortized_fixed_cost;
                        if (cost < best_cost) // if better, becomes new best (without tolerance)
                          {
                            best_w = w;
                            best_s = s;
                            best_i = i;
                            best_cost = cost;
                            equal_bests = 1;
                          }
                        else if (cost < best_cost + equal_tolerance)
                          {
                            equal_bests++;
                            if (Random::Uniform<int>(1,equal_bests) == 1)
                              {
                                best_w = w;
                                best_s = s;
                                best_i = i;
                                // best_cost is not updated
                              }
                          }
                      }
                  }
              }
          }
        if (!found_first)
          break;
        if (out.FirstSupplier(best_s) == -1)
          { 
            if (out.ResidualCapacity(best_w) >= in.AmountOfGoods(best_s))
              {
                out.FullAssign(best_s,best_w);
                unserved_stores.erase(unserved_stores.begin() + best_i);
              }
            else
              {
                out.AssignFirst(best_s,best_w,out.ResidualCapacity(best_w));
              }
          }
        else
          {
            out.AssignSecond(best_s,best_w,in.AmountOfGoods(best_s) - out.FirstQuantity(best_s));
            unserved_stores.erase(unserved_stores.begin() + best_i);
          }
      }
    if (count == 50)
      {
        cout << "{\"cost\": " << 100000000000 <<  ", \"greedy\": \"infeasible\"}" << endl;
        exit(0);
      }
  }
  while (unserved_stores.size() > 0);
}

bool FLP_SolutionManager::CheckConsistency(const FLP_Output& st) const
{
  int w, load, i, s;
  // TODO: need to check some other cases
  for (w = 0; w < in.Warehouses(); w++)
  {
    if (st.ResidualCapacity(w) < 0)
      {
        cerr << "Excessive load for warehouse " << w << ": capacity " << in.Capacity(w) << ", load " << st.Load(w) << endl;
        return false;
      }
    load = 0;
    for (i = 0; i < st.Clients(w); i++)
      {
        s = st.Client(w,i);
        if (st.FirstSupplier(s) == w)
          load += st.FirstQuantity(s);
        else if (st.SecondSupplier(s) == w)
          load += st.SecondQuantity(s);
        else
          {
            cerr << "Inconsistency between warehouse " << w << " and store " << s << endl;
            return false;
          }
      }
    if (st.Load(w) != load)
      {
        cerr << "Warehouse " << w << " with stored load " << st.Load(w) << " and computed load " << load << endl;
        return false;
      }
  }	
  for (s = 0; s < in.Stores(); s++)
    {
      if (st.FirstQuantity(s) + st.SecondQuantity(s) != in.AmountOfGoods(s))
        {
          cerr << "Store " << s << " is not supplied correctly: " << st.FirstQuantity(s) 
               << "+" << st.SecondQuantity(s) << "!=" << in.AmountOfGoods(s) << endl;
          return false;
        }
      if (st.FirstQuantity(s) <= 0)
        {
          cerr  << "Store " << s << " is not supplied correctly: first quantity is " << st.FirstQuantity(s) << endl;
          return false;
        }
      if (st.SecondQuantity(s) < 0 || (st.SecondQuantity(s) == 0 && st.SecondSupplier(s) != -1))
        {
          cerr  << "Store " << s << " is not supplied correctly: second quantity is " << st.SecondQuantity(s) 
                << " (with second supplier " << st.SecondSupplier(s) << ")" << endl;
          return false;
        }
      if (st.SecondSupplier(s) != -1 && in.SupplyCost(s,st.FirstSupplier(s)) > in.SupplyCost(s,st.SecondSupplier(s)))
        {
          cerr << "Reversed warehouses for store " << s << endl;
          return false;
        }
      if (st.FirstSupplier(s) == st.SecondSupplier(s))
        {
          cerr << "Identical suppliers for store " << s << endl;
          return false;
        }
      if (!st.Compatible(s,st.FirstSupplier(s)))
        {
          cerr << "Store " << s << " served (as first) by incompatible warehouse " << st.FirstSupplier(s) << endl;
          return false;
        }
      if (st.SecondSupplier(s) != -1 && !st.Compatible(s,st.SecondSupplier(s)))
        {
          cerr << "Store " << s << " served (as sencond) by incompatible warehouse " << st.SecondSupplier(s) << endl;
          return false;
        }
    }
  return true;
}

void FLP_SolutionManager::PrettyPrintOutput(const FLP_Output& st, string filename) const
{
  ofstream os(filename);
  st.PrettyPrint(os);
  if (!CheckConsistency(st))
  {
    cerr << "State non consistent" << endl;
  }
  os.close();
}

CostType FLP_Supply::ComputeCost(const FLP_Output& st) const
{
  int s;
  CostType cost = 0;
  for (s = 0; s < in.Stores(); s++)
  {
    cost += static_cast<CostType>(st.FirstQuantity(s)) * in.SupplyCost(s,st.FirstSupplier(s));
    if (st.SecondSupplier(s) != -1)
      cost +=  static_cast<CostType>(st.SecondQuantity(s)) * in.SupplyCost(s,st.SecondSupplier(s));
  }
  return cost;
}

void FLP_Supply::PrintViolations(const FLP_Output& st, ostream& os) const
{
  int s;
  for (s = 0; s < in.Stores(); s++)
  {
    os << "The cost of supplying " << st.FirstQuantity(s) << " units from " << st.FirstSupplier(s) 
       << " to " << s << " is " << st.FirstQuantity(s)*in.SupplyCost(s,st.FirstSupplier(s)) 
       << " (cost per unit " << in.SupplyCost(s,st.FirstSupplier(s)) << ")" << endl;
    if (st.SecondSupplier(s) != -1)
      os << "The cost of supplying " << st.SecondQuantity(s) << " units from " << st.SecondSupplier(s) 
         << " to " << s << " is " << st.SecondQuantity(s)*in.SupplyCost(s,st.SecondSupplier(s)) 
         << " (cost per unit " << in.SupplyCost(s,st.SecondSupplier(s)) << ")" << endl;	   
  }
}

CostType FLP_Opening::ComputeCost(const FLP_Output& st) const
{ 
  int w, cost = 0;
  for (w = 0; w < in.Warehouses(); w++)
    if (st.Clients(w) > 0)
      cost += in.FixedCost(w);
  return cost;
}

void FLP_Opening::PrintViolations(const FLP_Output& st, ostream& os) const
{
  int w;
  for (w = 0; w < in.Warehouses(); w++)
    if (st.Clients(w) > 0)
      os << "The cost of opening warehouse " << w << " is " << in.FixedCost(w) << endl;
}

/*****************************************************************************
 * FLP_Change Neighborhood Explorer Methods
 *****************************************************************************/

// ------------- FLP_Change move
bool operator==(const FLP_Change& mv1, const FLP_Change& mv2)
{
  return mv1.store == mv2.store && mv1.new_w == mv2.new_w && mv1.pos == mv2.pos;
}

bool operator!=(const FLP_Change& mv1, const FLP_Change& mv2)
{
  return mv1.store != mv2.store || mv1.new_w != mv2.new_w || mv1.pos != mv2.pos;
}

bool operator<(const FLP_Change& mv1, const FLP_Change& mv2)
{
  return (mv1.store < mv2.store)
  || (mv1.store == mv2.store && mv1.new_w < mv2.new_w)
  || (mv1.store == mv2.store && mv1.new_w == mv2.new_w && mv1.pos < mv2.pos);
}

istream& operator>>(istream& is, FLP_Change& mv)
{// FIXME: computed attributes are not read (neither recomputed)
  char ch, ch1;
  is >> mv.store >> ch >> ch >> ch1 >> ch >> ch >> mv.new_w >> ch >> mv.new_q;
  mv.pos = (ch1 == '1' ? Position::FIRST : Position::SECOND);
  return is;
}

ostream& operator<<(ostream& os, const FLP_Change& mv)
{
  char ch = (mv.pos == Position::FIRST ? '1' : '2');
  os << mv.store << ":--" << ch << "(" << (ch == '1' ? mv.old_w1 : mv.old_w2) << ")->" 
     << mv.new_w << '/' << mv.new_q;
  return os;
}

void FLP_ChangeNeighborhoodExplorer::RandomMove(const FLP_Output& st, FLP_Change& mv) const
{
  do 
    {
      mv.store = Random::Uniform<int>(0, in.Stores()-1);
      mv.old_w1 = st.FirstSupplier(mv.store);
      mv.old_w2 = st.SecondSupplier(mv.store);
      if (mv.old_w2 == -1)
        mv.pos = static_cast<Position>(Random::Uniform<int>(0, 1));
      else
        mv.pos = Position::SECOND; // case mv.pos == Position::FIRST && mv.old_w2 != -1 eliminated (Andrea 4/5/2023)
      mv.new_w_index = Random::Uniform<int>(0, in.PreferredSuppliers(mv.store)-1);
      mv.new_w = in.PreferredSupplier(mv.store,mv.new_w_index);
    }
  while (!FeasibleMove(st,mv));
} 

bool FLP_ChangeNeighborhoodExplorer::FeasibleMove(const FLP_Output& st, const FLP_Change& mv) const
{
  if ( mv.new_w == mv.old_w1 
       || mv.new_w == mv.old_w2 
       || !st.Compatible(mv.store,mv.new_w)
       || (mv.pos == Position::FIRST && mv.old_w2 != -1) // CASE II eliminated (at present not generated by RandomMove, but generated by NextMove)
       || st.ResidualCapacity(mv.new_w) <= 0) // new_w must have some space (because even when we rebalance, something goes to new_w)
    return false;
  else
    { // CheckAndComputeQuantity returns -1 if it is infeasible
      mv.new_q = st.CheckAndComputeQuantity(mv.store,mv.new_w,mv.pos);
      return mv.new_q != -1;
    }
} 

void FLP_ChangeNeighborhoodExplorer::MakeMove(FLP_Output& st, const FLP_Change& mv) const
{ 
  // cerr << 1;
  //  int revised_q = in.AmountOfGoods(mv.store) - mv.new_q;
  if (mv.pos == Position::FIRST)
    {
      st.ChangeFirstSupplierAndQuantity(mv.store,mv.new_w,mv.new_q);
    }
  else
    {
      st.ChangeSecondSupplierAndQuantity(mv.store,mv.new_w,mv.new_q);
    }
//   if (!sm.CheckConsistency(st))
//     {
//       cerr << mv << endl;
//       exit(1);
//     }
}  

void FLP_ChangeNeighborhoodExplorer::FirstMove(const FLP_Output& st, FLP_Change& mv) const
{
  AnyFirstMove(st,mv);
  while (!FeasibleMove(st,mv))
    AnyNextMove(st,mv);	
}

void FLP_ChangeNeighborhoodExplorer::AnyFirstMove(const FLP_Output& st, FLP_Change& mv) const
{
  mv.store = 0;
  mv.new_w_index = 0; 
  mv.new_w = in.PreferredSupplier(mv.store,mv.new_w_index); 
  mv.pos = Position::FIRST;
  mv.old_w1 = st.FirstSupplier(mv.store);
  mv.old_w2 = st.SecondSupplier(mv.store);
}

bool FLP_ChangeNeighborhoodExplorer::NextMove(const FLP_Output& st, FLP_Change& mv) const
{
  do
    if (!AnyNextMove(st,mv))
      return false;
  while (!FeasibleMove(st,mv));
  return true;
}

bool FLP_ChangeNeighborhoodExplorer::AnyNextMove(const FLP_Output& st, FLP_Change& mv) const
{
 if (mv.pos == Position::FIRST)
  {
    mv.pos = Position::SECOND;
    return true;	 
  }
  else if (mv.new_w_index < in.PreferredSuppliers(mv.store) - 1)
    {
      mv.new_w_index++; 
      mv.new_w = in.PreferredSupplier(mv.store,mv.new_w_index);
      mv.pos = Position::FIRST;
      return true;
    }
  else if (mv.store < in.Stores() - 1)
    {
      mv.store++;
      mv.pos = Position::FIRST;
      mv.new_w_index = 0; 
      mv.new_w = in.PreferredSupplier(mv.store,mv.new_w_index);
      mv.old_w1 = st.FirstSupplier(mv.store);
      mv.old_w2 = st.SecondSupplier(mv.store);
      return true;
    }
  else
    return false;
}

CostType FLP_ChangeDeltaSupply::ComputeDeltaCost(const FLP_Output& st, const FLP_Change& mv) const
{
  CostType cost = 0;
  cost += mv.new_q * in.SupplyCost(mv.store,mv.new_w);
  if (mv.pos == Position::FIRST)
    {
      cost -= st.FirstQuantity(mv.store) * in.SupplyCost(mv.store,mv.old_w1);
      // rebalance delta cost (0 if mv.new_q == st.FirstQuantity(mv.store))
      cost += (st.FirstQuantity(mv.store) - mv.new_q) * in.SupplyCost(mv.store,mv.old_w2); // positive or negative
    } 
  else
    {
      if (mv.old_w2 != -1)
        cost -= st.SecondQuantity(mv.store) * in.SupplyCost(mv.store,mv.old_w2);
      // rebalance delta cost (0 if mv.new_q == st.SecondQuantity(mv.store))
      cost += (st.SecondQuantity(mv.store) - mv.new_q) * in.SupplyCost(mv.store,mv.old_w1);
    } 
  return cost;
}

CostType FLP_ChangeDeltaOpening::ComputeDeltaCost(const FLP_Output& st, const FLP_Change& mv) const
{
  int cost = 0;
  if (mv.new_q > 0 && st.Clients(mv.new_w) == 0)
    cost += in.FixedCost(mv.new_w);
  if (mv.pos == Position::FIRST)
    {
      if (st.Clients(mv.old_w1) == 1)  // remove the last client
        cost -= in.FixedCost(mv.old_w1);
    }
  else
    {
      if (mv.old_w2 != -1 && st.Clients(mv.old_w2) == 1)  
        cost -= in.FixedCost(mv.old_w2); 
    }
  return cost;
}

/*****************************************************************************
 * FLP_Swap Neighborhood Explorer Methods
 *****************************************************************************/

bool operator==(const FLP_Swap& mv1, const FLP_Swap& mv2)
{
  return mv1.s1 == mv2.s1 && mv1.s2 == mv2.s2 && mv1.pos1 == mv2.pos1 && mv1.pos2 == mv2.pos2;
}

bool operator!=(const FLP_Swap& mv1, const FLP_Swap& mv2)
{
  return mv1.s1 != mv2.s1 || mv1.s2 != mv2.s2 || mv1.pos1 != mv2.pos1 || mv1.pos2 != mv2.pos2;
}

bool operator<(const FLP_Swap& mv1, const FLP_Swap& mv2)
{
  return (mv1.s1 < mv2.s1)
  || (mv1.s1 == mv2.s1 && mv1.s2 < mv2.s2)
  || (mv1.s1 == mv2.s1 && mv1.s2 == mv2.s2 && mv1.pos1 < mv2.pos1)
  || (mv1.s1 == mv2.s1 && mv1.s2 == mv2.s2 && mv1.pos1 == mv2.pos1 && mv1.pos2 < mv2.pos2);
}

istream& operator>>(istream& is, FLP_Swap& mv)
{
  char ch, p1, p2;
  is >> mv.s1 >> ch >> p1 >> ch >> mv.w1 >> ch >> ch >> ch >> mv.s2 >> ch >> p2 >> ch >> mv.w2;
  mv.pos1 = (p1 == '1' ? Position::FIRST : Position::SECOND);
  mv.pos2 = (p2 == '1' ? Position::FIRST : Position::SECOND);
  return is;
}

ostream& operator<<(ostream& os, const FLP_Swap& mv)
{
  os << mv.s1 << "^" << (mv.pos1 == Position::FIRST ? '1' : '2') << '/' << mv.w1 << "<->" 
     << mv.s2 << "^" << (mv.pos2 == Position::FIRST ? '1' : '2') << '/' << mv.w2;
  return os;
}

void FLP_SwapNeighborhoodExplorer::RandomMove2(const FLP_Output& st, FLP_Swap& mv) const
{ 
  int i;
  int count = 0;
  do 
  {
    mv.s1 = Random::Uniform<int>(0, in.Stores() - 1);
    if (st.SecondSupplier(mv.s1) != -1)
      { // the second is selected with probability bias + (1-bias)/2  --> (bias+1)/2
        if (Random::Uniform<double>(0.0,1.0) <= (bias+1)/2.0)
          mv.pos1 = Position::SECOND;
        else
          mv.pos1 = Position::FIRST;
        // if (Random::Uniform<double>(0.0,1.0) <= bias)
          // mv.pos1 = Position::SECOND;
        // else
          // mv.pos1 = static_cast<Position>(Random::Uniform<int>(0,1));
      }
    else
      mv.pos1 = Position::FIRST;
    if (mv.pos1 == Position::FIRST)
      {
        mv.w1 = st.FirstSupplier(mv.s1);
        mv.q1 = st.FirstQuantity(mv.s1);
      }
    else
      {
        mv.w1 = st.SecondSupplier(mv.s1);
        mv.q1 = st.SecondQuantity(mv.s1);
      }
   
    do // draw another preferred supplier of s1
      {
        i = Random::Uniform<int>(0, in.PreferredSuppliers(mv.s1) - 1);
        mv.w2 = in.PreferredSupplier(mv.s1, i);
      }
    while (st.Clients(mv.w2) == 0);
    i = Random::Uniform<int>(0, st.Clients(mv.w2)-1);
    mv.s2 = st.Client(mv.w2,i);
    
    if (st.FirstSupplier(mv.s2) == mv.w2)
      {
        mv.pos2 = Position::FIRST;
        mv.q2 = st.FirstQuantity(mv.s2);
      }
    else
      {
        mv.pos2 = Position::SECOND;
        mv.q2 = st.SecondQuantity(mv.s2);
      }
    count++;
  }
  while (!FeasibleMove(st,mv));
//   cerr << count << ' ';
  if (mv.s2 < mv.s1)
    {
      swap(mv.s1,mv.s2);
      swap(mv.w1,mv.w2);
      swap(mv.q1,mv.q2);
      swap(mv.pos1,mv.pos2);
    }
}

void FLP_SwapNeighborhoodExplorer::RandomMove(const FLP_Output& st, FLP_Swap& mv) const
{ // s2 is drawn among the preferred clients of w1
  // NOTE: in the old version s2 was drawn fully randomly 
  // (less efficient because many moves are infeasible)
  int i;
  do 
  {
    mv.s1 = Random::Uniform<int>(0, in.Stores() - 1);
    if (st.SecondSupplier(mv.s1) != -1)
      {
        if (Random::Uniform<double>(0.0,1.0) <= bias)
          mv.pos1 = Position::SECOND;
        else
          mv.pos1 = static_cast<Position>(Random::Uniform<int>(0,1));
      }
    else
      mv.pos1 = Position::FIRST;
    if (mv.pos1 == Position::FIRST)
      {
        mv.w1 = st.FirstSupplier(mv.s1);
        mv.q1 = st.FirstQuantity(mv.s1);
      }
    else
      {
        mv.w1 = st.SecondSupplier(mv.s1);
        mv.q1 = st.SecondQuantity(mv.s1);		
      }
    do 
      {
        i = Random::Uniform<int>(0, in.PreferredClients(mv.w1) - 1);
        mv.s2 = in.PreferredClient(mv.w1, i);
      }
    while (mv.s2 == mv.s1);
    if (st.SecondSupplier(mv.s2) != -1)
      {
        if (Random::Uniform<double>(0.0,1.0) <= bias)
          mv.pos2 = Position::SECOND;
        else      
          mv.pos2 = static_cast<Position>(Random::Uniform<int>(0,1));
      }
    else
      mv.pos2 = Position::FIRST;
    if (mv.pos2 == Position::FIRST)
      {
        mv.w2 = st.FirstSupplier(mv.s2);
        mv.q2 = st.FirstQuantity(mv.s2);
      }
    else
      {
        mv.w2 = st.SecondSupplier(mv.s2);
        mv.q2 = st.SecondQuantity(mv.s2);		
      }
  }
  while (!FeasibleMove(st,mv));
  if (mv.s2 < mv.s1)
    {
      swap(mv.s1,mv.s2);
      swap(mv.w1,mv.w2);
      swap(mv.q1,mv.q2);
      swap(mv.pos1,mv.pos2);
    }
} 

bool FLP_SwapNeighborhoodExplorer::FeasibleMove(const FLP_Output& st, const FLP_Swap& mv) const
{
  if (mv.w1 == mv.w2)
    return false;  
  if (  (st.ResidualCapacity(mv.w2) < mv.q1 - mv.q2)
    ||  (st.ResidualCapacity(mv.w1) < mv.q2 - mv.q1))
      return false;  
  if (in.Incompatible(mv.s1,mv.s2)) // in this case the incompatibility is present, but is removed
    {
      return st.AlmostCompatible(mv.s1,mv.w2) && st.AlmostCompatible(mv.s2,mv.w1);
    }
  else
    {
      return st.Compatible(mv.s1,mv.w2) && st.Compatible(mv.s2,mv.w1); 
    }
} 

void FLP_SwapNeighborhoodExplorer::MakeMove(FLP_Output& st, const FLP_Swap& mv) const
{
  st.ReplaceSupplier(mv.s1,mv.pos1,mv.w2,mv.q1);
  st.ReplaceSupplier(mv.s2,mv.pos2,mv.w1,mv.q2);
}  

void FLP_SwapNeighborhoodExplorer::FirstMove(const FLP_Output& st, FLP_Swap& mv) const
{
  AnyFirstMove(st,mv);
  while (!FeasibleMove(st,mv))
    AnyNextMove(st,mv);	
}

void FLP_SwapNeighborhoodExplorer::AnyFirstMove(const FLP_Output& st, FLP_Swap& mv) const
{
  mv.s1 = 0;
  mv.pos1 = Position::FIRST;
  mv.w1 = st.FirstSupplier(mv.s1);
  mv.q1 = st.FirstQuantity(mv.s1);
  mv.s2 = 1;
  mv.pos2 = Position::FIRST;
  mv.w2 = st.FirstSupplier(mv.s2);
  mv.q2 = st.FirstQuantity(mv.s2);
}

bool FLP_SwapNeighborhoodExplorer::NextMove(const FLP_Output& st, FLP_Swap& mv) const
{
  do
    if (!AnyNextMove(st,mv))
      return false;
  while (!FeasibleMove(st,mv));
  return true;
}

bool FLP_SwapNeighborhoodExplorer::AnyNextMove(const FLP_Output& st, FLP_Swap& mv) const
{
  if (mv.pos2 == Position::FIRST && st.SecondSupplier(mv.s2) != -1)
  {
    mv.pos2 = Position::SECOND;
    mv.w2 = st.SecondSupplier(mv.s2);
    mv.q2 = st.SecondQuantity(mv.s2);
    return true;	 
  }
  else if (mv.pos1 == Position::FIRST && st.SecondSupplier(mv.s1) != -1)
  {
    mv.pos1 = Position::SECOND;
    mv.w1 = st.SecondSupplier(mv.s1);
    mv.q1 = st.SecondQuantity(mv.s1);
    mv.pos2 = Position::FIRST;
    mv.w2 = st.FirstSupplier(mv.s2);
    mv.q2 = st.FirstQuantity(mv.s2);
    return true;	 	  
  }
  else if (mv.s2 < in.Stores() - 1)
  {
    mv.s2++;
    mv.pos1 = Position::FIRST;
    mv.w1 = st.FirstSupplier(mv.s1);
    mv.q1 = st.FirstQuantity(mv.s1);
    mv.pos2 = Position::FIRST;
    mv.w2 = st.FirstSupplier(mv.s2);
    mv.q2 = st.FirstQuantity(mv.s2);
    return true;	 	  
  }
  else if (mv.s1 < in.Stores() - 2)
  {
    mv.s1++;
    mv.pos1 = Position::FIRST;
    mv.w1 = st.FirstSupplier(mv.s1);
    mv.q1 = st.FirstQuantity(mv.s1);
    mv.s2 = mv.s1 + 1;
    mv.pos2 = Position::FIRST;
    mv.w2 = st.FirstSupplier(mv.s2);
    mv.q2 = st.FirstQuantity(mv.s2);
    return true;	 	  	 
  }
  else
    return false;
}

CostType FLP_SwapDeltaSupply::ComputeDeltaCost(const FLP_Output& st, const FLP_Swap& mv) const
{
  return mv.q1 * (in.SupplyCost(mv.s1,mv.w2) - in.SupplyCost(mv.s1,mv.w1))
    + mv.q2 * (in.SupplyCost(mv.s2,mv.w1) - in.SupplyCost(mv.s2,mv.w2));
}

/*****************************************************************************
 * FLP_Clopen Neighborhood Explorer Methods
 *****************************************************************************/

ostream& operator<<(ostream& os, const Transfer& t)
{
  os << "(" << t.store << "," << t.from_w << "," << t.to_w << ',' << t.quantity << ") ";
  return os;
}

bool operator==(const FLP_Clopen& mv1, const FLP_Clopen& mv2)
{
  return mv1.open_w == mv2.open_w && mv1.close_w == mv2.close_w;
}

bool operator!=(const FLP_Clopen& mv1, const FLP_Clopen& mv2)
{
  return  mv1.open_w != mv2.open_w || mv1.close_w != mv2.close_w;
}

bool operator<(const FLP_Clopen& mv1, const FLP_Clopen& mv2)
{
  return (mv1.open_w < mv2.open_w) 
    || (mv1.open_w == mv2.open_w && mv1.close_w < mv2.close_w);
}

istream& operator>>(istream& is, FLP_Clopen& mv)
{// TODO: read all 
  char ch;
  is >> ch >> mv.close_w >> ch >> mv.open_w >> ch;
  return is;
}

ostream& operator<<(ostream& os, const FLP_Clopen& mv)
{
  unsigned i;
  os << "<" << mv.close_w << "," << mv.open_w << ">";
  for (i = 0; i < mv.transfer.size(); i++)
    {
      os << mv.transfer[i];
    }
  os << "[";
  for (i = 0; i < mv.closings.size(); i++)
    {
      os << mv.closings[i];
      if (i < mv.closings.size() - 1) os << " ";
    }
  os << "]";
  os << "[";
  for (i = 0; i < mv.openings.size(); i++)
    {
      os << mv.openings[i];
      if (i < mv.openings.size() - 1) os << " ";
    }      
  os << "]";
  return os;
}

void FLP_ClopenNeighborhoodExplorer::RandomMove(const FLP_Output& st, FLP_Clopen& mv) const
{ 
  float draw;
  do 
  {
    draw = Random::Uniform<float>(0.0,1.0);
    if (draw < close_rate)
      {
        mv.open_w = -1;
        do 
          mv.close_w = Random::Uniform<int>(0, in.Warehouses() - 1);
        while (st.Closed(mv.close_w));
      }
    else if (draw < close_rate + open_rate)
      {
        mv.close_w = -1;
        do 
          mv.open_w = Random::Uniform<int>(0, in.Warehouses() - 1);
        while (st.Open(mv.open_w));
      }
    else
      {
        mv.index = Random::Uniform<int>(0, in.NeighborWarehousePairs() - 1);
        tie(mv.open_w,mv.close_w) = in.NeighborWarehouses(mv.index);
        if (st.Open(mv.open_w)) // if mv.open_w is open test the pair in reverse order
          swap(mv.open_w,mv.close_w);
      }
  }
  while (!FeasibleMove(st,mv));
  // do 
  // {
    // mv.open_w = Random::Uniform<int>(0, in.Warehouses() - 1);
    // mv.close_w = Random::Uniform<int>(0, in.Warehouses() - 1);
  // }
  // while (!FeasibleMove(st,mv));
} 

bool FLP_ClopenNeighborhoodExplorer::FeasibleMove(const FLP_Output& st, const FLP_Clopen& mv) const
{
  return 
    (mv.open_w == -1 || st.Closed(mv.open_w)) 
    && (mv.close_w == -1 || st.Open(mv.close_w))
    && (mv.close_w != -1 || mv.open_w != -1)
    && ComputeAndCheckInvolvedStores(st,mv); 
} 

bool FLP_ClopenNeighborhoodExplorer::ComputeAndCheckInvolvedStores(const FLP_Output& st, const FLP_Clopen& mv) const
{  
  int i, s, old_w, new_w, q, new_load = 0;
  bool second_supplier_checked;
  mv.transfer.clear();
  mv.openings.clear();
  mv.closings.clear();

  if (mv.open_w != -1)
    mv.openings.push_back(mv.open_w); // this is needed by the closing part

  // transfer from the to-be-closed warehouse
  if (mv.close_w != -1)
    {
      mv.closings.push_back(mv.close_w); 
      for (i = 0; i < st.Clients(mv.close_w); i++)
        {
          s = st.Client(mv.close_w,i);		  
          q = (mv.close_w == st.FirstSupplier(s) ? st.FirstQuantity(s) : st.SecondQuantity(s));
          new_w = st.BestTransfer(s,mv.close_w,q,mv.openings,mv.transfer); // mv.open_w is included in openings
          if (new_w == -1)
            return false;
          if (new_w == mv.open_w) new_load += q;
          mv.transfer.push_back(Transfer(s,mv.close_w,new_w,q));
          // cerr << i << " " << mv.transfer.back() << endl;
          if (st.Closed(new_w) && !Member(mv.openings,new_w)) // mv.open_w is included in openings
            mv.openings.push_back(new_w);
        }
    }

  if (mv.open_w != -1)
    {
      // transfer to the to-be-open warehouse
      second_supplier_checked = false;
      i = 0;
      while (i < in.PreferredClients(mv.open_w))
        {
          s = in.PreferredClient(mv.open_w,i);
          if (!st.Compatible(s,mv.open_w) || IncompatibleTransfers(mv.transfer,in,s,mv.open_w)) { i++; continue; }
          if (st.SecondSupplier(s) == -1 || second_supplier_checked)
            { 
              old_w = st.FirstSupplier(s);
              q = st.FirstQuantity(s);
              i++; // there is only one supplier of s or the second has been checked, move on to next
              second_supplier_checked = false;
            }
          else 
            {
              old_w = st.SecondSupplier(s);
              q = st.SecondQuantity(s);
              second_supplier_checked = true; // don't move on (first supplier tested later)
            }
          if (old_w == mv.close_w || OccursPairStoreTo(mv.transfer,s,old_w))
            continue; // do not double transfer
          if (new_load + q <= in.Capacity(mv.open_w))
            {
              if (OccurrenciesAsFrom(mv.transfer,old_w) - OccurrenciesAsTo(mv.transfer,old_w) == st.Clients(old_w) - 1)
                { // if old_w gets closed by the move, include it without checking the cost
                  mv.closings.push_back(old_w); 
                  mv.transfer.push_back(Transfer(s,old_w,mv.open_w,q));
                  new_load += q;
                  // cerr << mv.transfer.back() << endl;      
                }
              else if (in.SupplyCost(s,mv.open_w) < in.SupplyCost(s,old_w))
                {
                  mv.transfer.push_back((Transfer(s,old_w,mv.open_w,q)));
                  new_load += q;
                  // cerr << mv.transfer.back() << endl;      
                }         
            }
          if (new_load == in.Capacity(mv.open_w))
            break;
        }
      return new_load > 0;
    }
  else
    return true;
}

void FLP_ClopenNeighborhoodExplorer::MakeMove(FLP_Output& st, const FLP_Clopen& mv) const
{
  // cerr << 5;
  unsigned i;
  for (i = 0; i < mv.transfer.size(); i++)
    {
      if (st.FirstSupplier(mv.transfer[i].store) == mv.transfer[i].from_w)
        st.ReplaceSupplier(mv.transfer[i].store,Position::FIRST,mv.transfer[i].to_w,mv.transfer[i].quantity);
      else // if (st.SecondSupplier(mv.transfer[i].store) == mv.transfer[i].from_w)
        st.ReplaceSupplier(mv.transfer[i].store,Position::SECOND,mv.transfer[i].to_w,mv.transfer[i].quantity);
    }
//   if (!sm.CheckConsistency(st))
//     {
//       cerr << mv << endl;
//       exit(1);
//     }
}  

void FLP_ClopenNeighborhoodExplorer::FirstMove(const FLP_Output& st, FLP_Clopen& mv) const
{
  AnyFirstMove(st,mv);
  while (!FeasibleMove(st,mv))
    AnyNextMove(st,mv);	
}

void FLP_ClopenNeighborhoodExplorer::AnyFirstMove(const FLP_Output& st, FLP_Clopen& mv) const
{ // the first move is an opening move (then closing ones, then flip ones)
  mv.close_w = -1;
  mv.open_w = 0;
  mv.index = -1;
}

bool FLP_ClopenNeighborhoodExplorer::NextMove(const FLP_Output& st, FLP_Clopen& mv) const
{
  do
    if (!AnyNextMove(st,mv))
      return false;
  while (!FeasibleMove(st,mv));
  return true;
}

bool FLP_ClopenNeighborhoodExplorer::AnyNextMove(const FLP_Output& st, FLP_Clopen& mv) const
{
  if (mv.close_w == -1)
    {
      mv.open_w++;
      if (mv.open_w == in.Warehouses())
        {
          mv.open_w = -1;
          mv.close_w = 0;
        }
      return true;
    }
  else if (mv.open_w == -1)
    {
      mv.close_w++; 
      if (mv.close_w == in.Warehouses())
        {
          mv.index = 0;
          tie(mv.open_w,mv.close_w) = in.NeighborWarehouses(mv.index);
          if (st.Open(mv.open_w)) // if mv.open_w is open test the pair in reverse order
            swap(mv.open_w,mv.close_w);
        }
      return true;
    }
  else if (mv.index < in.NeighborWarehousePairs() - 1)
    { // recall that mv.index is initially -1 
      mv.index++;
      tie(mv.open_w,mv.close_w) = in.NeighborWarehouses(mv.index);
      if (st.Open(mv.open_w)) // if mv.open_w is open test the pair in reverse order
        swap(mv.open_w,mv.close_w);
      return true;
    }
  else
    return false;
}

CostType FLP_ClopenDeltaSupply::ComputeDeltaCost(const FLP_Output& st, const FLP_Clopen& mv) const
{ 
  int cost = 0;
  unsigned i;
  for (i = 0; i < mv.transfer.size(); i++)
  {
    cost += mv.transfer[i].quantity * 
      (in.SupplyCost(mv.transfer[i].store,mv.transfer[i].to_w) 
       - in.SupplyCost(mv.transfer[i].store,mv.transfer[i].from_w));
  }
  return cost;
}

CostType FLP_ClopenDeltaOpening::ComputeDeltaCost(const FLP_Output& st, const FLP_Clopen& mv) const 
{ 
  int cost = 0;
  for (unsigned i = 0; i < mv.closings.size(); i++)
    cost -= in.FixedCost(mv.closings[i]);
  for (unsigned i = 0; i < mv.openings.size(); i++)
    cost += in.FixedCost(mv.openings[i]);
  return cost;
}

