# Compiler settings
CXX = g++
CXXFLAGS = -std=c++20 -Wall

# The final executable name
TARGET = RobotWarz

# Build the main executable
$(TARGET): main.cpp Arena.cpp RobotBase.o
	$(CXX) $(CXXFLAGS) main.cpp Arena.cpp RobotBase.o -o $(TARGET)

# Clean up compiled files
clean:
	rm -f $(TARGET)