CXX := clang++
CXX_FLAGS := -g -O0 -std=c++20 -Wall

-include *.d
test_%: test_%.cpp
	$(CXX) -MMD $< $(CXX_FLAGS) -o $@
