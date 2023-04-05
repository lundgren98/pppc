SRC_DIR = src
HEAD_DIR = include
OBJ_DIR = build

SRC = $(wildcard $(SRC_DIR)/*.cpp)
OBJ = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRC))

TARGET = pppc

STANDARD = -std=c++17

WARNINGS = -Wall \
	   -Wextra \
	   -Wshadow \
	   -Wconversion \
	   -Wpedantic \
	   -Werror \
	   -Wmisleading-indentation

EXPERIMENTAL = -fno-rtti \
	       -fno-exceptions \
	       -Wenum-conversion \
	       -Wctor-dtor-privacy \
	       -Wmismatched-tags \
	       -Wzero-as-null-pointer-constant \
	       -Wold-style-cast \
	       -Weffc++

GNU_EXPERIMENTAL = -Wredundant-tags \
	    -Wuseless-cast \
	    -fanalyzer-checker=cppcheck

SANITIZE = -fsanitize=undefined \
	   -fsanitize=address \
	   -fsanitize=pointer-compare \
	   -fsanitize=pointer-subtract

CXXFLAGS = $(STANDARD) \
	   -g \
	   $(WARNINGS) \
	   $(EXPERIMENTAL) \
	   $(SANITIZE)

INCLUDES = -I "$(HEAD_DIR)"
LDFLAGS = -Llib $(SANITIZE)
LIBS = -lcpr

.PHONY: all gnu clang build install clean analyze format

all: format analyze clang gnu

clang:
	@echo "Using clang++"
	@make \
	CXX=clang++ \
	OBJ_DIR='$(OBJ_DIR)/clang' \
	--no-print-directory \
	build

gnu:
	@echo "Using g++"
	@make \
	CXX=g++ \
	CXXFLAGS='$(CXXFLAGS) $(GNU_EXPERIMENTAL)' \
	OBJ_DIR='$(OBJ_DIR)/gnu' \
	--no-print-directory \
	build

build: $(TARGET)

$(TARGET): $(OBJ)
	@$(CXX) $(LDFLAGS) $^ $(LIBS) -o $@
	@mkdir -p cache

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@echo "        $?"
	@mkdir -p $(OBJ_DIR)
	@$(CXX) $(INCLUDES) $(CXXFLAGS) -c $? -o $@

install:
	cp $(TARGET) /usr/local/bin/$(TARGET)
	chmod 755 /usr/local/bin/$(TARGET)

clean:
	@$(RM) -rv $(OBJ_DIR) cache

quiet_clean:
	@$(RM) -r $(OBJ_DIR) cache

format:
	@clang-format -i --style=file $(HEAD_DIR)/*.h $(SRC_DIR)/*.cpp

analyze:
	@echo "cppcheck"
	@mkdir -p .cppcheck_build
	@cppcheck -v --enable=all \
		--suppress=missingIncludeSystem \
		--cppcheck-build-dir=.cppcheck_build \
		--project=compile_commands.json

-include $(OBJ:.o=.d)
