EASYLOCAL = ../easylocal-3
FLAGS = -std=c++17 -Wall -O3
LINKOPTS = -lboost_program_options -pthread
COMPOPTS = -I$(EASYLOCAL)/include $(FLAGS)
OBJECT_FILES = FLP_Input.o FLP_Output.o FLP_Helpers.o FLP_Main.o

flp: $(OBJECT_FILES)
	g++ $(OBJECT_FILES) $(LINKOPTS) -o flp

FLP_Input.o: FLP_Input.cc FLP_Input.hh
	g++ -c $(FLAGS) FLP_Input.cc

FLP_Output.o: FLP_Output.cc FLP_Input.hh FLP_Output.hh
	g++ -c $(FLAGS) FLP_Output.cc

FLP_Helpers.o: FLP_Helpers.cc FLP_Helpers.hh FLP_Input.hh FLP_Output.hh
	g++ -c $(COMPOPTS) FLP_Helpers.cc

FLP_Main.o: FLP_Main.cc FLP_Helpers.hh FLP_Input.hh FLP_Output.hh
	g++ -c $(COMPOPTS) FLP_Main.cc

clean:
	rm -f $(OBJECT_FILES) flp

