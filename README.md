# MS-CFLP-CI
Repository for the Multi-Source Capacitated Facility Location Problem with Customer Incompatibilities.

This repository contains instances, best solutions, log files, and the source code of the paper *Multi-Neighborhood Simulated Annealing for the Capacitated Facility Location Problem with Customer Incompatibilities* by Sara Ceschia and Andrea Schaerf, submitted for publication.

------------------------------------------------------------------------

## Instances

The directory [`Instances`](Instances) contains the instances of dataset *CFLP-CI*.

The dataset *MESS-2020-1* is available in the [MESS-2020+1 repository](https://github.com/MESS-2020-1/).

All instances are written in the MiniZinc format.
Input and solution file formats are those of the MESS-2020-1 competion, explained in the [specification file](https://github.com/MESS-2020-1/Validator/blob/master/MESS-CompetitionSpecs.pdf).


## Solutions

The directory [`Results`](Results) stores the best solutions for the *CFLP-CI* and *MESS-2020-1* datasets.
Log files of the experiments are available in `ods` format.

The solution validator can be found in the MESS-2020+1 repository at [solution validator](https://github.com/MESS-2020-1/Validator).

## Source code

The directory [`SA-Solver`](SA-Solver) stores the C++ source code of the Multi-Neighborhood Simulated Annealing.

### How to install the SA solver

Download the *EasyLocal++* framework

`> git clone git@bitbucket.org:satt/easylocal-3.git`

Set the path of the *EasyLocal++* installation (e.g. `../easylocal-3`)  to the directory where it has been downloaded

`EASYLOCAL = ../easylocal-3`

Compile the solver with:

`make`

The executable `wlp` is created in the same directory of the source code.


Run the solver with:

`./wlp --main::instance  <instance_file>  --main::method CSKSAtb --main::init_state_strategy greedy --CSKSAtb::neighbors_accepted_ratio 0.13 --CSKSAtb::cooling_rate 0.994 --CSKSAtb::start_temperature 16.42 --CSKSAtb::min_temperature 0.183 --main::swap_rate 0.79 --main::swap_bias 0.45 --main::clopen_rate 0.04 --main::open_irate 0.16 --main::close_irate 0.019 --input::diff_threshold 8 --input::sqrt_ratio_preferred 1.375 --main::timeout_mode  <timeout> --main::output_file <solution_file>  --main::seed  <seed>`

For example, the command line:

`./wlp --main::instance  ../Instances/CFLP-CI/cflp-ci-00.dzn  --main::method CSKSAtb --main::init_state_strategy greedy --CSKSAtb::neighbors_accepted_ratio 0.13 --CSKSAtb::cooling_rate 0.994 --CSKSAtb::start_temperature 16.42 --CSKSAtb::min_temperature 0.183 --main::swap_rate 0.79 --main::swap_bias 0.45 --main::clopen_rate 0.04 --main::open_irate 0.16 --main::close_irate 0.019 --input::diff_threshold 8 --input::sqrt_ratio_preferred 1.375 --main::timeout_mode  linear --main::output_file sol-cflp-ci-00.txt  --main::seed  0`

Runs the solver on instance `cflp-ci-00.dzn` stored in the directory `../Instances/CFLP-CI/` and delivers the solution in the file `sol-cflp-ci-00.txt`. The `timout_mode` can be `linear` or `sqrt`.


The main parameters are the following:

- `--main::instance <file_name>` sets the path of the instance file (mandatory argument)

- `--main::timeout_mode <string>` sets the timeout among linear and sqrt (default sqrt)

- `--main::output_file <file_name>` this option allows you to write the solution in the file_name, otherwise only the cost and running time are printed in the output stream in `json` format.

- `--main::seed <number>`  this option sets the value of the seed, otherwise it is pulled at random by the solver. The number must be an integer.
 
