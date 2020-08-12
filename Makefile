# the poly3 Makefile adapted over and over...

libs=\
	  sqlite3\
	  fuse3

CFLAGS:=$(shell pkg-config --cflags $(libs)) -DFUSE_USE_VERSION=31 $(CFLAGS)
LDLIBS:=$(shell pkg-config --libs $(libs)) $(LDLIBS)

.PHONY : clean distclean install uninstall

prefix?=/usr/local

bins=\
		stfs\
		tag\

all_objects=\
		cstrtok.o\
		db.o\
		error.o\
		getattr.o\
		mkdir.o\
		part.o\
		readdir.o\
		readlink.o\
		rename.o\
		rmdir.o\
		select-inodes.o\
		select-tags.o\
		stfs.o\
		string.o\
		tag.o\
		unlink.o\

.SECONDEXPANSION:

stfs_objects=\
		cstrtok.o\
		db.o\
		error.o\
		getattr.o\
		mkdir.o\
		part.o\
		readdir.o\
		readlink.o\
		rename.o\
		rmdir.o\
		select-inodes.o\
		select-tags.o\
		stfs.o\
		string.o\
		unlink.o\

tag_objects=\
		db.o\
		error.o\
		tag.o\

deps:=$(addprefix .d/,$(all_objects:.o=.d))

list=$(eval $(1):=$(addprefix .o/,$($(1))))

$(call list,all_objects)
$(call list,stfs_objects)
$(call list,tag_objects)

cc:=$(CC)

ifdef echo
header=
else
header=@echo $(1)
CC:=@$(CC)
endif

all : $(bins)

# not sure why it restarts, but don't print this twice
ifndef MAKE_RESTARTS
$(info $(shell $(cc) -v))
$(info CC       : $(cc))
$(info CPPFLAGS : $(CPPFLAGS))
$(info CFLAGS   : $(CFLAGS))
$(info LDFLAGS  : $(LDFLAGS))
$(info LDLIBS   : $(LDLIBS))
$(info )
endif

$(bins) : $$($$@_objects)
	$(call header,Linking $@...)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

include $(deps)

.d :
	$(call header,Checking dependencies...)
	@mkdir -p $@

.o :
	$(call header,Compiling...)
	@mkdir -p $@

.d/%.d : %.c | .d
	$(CC) $(CPPFLAGS) $(CFLAGS) -M -MF $@ $<
	@sed -i 's|$(<:.c=)\.o|\.o/&|' $@

.o/%.o : %.c | .o
	$(call header,-e "CC\t$<")
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

clean :
	$(RM) $(bins) $(all_objects)

distclean :
	$(RM) $(deps)

install : $(bins)
	install -m 755 -t $(prefix)/bin stfs tag
	install -m 755 -T mkstfs.sh $(prefix)/bin/mkstfs

uninstall :
	rm -f $(addprefix $(prefix)/bin/,stfs tag mkstfs)
