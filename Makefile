FLAGS = 

CC = g++ -std=c++23 -I. -march=native -fno-math-errno $(FLAGS)

IMPL_DIR = impl
OBJ_DIR = obj
BIN_DIR = bin
DATA_DIR = data

FIB = fibsonicci.cpp
EVAL = eval.cpp

.PHONY: init
init:
	mkdir -p $(OBJ_DIR)
	mkdir -p $(BIN_DIR)
	mkdir -p $(DATA_DIR)

.PHONY: clean clean-bin clean-data clean-all
clean-all: clean clean-bin clean-data

clean: # clean objects
	rm -f $(OBJ_DIR)/*

clean-bin:
	rm -f $(BIN_DIR)/*

clean-data:
	rm -f $(DATA_DIR)/*



###############################################################################
## Fibonacci implementations
IMPL = naive \
	   linear \
	   matmul_simple \
	   matmul_fastexp \
	   matmul_strassen \
	   matmul_karatsuba \
	   matmul_dft \
	   matmul_fft \
	   field_ext

IMPL_OPT = $(IMPL:%=%.Og) $(IMPL:%=%.O3)

IMPL_GOAL = $(IMPL:%=%.O3.1)
IMPL_LONG = $(IMPL_OPT:%=%.5)

IMPL_LIMIT = $(IMPL_LONG) $(IMPL_GOAL)

.PHONY: $(IMPL_LIMIT:%=run-%) all-data all-data-long

all-data: $(IMPL_GOAL:%=$(DATA_DIR)/%.dat)

all-data-long: $(IMPL_LONG:%=$(DATA_DIR)/%.dat)

$(IMPL_LIMIT:%=run-%): run-%: $(BIN_DIR)/%
	./$^

$(IMPL_LIMIT:%=$(DATA_DIR)/%.dat): $(DATA_DIR)/%.dat: $(BIN_DIR)/%
	./$^ > $@


.PHONY: all all-obj

all: $(IMPL_LIMIT:%=$(BIN_DIR)/%.out)

all-obj: $(IMPL_OPT:%=$(OBJ_DIR)/%.o)


.SECONDEXPANSION:
$(IMPL_OPT:%=$(BIN_DIR)/one_%.out): $(BIN_DIR)/one_%.out: $(FIB) $(OBJ_DIR)/$$(word 1,$$(subst ., ,%)).$$(word 2,$$(subst ., ,%)).o
	$(CC) $^ -o $@ -$(word 2,$(subst ., ,$@))

.SECONDEXPANSION:
$(IMPL_LIMIT:%=$(BIN_DIR)/%.out): $(BIN_DIR)/%.out: $(EVAL) $(OBJ_DIR)/$$(word 1,$$(subst ., ,%)).$$(word 2,$$(subst ., ,%)).o
	$(CC) $^ -o $@ -$(word 2,$(subst ., ,$@)) -DLIMIT=$(patsubst %,%,$(word 3,$(subst ., ,$@))) -lpthread


.SECONDEXPANSION:
$(IMPL_OPT:%=$(OBJ_DIR)/%.o): $(OBJ_DIR)/%.o: $(IMPL_DIR)/$$(word 1,$$(subst ., ,%)).cpp
	$(CC) -c $^ -o $@ -$(word 2,$(subst ., ,$@))