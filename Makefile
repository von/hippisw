###########################################################################
#
#	Makefile for hippisw
#
#	$Id: Makefile,v 1.8 1995/08/16 00:46:25 vwelch Exp $
#
###########################################################################

include make.hippisw.inc

CONFIGURED_FILE	=	./configured

###########################################################################

# For SGIs

SHELL		=	/bin/sh

###########################################################################


LINK		=	$(CC) $(ARCH_LD_FLAGS)

###########################################################################

CFLAGS =	-g $(PROTOTYPES) $(ARCH_CC_FLAGS)

# Sources for data structures
#
DSRCS	=	address_config.c address_map.c find.c hippiswd_conf.c \
		ip_addr.c logical_addr.c parse_file.c parse_token.c \
		path.c port_conf.c portlist.c read_config.c strqtok.c \
		switch.c switch_conf.c switch_map.c switchlist.c \
		version.c

INCS	=	address_config.h address_map.h basic_defines.h \
		client_request.h connections.h daemon_config.h find.h \
		handle_input.h handle_output.h ifield.h ip_addr.h logger.h \
		logical_addr.h parse_file.h parse_token.h password_config.h \
		path.h port.h portlist.h prompt.h read_config.h sw_output.h \
		sw_init.h switch.h switch_map.h switchlist.h telnet.h \
		time_string.h

# Binary sources
BSRCS	=	harp.c hippi_cmd.c hippi_config.c ifield.c mksw.c 

DAEMON_SRCS =	hippiswd.c \
		connections.c client_request.c handle_input.c handle_output.c \
		logger.c password_config.c prompt.c sw_init.c telnet.c \
		time_string.c

CLIENT_SRCS =	hippisw.c

MANPAGES =	config_hippi.5 config_hippi_passwd.5 hippi_utils.8 \
		hippisw.8 hippiswd.8

HIPPI_UTIL_LNS	=	harp.8 hippi_config.8 ifield.8 mksw.8

###########################################################################

SRCS	=	$(DSRCS) $(BSRCS) $(DAEMON_SRCS) $(CLIENT_SRCS) sw_output.c

BINS	=	hippi_config mksw ifield hippi_cmd harp hippiswd hippisw

###########################################################################

DOBJS	=	${DSRCS:.c=.o}

OBJS	=	${SRCS:.c=.o}

DAEMON_OBJS	=	${DAEMON_SRCS:.c=.o} $(DOBJS)

CLIENT_OBJS	=	${CLIENT_SRCS:.c=.o} $(DOBJS)

###########################################################################
#
#	Rules called by user directly
#

default:	
	@if [ ! -f $(CONFIGURED_FILE) ]; then \
		echo "Run './configure' first." ;\
	else \
		(echo "Making hippisw in `cat $(CONFIGURED_FILE)`." ;\
			cd `cat $(CONFIGURED_FILE)` ; \
			echo "Checking Makefile:" ;\
			make Makefile ;\
			echo "Checking source links:" ;\
			make srcs ; \
			echo "Checking dependancies:" ;\
			make depend ; \
			echo "Building:" ;\
			make internal ) ;\
	fi

clean:
	@ if [ -f $(CONFIGURED_FILE) ]; then \
		echo "Cleaning up `cat $(CONFIGURED_FILE)` directory" ;\
		cd `cat $(CONFIGURED_FILE)` ;\
		rm -f $(OBJS) ;\
	fi

devclean:
	rm -f *~ .#*

clobber:
	@ if [ -f $(CONFIGURED_FILE) ]; then \
		echo Removing `cat $(CONFIGURED_FILE)` directory ;\
		rm -rf `cat $(CONFIGURED_FILE)` ;\
		rm -f $(CONFIGURED_FILE) ;\
	fi

install:
	@if [ -f ./configured ]; then \
		echo "Installing `cat ./configured` binaries." ;\
		cd `cat ./configured` ;\
		make internal ;\
		make int_install ;\
	else \
		echo "Run ./configure and make first." ;\
	fi


###########################################################################
#
#	Rules called only by Makefile only after changing to
#	architecture directory
#
#	make.hippisw.inc defines the following:
#		ARCH_DIR	name of architecture directory
#		ARCH_CC_FLAGS	flags for compiling
#		ARCH_LD_FLAGS	flags for linking
#		STRIP		strip binary
#		MKDIR		mkdir binary
#		INSTALL		install binary (probably cp)
#

Makefile:       $(SRC_DIR)/Makefile
	@echo Copying $(SRC_DIR)/Makefile to $(ARCH_DIR)/Makefile
	@cp $(SRC_DIR)/Makefile ./Makefile

internal:	$(BINS)

hippiswd:		$(DAEMON_OBJS)
	$(LINK) -o $@ $(DAEMON_OBJS)

hippisw:		$(CLIENT_OBJS)
	$(LINK) -o $@ $(CLIENT_OBJS)

ifield hippi_config harp:	$$@.o	$(DOBJS)
	$(LINK) -o $@ $@.o $(DOBJS)

mksw hippi_cmd:		$$@.o $(DOBJS) sw_output.o
	$(LINK) -o $@ $@.o $(DOBJS) sw_output.o

objects:	$(OBJS)

depend:		$(SRCS) $(INCS) Makefile
	@echo
	@echo Updating Makefile dependancies
	@makedepend $(SRCS)
	@touch depend
	@echo Done.
	@echo

srcs:
	@(for file in $(SRCS) $(INCS) ; do \
		if [ ! -f $$file ]; then \
			ln -s $(SRC_DIR)/$$file . ; \
		fi ;\
	done ;\
	)

int_install:
	@if [ ! -d $(BIN_DIR) ]; then \
		echo "Making $(BIN_DIR)" ;\
		$(MKDIR) $(BIN_DIR) ;\
	fi
	@echo Installing binaries in $(BIN_DIR)
	@$(STRIP) $(BINS)
	@$(INSTALL) $(BINS) $(BIN_DIR)
	@if [ ! -d $(MAN_DIR) ]; then \
		@echo Creating $(MAN_DIR)... ;\
		install -d $(MAN_DIR) ;\
		echo Making $(MAN_DIR) ;\
		$(MKDIR) $(MAN_DIR) ;\
	fi
	@echo Installing man pages in $(MAN_DIR)
	@(cd $(SRC_DIR); $(INSTALL) $(MANPAGES) $(MAN_DIR))
	@(cd $(MAN_DIR) ;\
		for file in $(HIPPI_UTIL_LNS) ; do \
			if [ ! -f $$file ]; then \
				ln -s hippi_utils.8 $$file ;\
			fi ;\
		done )

###########################################################################
#
#	Stuff here really only for development.
#

int_clean:
	@echo "Cleaning ."
	@rm -f *~ *.bak *.o

tags:
	etags *.c

###########################################################################
