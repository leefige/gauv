CXX := clang++
CXX_FLAGS := -O0 -std=c++20 -Wall

-include *.d
test_%: test_%.cpp
	$(CXX) -MMD -g $< $(CXX_FLAGS) -o $@
