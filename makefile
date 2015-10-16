CXX				:= gcc
AR				:= ar
MV           	:= mv -f
RM           	:= rm -f
SED          	:= sed

INCLUDE_DIR		:= .
SOURCE_DIR		:= .
INCLUDES		:= -I$(INCLUDE_DIR)
tester			:= rmc_test
librmc			:= librmc.a

CXXFLAGS		:= -Wall -Wno-switch -g3 $(INCLUDES) -lm
ARFLAGS			:= -cvq

sources 		= $(SOURCE_DIR)/nmea0183_parser.c
test_sources	= $(SOURCE_DIR)/nmea0183_tester.c

objects      	:= $(subst .c,.o, $(sources))
dependencies 	:= $(subst .c,.d, $(sources))
test_objects      	:= $(subst .c,.o, $(test_sources))
test_dependencies 	:= $(subst .c,.d, $(test_sources))

.PHONY: clean

all: $(tester) $(librmc)

lib: $(librmc)

$(tester): $(objects) $(dependencies) $(test_objects) $(test_dependencies)
	$(CXX) $(objects) $(test_objects) -o $@ $(CXXFLAGS)

$(librmc): $(objects) $(dependencies)
	$(AR)  $(ARFLAGS) $@ $(objects) 

clean:
	rm -f $(tester) $(test_objects) $(test_dependencies)
	rm -f $(librmc) $(objects) $(dependencies)

-include $(objects:.o=.d) $(test_objects:.o=.d)

%.o: %.c
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.d: %.c
	$(CXX) $(CXXFLAGS) $(TARGET_ARCH) -M $< |      \
	$(SED) 's,\($*\.o\) *:,\1 $@: ,' > $@.tmp
	$(MV) $@.tmp $@
