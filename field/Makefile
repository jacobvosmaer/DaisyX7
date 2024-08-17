# Project Name
TARGET = DaisyX7

# Sources
CPP_SOURCES = DaisyX7.cpp
C_SOURCES = ../engine.c

# Library Locations
LIBDAISY_DIR = ../libDaisy

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile

format: *.cpp
	clang-format -i *.cpp *.h *.c
