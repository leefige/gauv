CXX := clang++
CXX_FLAGS := -O2 -std=c++17

-include *.d
test_%: test_%.cpp
	$(CXX) -MMD $< $(CXX_FLAGS) -o $@
