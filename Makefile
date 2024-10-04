#CC = clang -ferror-limit=0 -fmacro-backtrace-limit=0
CC = gcc -fwrapv # for signed interger overflow
GCC_COVERAGE = -fprofile-arcs -ftest-coverage
WARNINGS = -Wall -pedantic
#COMPILE = -pipe -O3 -fPIC
DEBUG = -g -DDEBUG
#PROFILE = -pg -fgnu89-inline
CFLAGS += $(DEBUG) $(GCC_COVERAGE) $(PROFILE) $(WARNINGS) $(COMPILE) $(PROC_OPT)

.PHONY: all
all: libtm.a run_tu

dates.o: dates.c dates.h

libtm.a: dates.o
	ar rcs "$@" $^

dates_tu_check: LDLIBS += -lpthread -lcheck -ltm -lm -lsubunit # -lrt
dates_tu_check: LDFLAGS += -L/usr/lib/llvm-6.0/lib -L. # -lprofile_rt
dates_tu_check: libtm.a

.PHONY: run_tu
run_tu: dates_tu_check
	CK_DEFAULT_TIMEOUT=10 CK_VERBOSITY=verbose ./dates_tu_check | tee dates_tu_check.result
#	@LD_LIBRARY_PATH=/usr/lib/llvm-3.2/lib:${LD_LIBRARY_PATH} CK_VERBOSITY=verbose valgrind --leak-check=full --track-origins=yes --show-reachable=yes  --error-limit=no --gen-suppressions=all --log-file=utest_valgrind.log "./$@" || rm "./$@"
	gcov dates.c
