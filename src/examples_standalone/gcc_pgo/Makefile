include ../../../Makefile.defs

CXXFLAGS:=-I../../include -O2 -Wall -Werror

.PHONY: all
all: main_phase2.$(SUFFIX_BIN)

main.o: main.cc
	$(Q)g++ $(CXXFLAGS) -fprofile-generate -c -o main.o main.cc
main_phase1.$(SUFFIX_BIN): main.o
	$(Q)g++ -fprofile-generate -o $@ $<
main.gcda main_run.stamp: main_phase1.$(SUFFIX_BIN)
	$(Q)./main_phase1.$(SUFFIX_BIN) 1> /dev/null 2> /dev/null
	$(Q)touch main_run.stamp
main_phase2.o: main.cc main.gcda
	$(Q)cp main.gcda main_phase2.gcda
	$(Q)g++ $(CXXFLAGS) -fprofile-use -c -o main_phase2.o main.cc
	#$(Q)mv main.o main_phase2.o
main_phase2.$(SUFFIX_BIN): main_phase2.o
	$(Q)g++ -o $@ $<

.PHONY: clean
clean:
	$(Q)-rm -f main_phase2.$(SUFFIX_BIN) main_phase2.o main.gcda main_phase2.gcda main.o main.$(SUFFIX_BIN) main_phase1.$(SUFFIX_BIN) main_run.stamp
