###########################################################################
#
#	Makefile for hippisw
#
#	$Id: Makefile,v 1.1 1995/02/28 23:17:16 vwelch Exp $
#
###########################################################################

#	Configurable parameters

MAGIC_STRING	=	hippi

CONFIG_FILE	=	/afs/ncsa/packages/hippisw/config.hippi

BIN_DIR		=	/afs/ncsa/packages/hippisw/`uname`/bin
MAN_DIR		=	/afs/ncsa/packages/hippisw/man

#	End configurable parameters

###########################################################################

# SGI needs this
SHELL		=	/bin/sh

ARCH		=	`uname`

CC		=	gcc

# If using cc, you probably also must define NO_PROTOTYPES
#CC		=	cc
#PROTOTYPES	=	-DNO_PROTOTYPES


###########################################################################
#
#	Possibles debugging defines:
#	-DHIPPISW_MAKE_CORE	hippiswd should make core on segment
#				or bus violations instead of exiting
#				gracefully.
#
# 	The following need to be combine with -DDEBUG.
#	-DDEBUG_CONF		Debug configuration reading routines.
#	-DDEBUG_PORTLIST	Debug portlist routines.
#	-DDEBUG_SWITCHLIST	Debug switchlist routines.
#	-DDEBUG_SWMAP		Debug switchmap building routines.
#

#DEBUG		=	-DHIPPISWD_MAKE_CORE

###########################################################################

CFLAGS =	-g $(PROTOTYPES) \
		-DMAGIC_STRING=\"$(MAGIC_STRING)\"  \
		-DCONFIG_FILE=\"$(CONFIG_FILE)\" \
		$(DEBUG)

# Sources for data structures
#
DSRCS	=	address_config.c address_map.c find.c hippiswd_conf.c \
		ip_addr.c logical_addr.c parse_file.c parse_token.c \
		path.c port_conf.c portlist.c read_config.c strqtok.c \
		switch.c switch_conf.c switch_map.c switchlist.c \
		version.c

INCS	=	address_config.h address_map.h basic_defines.h \
		client_request.h connections.h daemon_config.h find.h \
		handle_input.h ifield.h ip_addr.h logger.h logical_addr.h \
		parse_file.h parse_token.h password_config.h path.h port.h \
		portlist.h prompt.h read_config.h sw_output.h switch.h \
		switch_map.h switchlist.h telnet.h time_string.h

# Binary sources
BSRCS	=	harp.c hippi_cmd.c hippi_config.c ifield.c mksw.c 

DAEMON_SRCS =	hippiswd.c \
		connections.c client_request.c handle_input.c logger.c \
		password_config.c prompt.c telnet.c time_string.c

CLIENT_SRCS =	hippisw.c

MANPAGES =	config_hippi.5 config_hippi_passwd.5 hippi_utils.8 \
		hippisw.8 hippiswd.8

###########################################################################

SRCS	=	$(DSRCS) $(BSRCS) $(DAEMON_SRCS) $(CLIENT_SRCS) sw_output.c

BINS	=	hippi_config mksw ifield hippi_cmd harp hippiswd hippisw

###########################################################################

DOBJS	=	${DSRCS:.c=.o}

OBJS	=	${SRCS:.c=.o}

DAEMON_OBJS	=	${DAEMON_SRCS:.c=.o} $(DOBJS)

CLIENT_OBJS	=	${CLIENT_SRCS:.c=.o} $(DOBJS)

###########################################################################

default:	$(ARCH)
	@(cd $(ARCH) ; make internal )

$(ARCH):	$(SRCS) $(INCS)
	@if [ ! -d $(ARCH) ]; then \
		echo Making $(ARCH) directory ;\
		mkdir $(ARCH) ;\
	fi
	@if [ ! -f $(ARCH)/Makefile ]; then \
		echo Copying Makefile into $(ARCH) directory ;\
		cp Makefile $(ARCH) ;\
	fi	
	@(cd $(ARCH) ;\
		for file in $(SRCS) $(INCS) ; do \
			if [ ! -f $$file ]; then \
				ln -s ../$$file . ; \
			fi ;\
		done ;\
		echo "Making sure $(ARCH)/Makefile is up to date." ;\
		make Makefile ;\
		make depend ;\
	)

Makefile:       ../Makefile
	@echo Copying Makefile to $(ARCH)/Makefile
	@cp ../Makefile Makefile


internal:	$(BINS)

hippiswd:		$(DAEMON_OBJS)
	$(CC) -o $@ $(DAEMON_OBJS)

hippisw:		$(CLIENT_OBJS)
	$(CC) -o $@ $(CLIENT_OBJS)

ifield hippi_config harp:	$$@.o	$(DOBJS)
	$(CC) -o $@ $@.o $(DOBJS)

mksw hippi_cmd:		$$@.o $(DOBJS) sw_output.o
	$(CC) -o $@ $@.o $(DOBJS) sw_output.o

objects:	$(OBJS)

clean:
	@ if [ -d $(ARCH) ]; then \
		echo Cleaning up $(ARCH) directory ;\
		cd $(ARCH) ;\
		/bin/rm -f $(OBJS) *~ *.bak $(SRCS) $(INCS) Makefile depend ;\
	fi

clobber:
	@ if [ -d $(ARCH) ]; then \
		echo Removing $(ARCH) directory ;\
		/bin/rm -rf $(ARCH) ;\
	fi


depend:		$(SRCS) $(INCS) Makefile
	@echo
	@echo Updating Makefile dependancies
	@makedepend $(SRCS)
	@touch depend
	@echo Done.
	@echo

install:	default
	@if [ ! -d $(BIN_DIR) ]; then \
		echo Making $(BIN_DIR) ;\
		mkdir -p $(BIN_DIR) ;\
		chmod 755 $(BIN_DIR) ;\
	fi
	@echo Installing binaries in $(BIN_DIR)
	@(cd $(ARCH); cp $(BINS) $(BIN_DIR))
	@(cd $(BIN_DIR) ; chmod 755 $(BINS) ;)
	@if [ ! -d $(MAN_DIR) ]; then \
		echo Making $(MAN_DIR) ;\
		mkdir -p $(MAN_DIR) ;\
		chmod 755 $(MAN_DIR) ;\
	fi
	@echo Installing man pages in $(MAN_DIR)
	@cp $(MANPAGES) $(MAN_DIR)
	@(cd $(MAN_DIR) ; chmod 644 $(MANPAGES) ;)

tags:
	etags *.c
###########################################################################

