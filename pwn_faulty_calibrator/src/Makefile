CXX = g++

CXXFLAGS = -m64 -g -Wall -Wextra \
           -fstack-protector-all \
           -fno-pie

LDFLAGS = -m64 \
          -no-pie \
          -Wl,-z,relro,-z,now \
          -Wl,-z,noexecstack

TARGET_DIR = build
SRC = faulty.cpp
OUT = faulty
TARGET = $(TARGET_DIR)/$(OUT)

all: $(TARGET)

$(TARGET_DIR):
	mkdir -p $@

$(TARGET): $(TARGET_DIR) $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run