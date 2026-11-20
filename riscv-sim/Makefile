CXX      ?= c++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -pedantic
SRC      := $(wildcard src/*.cpp)
OBJ      := $(SRC:src/%.cpp=build/%.o)

build/rvsim: $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

build/%.o: src/%.cpp $(wildcard src/*.hpp) | build
	$(CXX) $(CXXFLAGS) -c -o $@ $<

build:
	mkdir -p build

# Regenera los .bin de ejemplo con el mini-ensamblador (solo para pruebas).
examples:
	for f in examples/*.s; do python3 tools/rvasm.py $$f -o $${f%.s}.bin; done

test: build/rvsim
	./tests/run_tests.sh

clean:
	rm -f build/*.o build/rvsim

.PHONY: examples test clean
