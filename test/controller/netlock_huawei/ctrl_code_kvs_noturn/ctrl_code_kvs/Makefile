# ifeq ($(RTE_SDK),)
# $(error "Please define RTE_SDK environment variable")
# endif

# # Default target, can be overriden by command line or environment
# RTE_TARGET ?= x86_64-native-linuxapp-gcc

# include $(RTE_SDK)/mk/rte.vars.mk

# # binary name
# APP = client

# # all source are stored in SRCS-y
# SRCS-y = txn_queue.c client.c think_queue.c

# CFLAGS += -O3 # -g for gdb debugging
# EXTRA_CFLAGS= -I$(S)/include/
# #CFLAGS += $(WERROR_FLAGS)

# include $(RTE_SDK)/mk/rte.extapp.mk

########################################
# binary name
APP = netlock_ctrl

# all source are stored in SRCS-y
SRCS-y := client.c

PKGCONF ?= pkg-config

# Build using pkg-config variables if possible
ifneq ($(shell $(PKGCONF) --exists libdpdk && echo 0),0)
$(error "no installation of DPDK found")
endif

all: shared
.PHONY: shared static
shared: build/$(APP)-shared
	ln -sf $(APP)-shared build/$(APP)
static: build/$(APP)-static
	ln -sf $(APP)-static build/$(APP)

PC_FILE := $(shell $(PKGCONF) --path libdpdk 2>/dev/null)
CFLAGS += -O2 $(shell $(PKGCONF) --cflags libdpdk) 
LDFLAGS_SHARED = $(shell $(PKGCONF) --libs libdpdk)
LDFLAGS_STATIC = $(shell $(PKGCONF) --static --libs libdpdk)

ifeq ($(MAKECMDGOALS),static)
# check for broken pkg-config
ifeq ($(shell echo $(LDFLAGS_STATIC) | grep 'whole-archive.*l:lib.*no-whole-archive'),)
$(warning "pkg-config output list does not contain drivers between 'whole-archive'/'no-whole-archive' flags.")
$(error "Cannot generate statically-linked binaries with this version of pkg-config")
endif
endif

CFLAGS += -DALLOW_EXPERIMENTAL_API 

build/$(APP)-shared: $(SRCS-y) Makefile $(PC_FILE) | build
	$(CC) $(CFLAGS) $(SRCS-y) -o $@ $(LDFLAGS) $(LDFLAGS_SHARED) -lpthread -g -rdynamic

build/$(APP)-static: $(SRCS-y) Makefile $(PC_FILE) | build
	$(CC) $(CFLAGS) $(SRCS-y) -o $@ $(LDFLAGS) $(LDFLAGS_STATIC) -lpthread -g -rdynamic

build:
	@mkdir -p $@

.PHONY: clean
clean:
	rm -f build/$(APP) build/$(APP)-static build/$(APP)-shared
	test -d build && rmdir -p build || true