# Makefile for ITSP Simulator (Linux)
# Author: F. Baccari (adapted for Linux)

CC = gcc
CFLAGS = -Wall -O2 -Iinclude
LDFLAGS = -lpthread -lm
TARGET = itsp_simulator

# Source files
SOURCES = source/data_alert.c \
          source/data_config.c \
          source/data_file.c \
          source/data_link.c \
          source/data_payoffs.c \
          source/data_pools.c \
          source/data_race_status.c \
          source/data_results.c \
          source/data_scan.c \
          source/data_total.c \
          source/data_will_pay.c \
          source/i_exit.c \
          source/i_fast_map.c \
          source/i_fifo.c \
          source/i_file.c \
          source/i_sorted_set.c \
          source/i_sorted_vect.c \
          source/i_string.c \
          source/i_thread.c \
          source/i_tools.c \
          source/itsp_cnx.c \
          source/itsp_common.c \
          source/itsp_data.c \
          source/itsp_frame.c \
          source/itsp_frame_analyser.c \
          source/itsp_frame_maker.c \
          source/itsp_header.c \
          source/itsp_host.c \
          source/itsp_pilot.c \
          source/itsp_remote.c \
          source/itsp_scheduler.c \
          source/itsp_translator.c \
          source/main.c \
          source/memory_trace.c \
          source/s3k_commun.c \
          source/s3k_session.c

# Object files
OBJECTS = $(SOURCES:.c=.o)

# Build rules
all: $(TARGET)

$(TARGET): $(OBJECTS)
	@echo "Linking $@..."
	$(CC) $(OBJECTS) $(LDFLAGS) -o $(TARGET)
	@echo "Build complete: $(TARGET)"

%.o: %.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@echo "Cleaning build artifacts..."
	rm -f $(OBJECTS) $(TARGET)
	@echo "Clean complete"

rebuild: clean all

.PHONY: all clean rebuild
