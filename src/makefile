
CC = gcc
CX = g++
CP = cp
MV = mv
AR = ar
RM = -rm -rf
SP = strip
EC = @

EXEC = xlog.out

ODIR = obj
DIRS = ./
vpath %.c   $(DIRS)
vpath %.cpp $(DIRS)

SRCS = main.c xbuffer.c
LIBS = -lrt -lpthread

define match_rule
  $(addprefix $(ODIR)/, $(notdir $(patsubst %.c, $(1), $(filter %.c, $(SRCS))) $(patsubst %.cpp, $(1), $(filter %.cpp, $(SRCS)))))
endef

OBJS = $(call match_rule, %.o)
DEPS = $(call match_rule, %.d)
PRES = $(call match_rule, %.i)

INCDIR = $(addprefix -I, $(DIRS))
CFLAGS = -Wall -c -gdwarf-2 -fpic
XFLAGS = -std=gnu++11
LFLAGS = -L./lib -Wl,-rpath=${shell pwd}/lib -Wl,--as-needed

ifeq ($(host), arm)
    CC = arm-linux-gnueabihf-gcc
    CX = arm-linux-gnueabihf-g++
    AR = arm-linux-gnueabihf-ar
    SP = arm-linux-gnueabihf-strip
    ifeq (${shell file ./$(ODIR)/*.o | grep "ARM"    | wc -l}, 0)
        ${shell rm -rf ./$(ODIR)}
    endif
else
    ifeq (${shell file ./$(ODIR)/*.o | grep "x86-64" | wc -l}, 0)
        ${shell rm -rf ./$(ODIR)}
    endif
endif

ifeq ($(filter %.cpp, $(SRCS)),)
   LD = $(CC)
else
   LD = $(CX)
endif

ifeq ($(detail), on)
  EC =
  NU = >/dev/null
endif

ifeq ($(debug), on)
  CFLAGS += -g
else
  CFLAGS += -O2
endif

.PHONY: all clean distclean $(ODIR)

all : $(ODIR) $(EXEC)

$(EXEC): $(OBJS) $(PRES)
	$(EC)$(LD) $(LFLAGS) -Wl,--whole-archive -Wl,--start-group $(OBJS) $(LIBS) -Wl,--end-group -Wl,--no-whole-archive -o $@
ifeq ($(strip), on)
	$(EC)$(SP) $@
endif
	$(EC)$(CP) $@ $(ODIR)
	@echo LD "*.o" $(LIBS) "->" $@ ok.

$(ODIR)/%.i: %.cpp
	@$(CX) $(CFLAGS) $(XFLAGS)           -E $(INCDIR) -o $@ $<

$(ODIR)/%.o: %.cpp
	@echo CX $(notdir $<) $(NU)
	$(EC)$(CX) $(CFLAGS) $(XFLAGS) -MMD -MP $(INCDIR) -o $@ $<

$(ODIR)/%.i: %.c
	@$(CC) $(CFLAGS)                     -E $(INCDIR) -o $@ $<

$(ODIR)/%.o: %.c
	@echo CC $(notdir $<) $(NU)
	$(EC)$(CC) $(CFLAGS)           -MMD -MP $(INCDIR) -o $@ $<

-include $(DEPS)

$(ODIR):
	@test -d $@ || mkdir -p $@

clean:
	@$(RM)   $(ODIR) $(EXEC)
	@echo RM $(ODIR) $(EXEC)

distclean:
	@$(RM)   $(ODIR) $(EXEC)
	@echo RM $(ODIR) $(EXEC)

#SRCS = $(notdir $(foreach dir, $(DIRS), $(wildcard $(dir)*.cpp)))
#-Wl,-soname,$@
#-Wl,--whole-archive libx liby -Wl,--no-whole-archive
#-Wl,--start-group   libx liby -Wl,--end-group
