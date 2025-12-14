GXX_COMPILER=clang++
C_FLAGS=$(shell llvm-config --cxxflags)
L_FLAGS=$(shell llvm-config --cxxflags --ldflags --system-libs --libs all)

SRC_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build2

SOURCES := $(shell find $(SRC_DIR) -name '*.cpp')

OBJECTS := $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SOURCES))

DEPS := $(OBJECTS:.o=.d)

all: $(OBJECTS)
	$(GXX_COMPILER) $^ $(L_FLAGS) -o $(BUILD_DIR)/hw

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(@D)
	$(GXX_COMPILER) $(C_FLAGS) -I$(INCLUDE_DIR) -c $< -o $@

clean:
	rm -f $(BUILD_DIR)/*.o

-include $(DEPS)