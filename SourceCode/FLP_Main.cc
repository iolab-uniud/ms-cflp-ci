#include "FLP_Helpers.hh"

using namespace EasyLocal::Debug;

int main(int argc, const char* argv[])
{
  ParameterBox main_parameters("main", "Main Program options");
  Parameter<string> instance("instance", "Input instance", main_parameters); 
  Parameter<unsigned> seed("seed", "Random seed", main_parameters);
  Parameter<string> method("method", "Solution method (empty for tester)", main_parameters);   
  Parameter<string> init_state("init_state", "Initial state (to be read from file)", main_parameters);
  Parameter<string> output_file("output_file", "Write the output to a file (filename required)", main_parameters);
  Parameter<double> swap_rate("swap_rate", "Swap rate", main_parameters);
  Parameter<double> swap_bias("swap_bias", "Swap bias toward second supplier", main_parameters);
  Parameter<double> close_rate("close_irate", "Close internal rate", main_parameters);
  Parameter<double> open_rate("open_irate", "Open internal rate", main_parameters);
  Parameter<double> clopen_rate("clopen_rate", "Clopen rate", main_parameters);
  Parameter<string> init_state_strategy("init_state_strategy", "Initial state strategy (random or greedy)", main_parameters);
  Parameter<int> timeout_factor("timeout_factor", "Timeout factor for sqrt ration (default = 10)", main_parameters);

  swap_rate = 0.19;
  swap_bias = 0.44;
  close_rate = 0.33; 
  open_rate = 0.33;
  clopen_rate = 0.1;
  init_state_strategy = "greedy";
  timeout_factor = 10;

  Parameter<string> timeout_mode("timeout_mode", "Timeout mode", main_parameters);
  timeout_mode = "sqrt";
  
  ParameterBox input_parameters("input", "Input Program options");
  Parameter<double> sqrt_ratio_preferred("sqrt_ratio_preferred", "Square root ratio of preferred warehouses for store", input_parameters);
  Parameter<int> cost_diff_threshold("diff_threshold", "Threshold of the difference w.r.t. the minimum cost", input_parameters);

  sqrt_ratio_preferred = 1.0;
  cost_diff_threshold = 100;

  // 3rd parameter: false = do not check unregistered parameters, 4th parameter: true = silent
  CommandLineParameters::Parse(argc, argv, false, true);  

  if (!instance.IsSet())
    {
      cout << "Error: --main::instance filename option must always be set" << endl;
      return 1;
    }
  FLP_Input in(instance,  sqrt_ratio_preferred, cost_diff_threshold);

  FLP_Output init(in);  

  if (seed.IsSet())
    Random::SetSeed(seed);

  FLP_Supply cc1(in, 1, false);
  FLP_Opening cc2(in, 1, false);
 
  FLP_ChangeDeltaSupply dc_cc1(in, cc1);
  FLP_ChangeDeltaOpening dc_cc2(in, cc2);

  FLP_SwapDeltaSupply ds_cc1(in, cc1);
  // FLP_SwapDeltaOpening ds_cc2(in, cc2);

  FLP_ClopenDeltaSupply dk_cc1(in, cc1);
  FLP_ClopenDeltaOpening dk_cc2(in, cc2);

  // helpers
  FLP_SolutionManager sm(in);
  FLP_ChangeNeighborhoodExplorer cnhe(in, sm);
  FLP_SwapNeighborhoodExplorer snhe(in, sm, swap_bias);
  FLP_ClopenNeighborhoodExplorer knhe(in, sm, close_rate, open_rate);
  
  // All cost components must be added to the state manager
  sm.AddCostComponent(cc1);
  sm.AddCostComponent(cc2);
  
  // All delta cost components must be added to the neighborhood explorer
  cnhe.AddDeltaCostComponent(dc_cc1);
  cnhe.AddDeltaCostComponent(dc_cc2);

  snhe.AddDeltaCostComponent(ds_cc1);
  // snhe.AddDeltaCostComponent(ds_cc2);

  knhe.AddDeltaCostComponent(dk_cc1);
  knhe.AddDeltaCostComponent(dk_cc2);

 // neighborhood compositions
  SetUnionNeighborhoodExplorer<FLP_Input, FLP_Output, DefaultCostStructure<CostType>, FLP_ChangeNeighborhoodExplorer, FLP_SwapNeighborhoodExplorer> csnhe(in, sm, "Change/Swap",  cnhe, snhe, {1 - swap_rate, swap_rate});
  SetUnionNeighborhoodExplorer<FLP_Input, FLP_Output, DefaultCostStructure<CostType>, FLP_ChangeNeighborhoodExplorer, FLP_SwapNeighborhoodExplorer, FLP_ClopenNeighborhoodExplorer> csknhe(in, sm, "Change/Swap/Clopen",  cnhe, snhe, knhe, {1 - swap_rate - clopen_rate, swap_rate, clopen_rate});

  // runners
  HillClimbing<FLP_Input, FLP_Output, FLP_Change, DefaultCostStructure<CostType>> chc(in, sm, cnhe, "CHC");
  SteepestDescent<FLP_Input, FLP_Output, FLP_Change, DefaultCostStructure<CostType>> csd(in, sm, cnhe, "CSD");
  SteepestDescent<FLP_Input, FLP_Output, decltype(csnhe)::MoveType, DefaultCostStructure<CostType>> cssd(in, sm, csnhe, "CSSD");
  SimulatedAnnealing<FLP_Input, FLP_Output, FLP_Change, DefaultCostStructure<CostType>> csa(in, sm, cnhe, "CSA");
  TabuSearch<FLP_Input, FLP_Output, FLP_Change, DefaultCostStructure<CostType>> cts(in, sm, cnhe, "CTS", FLP_Change::Inverse);
  SimulatedAnnealing<FLP_Input, FLP_Output, decltype(csnhe)::MoveType, DefaultCostStructure<CostType>> cssa(in, sm, csnhe, "CSSA");
  SimulatedAnnealing<FLP_Input, FLP_Output, decltype(csknhe)::MoveType, DefaultCostStructure<CostType>> csksa(in, sm, csknhe, "CSKSA");
  TabuSearch<FLP_Input, FLP_Output, decltype(csknhe)::MoveType, DefaultCostStructure<CostType>> cskts(in, sm, csknhe, "CSKTS");
  SimulatedAnnealingTimeBased<FLP_Input, FLP_Output, decltype(csknhe)::MoveType, DefaultCostStructure<CostType>> csksa_tb(in, sm, csknhe, "CSKSAtb");

  // tester
  Tester<FLP_Input, FLP_Output, DefaultCostStructure<CostType>> tester(in, sm);
  MoveTester<FLP_Input, FLP_Output, FLP_Change, DefaultCostStructure<CostType>> change_move_test(in, sm, cnhe, "FLP_Change move", tester);
  MoveTester<FLP_Input, FLP_Output, FLP_Swap, DefaultCostStructure<CostType>> swap_move_test(in, sm, snhe, "FLP_Swap move", tester);
  MoveTester<FLP_Input, FLP_Output, FLP_Clopen, DefaultCostStructure<CostType>> k_move_test(in, sm, knhe, "FLP_Clopen move", tester);
  MoveTester<FLP_Input, FLP_Output, decltype(csnhe)::MoveType, DefaultCostStructure<CostType>> cs_move_test(in, sm, csnhe, "Change/Swap move", tester); 
  MoveTester<FLP_Input, FLP_Output, decltype(csknhe)::MoveType, DefaultCostStructure<CostType>> csk_move_test(in, sm, csknhe, "Change/Swap/Clopen move", tester); 

  SimpleLocalSearch<FLP_Input, FLP_Output, DefaultCostStructure<CostType>> solver(in, sm, "FLP solver");
  if (!CommandLineParameters::Parse(argc, argv, true, false))
    return 1;

  if (!method.IsSet())
    { // If no search method is set -> enter in the tester
      if (init_state.IsSet())
        tester.RunMainMenu(init_state);
      else
        tester.RunMainMenu();
    }
  else
    {
      if (method == string("CSA"))
        {
          solver.SetRunner(csa);
        }
      else if (method == string("CSSA"))
        {
          solver.SetRunner(cssa);
        }
      else if (method == string("CSKSAtb"))
        {
         solver.SetRunner(csksa_tb);
        }
       else if (method == string("CSKSA"))
        {
          solver.SetRunner(csksa);
        }
       else if (method == string("CHC"))
        {
          solver.SetRunner(chc);
        }
      else if (method == string("CSD"))
        {
          solver.SetRunner(csd);
        }
      else
        {
          cerr << "Unknown method " << static_cast<string>(method) << endl;
        }
      chrono::time_point<chrono::system_clock> start = chrono::system_clock::now(), end;
      if(init_state_strategy == "greedy")
	sm.GreedyState(init);
      else if(init_state_strategy == "random")
	sm.RandomState(init);
      else
	throw invalid_argument("Unknown initial state strategy");
	
      end = chrono::system_clock::now();
      double time1 = chrono::duration_cast<chrono::milliseconds>(end-start).count()/1000.0;

      if (method == string("CSKSAtb"))
        {
          if (timeout_mode == "linear")
            csksa_tb.SetParameter("allowed_running_time", in.Warehouses() - time1);
          else
            csksa_tb.SetParameter("allowed_running_time", timeout_factor*sqrt(in.Warehouses()) - time1);
        }

      auto result = solver.Resolve(init);
      // result is a tuple: 0: solution, 1: number of violations, 2: total cost, 3: computing time
    
      FLP_Output out = result.output;
      if (output_file.IsSet())
        { // write the output on the file passed in the command line
          ofstream os(static_cast<string>(output_file).c_str());
          out.PrettyPrint(os);
          os << endl;
          os << "Cost: " << result.cost.total << endl;
          os << "Time: " << result.running_time + time1 << "s"; 
          os.close();
        }
      else
        { 
          cout << "{" << setprecision(10)
               << "\"cost\": " <<  result.cost.total <<  ", "
               << "\"supply\": " << cc1.ComputeCost(out) << ", "
               << "\"opening\": " << cc2.ComputeCost(out) << ", "
               << "\"init_cost\": " <<  init.ComputeCost() <<  ", "
               << "\"init_supply\": " << cc1.ComputeCost(init) << ", "
               << "\"init_opening\": " << cc2.ComputeCost(init) << ", "
               << "\"init_time\": " << time1 << ", "
               << "\"time\": " << result.running_time << ", "            
               << "\"consistent\": \"" << (sm.CheckConsistency(out) ? "yes" : "no") << "\"" << ", "
               << "\"ss_ratio\": " << static_cast<double>(out.NumberOfSigleSourceStores())/in.Stores() << ", "
               << "\"open_ratio\": " << static_cast<double>(out.NumberOfOpenWarehouses())/in.Warehouses() << ", ";
            if (method == string("CSKSAtb"))
              cout << "\"iterations\": " << csksa_tb.Evaluations() <<  ", ";
            cout << "\"seed\": " << Random::GetSeed() << "} " << endl;
        }
   }
  return 0;
}
