include ../Makefile.conf

S=Event.cc Entity.cc System.cc World.cc Components.cc
L=libentity.a
SL=libentity.so
O=$(S:.cc=.o)

TESTS=Entity_test.cc System_test.cc Event_test.cc Components_test.cc
TESTS_O=$(TESTS:.cc=.o)
TESTS_BIN=$(TESTS:.cc=)
ALL_TESTS_BIN=.all_tests

ALL_SOURCES=$(S) $(TESTS)
ALL_O=$(ALL_SOURCES:.cc=.o)

ALL_DEPS=$(patsubst %.cc,.%.cc.dep,$(ALL_SOURCES))

LDFLAGS+=-L. -lentity 
CXXFLAGS+=-I..

.PHONY: all clean distclean test cleantests

all: $(L) $(SL)

# Consolidated test binary
test: $(ALL_TESTS_BIN)
	@./$(ALL_TESTS_BIN)

$(ALL_TESTS_BIN): $(TESTS_O) $(L)
	$(CXX) $(LDFLAGS) -lgtest -lgtest_main $(TESTS_O) -o $@

# Individual test binaries
$(TESTS_BIN): $(TESTS_O) $(L)
	$(CXX) $(LDFLAGS) -lgtest -lgtest_main $@.o -o $@

$(L): $(O)
	ar rc $(L) $(O)

$(SL): $(O)
	$(CXX) $(LDFLAGS) -fPIC -shared -o $@ $(O)

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Command targets

clean: cleantests
	rm -f $(SL) $(O) $(L)

cleantests:
	rm -f $(TESTS_O) $(TESTS_BIN) $(ALL_TESTS_BIN)

distclean: clean
	rm -f $(ALL_DEPS)

-include $(ALL_DEPS)

.%.cc.dep: %.cc %.h
	@touch $@
	@makedepend -f$@ -I.. $^ > /dev/null 2>&1
	@rm -f $@.bak

.%.cc.dep: %.cc
	@touch $@
	@makedepend -f$@ -I.. $< > /dev/null 2>&1
	@rm -f $@.bak
