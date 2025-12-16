GXX_COMPILER=clang++
C_FLAGS=$(shell llvm-config --cxxflags) -Wall -Wpedantic
L_FLAGS=$(shell llvm-config --cxxflags --ldflags --system-libs --libs all)

SRC_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build2
EXAMPLE_DIR = rdlgExamples

SOURCES := $(shell find $(SRC_DIR) -name '*.cpp')

OBJECTS := $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SOURCES))

DEPS := $(OBJECTS:.o=.d)

all: randlang

randlang: $(OBJECTS)
	$(GXX_COMPILER) $^ $(L_FLAGS) -o $(BUILD_DIR)/randlang

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(@D)
	$(GXX_COMPILER) $(C_FLAGS) -I$(INCLUDE_DIR) -c $< -o $@

example: exampleCompile
	$(GXX_COMPILER) $(EXAMPLE_DIR)/test.cpp $(EXAMPLE_DIR)/output.o -o $(EXAMPLE_DIR)/exampleMain

exampleCompile: randlang
	$(BUILD_DIR)/randlang $(EXAMPLE_DIR)/code.rdlg $(EXAMPLE_DIR)/output.o

clean:
	rm -f $(BUILD_DIR)/*.o

cleanExample:
	rm -f $(EXAMPLE_DIR)/*.o

-include $(DEPS)