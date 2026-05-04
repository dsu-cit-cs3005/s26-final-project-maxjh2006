CXX = g++
CXXFLAGS = -std=c++20 -Wall

TARGET = RobotWarz

$(TARGET): main.cpp Arena.cpp RobotBase.o
	$(CXX) $(CXXFLAGS) main.cpp Arena.cpp RobotBase.o -o $(TARGET)

clean:
	rm -f $(TARGET)