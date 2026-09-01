ifeq ($(OS),Windows_NT)
    CC = D:/mingw64/bin/gcc.exe
    RM = del /Q /F
    EXT = .exe
    WINFLAGS = -lgdi32 -luser32 -lgdiplus
else
    CC = gcc
    RM = rm -f
    EXT =
    WINFLAGS =
endif

CFLAGS = -std=c11 -Wall -Wextra -pedantic -O3 -Iinclude
LDFLAGS = -lm $(WINFLAGS)

SRC_DIR = src
INC_DIR = include
TEST_DIR = tests

TARGET = unified_terminal$(EXT)
TEST_TARGET = test_suite$(EXT)

all: $(TARGET) $(TEST_TARGET)

$(TARGET): $(SRC_DIR)/main.c $(INC_DIR)/*.h
	$(CC) $(CFLAGS) $(SRC_DIR)/main.c -o $(TARGET) $(LDFLAGS)

$(TEST_TARGET): $(TEST_DIR)/test_suite.c $(INC_DIR)/*.h
	$(CC) $(CFLAGS) $(TEST_DIR)/test_suite.c -o $(TEST_TARGET) $(LDFLAGS)

test: $(TEST_TARGET)
	./$(TEST_TARGET)

demo: $(TARGET)
	./$(TARGET) --demo

clean:
	$(RM) $(TARGET) $(TEST_TARGET) *.o 2>nul || true

.PHONY: all test demo clean
