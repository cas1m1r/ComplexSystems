CC ?= cc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -pthread
PROGRAMS := bft_fixed bft_flawed cas_claim counter_atomic counter_atomic_split counter_mutex counter_private_mutex counter_racy false_sharing first-reply inventory_cas inventory_mutex inventory_naive poll_ping publication seats snapshot spin_counter split_vote thread_relay ticket_compare

all: $(addprefix build/,$(PROGRAMS)) build/big_atomic_energy.o

build:
	mkdir -p build

build/%: examples/%.c | build
	$(CC) $(CFLAGS) $< -o $@

build/big_atomic_energy.o: examples/big_atomic_energy.c | build
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: all
