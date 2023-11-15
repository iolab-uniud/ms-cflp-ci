// File FLP_Helpers.hh
#ifndef FLP_HELPERS_HH
#define FLP_HELPERS_HH

#include "FLP_Output.hh"
#include <easylocal.hh>

using namespace EasyLocal::Core;

/***************************************************************************
 * Solution Manager 
 ***************************************************************************/

class FLP_SolutionManager : public SolutionManager<FLP_Input,FLP_Output,DefaultCostStructure<CostType>> 
{
public:
  FLP_SolutionManager(const FLP_Input &);
  void RandomState(FLP_Output& out) override;   
  void GreedyState(FLP_Output& out) override;   
  void DumpState(const FLP_Output& out, ostream& os) const override;   
  bool CheckConsistency(const FLP_Output& st) const override;
  void PrettyPrintOutput(const FLP_Output& st, string filename) const override;
protected:
}; 

class FLP_Supply : public CostComponent<FLP_Input,FLP_Output,CostType> 
{
public:
  FLP_Supply(const FLP_Input & in, int w, bool hard) :    CostComponent<FLP_Input,FLP_Output,CostType>(in,w,hard,"FLP_Supply") {}
  CostType ComputeCost(const FLP_Output& st) const override;
  void PrintViolations(const FLP_Output& st, ostream& os = cout) const override;
};

class  FLP_Opening: public CostComponent<FLP_Input,FLP_Output,CostType> 
{
public:
  FLP_Opening(const FLP_Input & in, int w, bool hard) : CostComponent<FLP_Input,FLP_Output,CostType>(in,w,hard,"FLP_Opening") 
  {}
  CostType ComputeCost(const FLP_Output& st) const override;
  void PrintViolations(const FLP_Output& st, ostream& os = cout) const override;
};

/***************************************************************************
 * FLP_Change Neighborhood Explorer:
 ***************************************************************************/

class FLP_Change
{
  friend bool operator==(const FLP_Change& m1, const FLP_Change& m2);
  friend bool operator!=(const FLP_Change& m1, const FLP_Change& m2);
  friend bool operator<(const FLP_Change& m1, const FLP_Change& m2);
  friend ostream& operator<<(ostream& os, const FLP_Change& mv);
  friend istream& operator>>(istream& is, FLP_Change& mv);
 public:
  static bool Inverse(const FLP_Change& m1, const FLP_Change& m2) { return m1.store == m2.store; }
  int store, new_w_index;
  int new_w; 
  int old_w1, old_w2;
  Position pos; // new_w replaces either w1 or w2, quantities are computed at best
  mutable int new_q; // the new quantity assigned to the new supplier
  FLP_Change() { store = -1; }
};

class FLP_ChangeNeighborhoodExplorer
  : public NeighborhoodExplorer<FLP_Input,FLP_Output,FLP_Change,DefaultCostStructure<CostType>> 
{
public:
  FLP_ChangeNeighborhoodExplorer(const FLP_Input & pin, SolutionManager<FLP_Input,FLP_Output,DefaultCostStructure<CostType>>& psm)  
    : NeighborhoodExplorer<FLP_Input,FLP_Output,FLP_Change,DefaultCostStructure<CostType>>(pin, psm, "FLP_ChangeNeighborhoodExplorer") {} 
  void RandomMove(const FLP_Output&, FLP_Change&) const override;          
  bool FeasibleMove(const FLP_Output&, const FLP_Change&) const override;  
  void MakeMove(FLP_Output&, const FLP_Change&) const override;             
  void FirstMove(const FLP_Output&, FLP_Change&) const override;  
  bool NextMove(const FLP_Output&, FLP_Change&) const override;   
protected:
  void AnyFirstMove(const FLP_Output&, FLP_Change&) const;  
  bool AnyNextMove(const FLP_Output&, FLP_Change&) const;   
};

