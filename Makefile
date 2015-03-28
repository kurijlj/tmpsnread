##
# Make documentation: http://www.gnu.org/software/make/manual/
#



##
# Global variables section.
#
BUG_ADDRESS = kurijlj@gmail.com
TARGETS = tmpsnread



##
# Define global flags here.
#
STD_FLAGS = -O0 -Wall -std=gnu99
STD_DBG_FLAGS = -g -O0 -Wall -std=gnu99
GLIB2CFLAGS = $(shell pkg-config --cflags glib-2.0)
GSLCFLAGS = $(shell gsl-config --cflags)



##
# Define global libraries here.
#
LIBGLIB2 = $(shell pkg-config --libs glib-2.0)
LIBMATH = -lm
LIBGSL = $(shell gsl-config --libs)
LIBCURSES = -lncurses
LIBSENSORS = -lsensors



##
# Set default compiler if none set.
#
ifeq ($(CC), gcc)
else ifeq ($(CC), clang)
else
	CC = gcc
endif


##
# Print some help information.
#
.ONESHELL:
help:
	@echo -e "Usage: LANG=<LANGUAGE> RELEASE=<true> CC=<compiler> make [option]\n"
	@echo -e "Supported options:"
	@echo -e "\thelp\t\t\tPrint this help list"
	@echo -e "\tall\t\t\tMake all executables"
	@echo -e "\tprepmsgs\t\tPrepare .po files for translation"
	@echo -e "\ttranslations\t\tMake .mo translations"
	@echo -e "\tclean\t\t\tRemove .o and *~ files"
	@echo -e "\tcleanall\t\tRemove all executables, .o, *~, .ppc"
	@echo -e "\t\t\t\tand .pot files"
	@echo -e "\ttmpsnread\t\tMake tmpsnread executable"
	@echo -e "\ttmpsnread.po\t\tPrepare tmpsnread.po file for translation"
	@echo -e "\ttmpsnread.mo\t\tMake tmpsnread.mo translation\n\n"
	@echo -e "Supported compilers: gcc, clang.\n\n"
	@echo -e "Report bugs and suggestions to <$(BUG_ADDRESS)>\n"



##
# Make all executables.
#
all: $(TARGETS)



##
# Prepare all messages for translation.
#
prepmsgs: $(foreach TARGET,$(TARGETS),$(TARGET).po)



##
# Prepare all messages for translation.
#
translations: $(foreach TARGET,$(TARGETS),$(TARGET).mo)



##
# tmpsnread section.
#
tmpsnread : SOURCES = tmpsnread.c
tmpsnread : TARGET = $(SOURCES:%.c=%)
tmpsnread : OBJS = $(TARGET).o
tmpsnread : LIBS = $(LIBSENSORS) $(LIBGLIB2)
tmpsnread : EXTRA_FLAGS = $(GLIB2CFLAGS)
tmpsnread : tmpsnread.o
	$(CC) -o $(TARGET) $(OBJS) $(LIBS)

tmpsnread.o : tmpsnread.c
ifeq ($(RELEASE), true)
	$(CC) $(STD_FLAGS) $(EXTRA_FLAGS) -c $(SOURCES)
else
	$(CC) $(STD_DBG_FLAGS) $(EXTRA_FLAGS) -c $(SOURCES)
endif

# tmpsnread localisation section.
tmpsnread.mo : SOURCE = tmpsnread.po
tmpsnread.mo : TARGET = $(SOURCE:%.po=%).mo
.ONESHELL:
tmpsnread.mo : tmpsnread.po
ifeq ($(strip $(LANG)),)
	mkdir -p ./$(LANG)/LC_MESSAGES
	msgfmt -c -v -o $(TARGET) $(SOURCE)
	shell mv -v ./$(TARGET) ./$(LANG)/LC_MESSAGES/
else
	mkdir -p ./sr_RS@latin/LC_MESSAGES
	msgfmt -c -v -o $(TARGET) $(SOURCE)
	mv -v ./$(TARGET) ./sr_RS@latin/LC_MESSAGES/
endif

tmpsnread.po : SOURCE = tmpsnread.pot
tmpsnread.po : TARGET = $(SOURCE:%.pot=%).po
tmpsnread.po : tmpsnread.pot
ifeq ($(strip $(LANG)),)
	msginit -l $(LANG) -o $(TARGET) -i $(SOURCE)
else
	msginit -l sr_RS@latin -o $(TARGET) -i $(SOURCE)
endif

tmpsnread.pot : SOURCE = tmpsnread.ppc
tmpsnread.pot : DOMAIN = $(SOURCE:%.ppc=%)
tmpsnread.pot : VERSION = 1.0
tmpsnread.pot : TARGET = $(DOMAIN).pot
tmpsnread.pot : tmpsnread.ppc
	xgettext -o $(TARGET) --package-name="$(DOMAIN)" \
		--package-version="$(VERSION)" \
		--msgid-bugs-address="$(BUG_ADDRESS)" $(SOURCE)

tmpsnread.ppc : SOURCE = tmpsnread.c
tmpsnread.ppc : TARGET = $(SOURCE:%.c=%).ppc
tmpsnread.ppc : EXTRA_FLAGS = $(GLIB2CFLAGS)
tmpsnread.ppc : tmpsnread.c
	cpp $(EXTRA_FLAGS) $(SOURCE) $(TARGET)



##
# Cleanup section.
#
clean :
	rm -v ./*.o ./*~

cleanall :
	rm -v $(foreach TARGET,$(TARGETS),./$(TARGET)) ./*.o ./*~ ./*.ppc ./*.pot
