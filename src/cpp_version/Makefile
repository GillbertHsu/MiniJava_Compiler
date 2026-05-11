CXX      = g++
CXXFLAGS = -std=c++17 -Wall -g
SRC      = main.cpp lexer.cpp parser.cpp symtab.cpp typecheck.cpp ir.cpp regalloc.cpp codegen.cpp
OBJ      = $(SRC:.cpp=.o)
TARGET   = mjc

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET) *.s

.PHONY: clean
