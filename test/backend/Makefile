CXX := clang++
CXX_FLAGS := -O2 -std=c++20 -Wall

-include *.d

test_%: test_%.cpp ../../src/backend/*.cpp
	$(CXX) $(CXX_FLAGS) $^
