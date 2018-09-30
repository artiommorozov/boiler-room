BUILD=build
SOURCE=$(shell find src -name "*.cpp")
HEADERS=$(shell find src -name "*.h")

OBJ=$(foreach v,$(SOURCE),$(BUILD)/$(v:%.cpp=%.o))
OBJDIR=$(foreach v,$(OBJ),$(dir $(v)))

all: heating

clean:
	rm -rf $(BUILD)

heating: $(OBJDIR) $(OBJ)
	g++ -o $@ $(OBJ)

$(OBJDIR):
	mkdir -p $@

$(BUILD)/%.o: %.cpp $(HEADERS)
	g++ -fpermissive -Wall -Isrc -std=c++0x -o $@ -c $<