class FLP_ChangeDeltaSupply
  : public DeltaCostComponent<FLP_Input,FLP_Output,FLP_Change,CostType>
{
public:
  FLP_ChangeDeltaSupply(const FLP_Input & in, FLP_Supply& cc) 
    : DeltaCostComponent<FLP_Input,FLP_Output,FLP_Change,CostType>(in,cc,"FLP_ChangeDeltaSupply") {}
  CostType ComputeDeltaCost(const FLP_Output& st, const FLP_Change& mv) const override;
};

class FLP_ChangeDeltaOpening
  : public DeltaCostComponent<FLP_Input,FLP_Output,FLP_Change,CostType>
{
public:
  FLP_ChangeDeltaOpening(const FLP_Input & in, FLP_Opening& cc) 
    : DeltaCostComponent<FLP_Input,FLP_Output,FLP_Change,CostType>(in,cc,"FLP_ChangeDeltaOpening") 
  {}
  CostType ComputeDeltaCost(const FLP_Output& st, const FLP_Change& mv) const override;
};

/***************************************************************************
 * FLP_Swap Neighborhood Explorer:
 ***************************************************************************/

class FLP_Swap
{
  friend bool operator==(const FLP_Swap& m1, const FLP_Swap& m2);
  friend bool operator!=(const FLP_Swap& m1, const FLP_Swap& m2);
  friend bool operator<(const FLP_Swap& m1, const FLP_Swap& m2);
  friend ostream& operator<<(ostream& os, const FLP_Swap& mv);
  friend istream& operator>>(istream& is, FLP_Swap& mv);
 public:
  int s1, s2;
  Position pos1, pos2;  
  int w1, w2;
  int q1, q2;
  FLP_Swap() { s1 = -1; s2 = -1; }
};

class FLP_SwapNeighborhoodExplorer
  : public NeighborhoodExplorer<FLP_Input,FLP_Output,FLP_Swap,DefaultCostStructure<CostType>> 
{
public:
  FLP_SwapNeighborhoodExplorer(const FLP_Input & pin, SolutionManager<FLP_Input,FLP_Output,DefaultCostStructure<CostType>>& psm, double b = 0.0)  
    : NeighborhoodExplorer<FLP_Input,FLP_Output,FLP_Swap,DefaultCostStructure<CostType>>(pin, psm, "FLP_SwapNeighborhoodExplorer") { bias = b; } 
  void RandomMove(const FLP_Output&, FLP_Swap&) const override;          
  void RandomMove2(const FLP_Output&, FLP_Swap&) const;          
  bool FeasibleMove(const FLP_Output&, const FLP_Swap&) const override;  
  void MakeMove(FLP_Output&, const FLP_Swap&) const override;             
  void FirstMove(const FLP_Output&, FLP_Swap&) const override;  
  bool NextMove(const FLP_Output&, FLP_Swap&) const override;   
protected:
  void AnyFirstMove(const FLP_Output&, FLP_Swap&) const;  
  bool AnyNextMove(const FLP_Output&, FLP_Swap&) const;   
  double bias;
};

class FLP_SwapDeltaSupply
  : public DeltaCostComponent<FLP_Input,FLP_Output,FLP_Swap,CostType>
{
public:
  FLP_SwapDeltaSupply(const FLP_Input & in, FLP_Supply& cc) 
    : DeltaCostComponent<FLP_Input,FLP_Output,FLP_Swap,CostType>(in,cc,"FLP_SwapDeltaSupply") 
  {}
  CostType ComputeDeltaCost(const FLP_Output& st, const FLP_Swap& mv) const override;
};

// class FLP_SwapDeltaOpening  (Openings not influenced by this move)

/***************************************************************************
 * FLP_Clopen Neighborhood Explorer:
 ***************************************************************************/

inline int OccurrenciesAsFrom(const vector<Transfer>& v, int e)
{
  int count = 0;
  for (unsigned i = 0; i < v.size(); i++)
    if (v[i].from_w == e)
      count++;
  return count;
}

