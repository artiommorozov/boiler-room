BOOST_ROOT=/home/olimex/boost_1_68_0

BUILD=build
SOURCE=$(shell find src -name "*.cpp")
HEADERS=$(shell find src -name "*.h")

OBJ=$(foreach v,$(SOURCE),$(BUILD)/$(v:%.cpp=%.o))
OBJDIR=$(foreach v,$(OBJ),$(dir $(v)))

DBGFLAGS=-g -O0

all: heating

clean:
	rm -rf $(BUILD)
	                                                                         
heating: $(OBJDIR) $(OBJ)
	g++ $(DBGFLAGS) -L$(BOOST_ROOT)/stage/lib -o $@ $(OBJ) -lboost_filesystem -lboost_system -lrt

$(OBJDIR):
	mkdir -p $@

$(BUILD)/%.o: %.cpp $(HEADERS)
	g++ $(DBGFLAGS) -fpermissive -Wall -Isrc -I$(BOOST_ROOT) -std=c++0x -o $@ -c $<
