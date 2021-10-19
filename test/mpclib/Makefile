CXX := clang++
CXX_FLAGS := -O2 -std=c++17

test_%: test_%.cpp
	$(CXX) -o $@ $^ $(CXX_FLAGS)