inline int OccurrenciesAsTo(const vector<Transfer>& v, int e)
{
  int count = 0;
  for (unsigned i = 0; i < v.size(); i++)
    if (v[i].to_w == e)
      count++;
  return count;
}

inline bool OccursPairStoreTo(const vector<Transfer>& v, int s, int w)
{
  for (unsigned i = 0; i < v.size(); i++)
    if (v[i].to_w == w && v[i].store == s)
      return true;
  return false;
}

inline bool IncompatibleTransfers(const vector<Transfer>& v, const FLP_Input& in, int s, int w)
{
  for (unsigned i = 0; i < v.size(); i++)
    if (v[i].to_w == w && in.Incompatible(v[i].store, s))
      return true;
  return false;
}

class FLP_Clopen
{
  friend bool operator==(const FLP_Clopen& m1, const FLP_Clopen& m2);
  friend bool operator!=(const FLP_Clopen& m1, const FLP_Clopen& m2);
  friend bool operator<(const FLP_Clopen& m1, const FLP_Clopen& m2);
  friend ostream& operator<<(ostream& os, const FLP_Clopen& mv);
  friend istream& operator>>(istream& is, FLP_Clopen& mv);
 public:
  int open_w, close_w, index; // if open_w = -1 --> close only, if close_w = -1 --> close only
  mutable vector<Transfer> transfer; 
  mutable vector<int> closings, openings; 
  FLP_Clopen() { open_w = -1; close_w = -1; index = -1; }
};

class FLP_ClopenNeighborhoodExplorer
  : public NeighborhoodExplorer<FLP_Input,FLP_Output,FLP_Clopen,DefaultCostStructure<CostType>> 
{
public:
  FLP_ClopenNeighborhoodExplorer(const FLP_Input & pin, SolutionManager<FLP_Input,FLP_Output,DefaultCostStructure<CostType>>& psm, double c_r, double o_r)  
    : NeighborhoodExplorer<FLP_Input,FLP_Output,FLP_Clopen,DefaultCostStructure<CostType>>(pin, psm, "FLP_ClopenNeighborhoodExplorer") 
  { close_rate = c_r; open_rate = o_r; } 
  void RandomMove(const FLP_Output&, FLP_Clopen&) const override;          
  bool FeasibleMove(const FLP_Output&, const FLP_Clopen&) const override;  
  void MakeMove(FLP_Output&, const FLP_Clopen&) const override;             
  void FirstMove(const FLP_Output&, FLP_Clopen&) const override;  
  bool NextMove(const FLP_Output&, FLP_Clopen&) const override;   
protected:
  void AnyFirstMove(const FLP_Output&, FLP_Clopen&) const;  
  bool AnyNextMove(const FLP_Output&, FLP_Clopen&) const;   
  bool ComputeAndCheckInvolvedStores(const FLP_Output&, const FLP_Clopen&) const; 
  double close_rate, open_rate;
};

class FLP_ClopenDeltaSupply
  : public DeltaCostComponent<FLP_Input,FLP_Output,FLP_Clopen,CostType>
{
public:
  FLP_ClopenDeltaSupply(const FLP_Input & in, FLP_Supply& cc) 
    : DeltaCostComponent<FLP_Input,FLP_Output,FLP_Clopen,CostType>(in,cc,"FLP_ClopenDeltaSupply") 
  {}
  CostType ComputeDeltaCost(const FLP_Output& st, const FLP_Clopen& mv) const override;
};

class FLP_ClopenDeltaOpening 
  : public DeltaCostComponent<FLP_Input,FLP_Output,FLP_Clopen,CostType>
{
public:
  FLP_ClopenDeltaOpening(const FLP_Input & in, FLP_Opening& cc) 
    : DeltaCostComponent<FLP_Input,FLP_Output,FLP_Clopen,CostType>(in,cc,"FLP_ClopenDeltaOpening") 
  {}
  CostType ComputeDeltaCost(const FLP_Output& st, const FLP_Clopen& mv) const override;
};
#endif
