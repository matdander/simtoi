##############################################################################
#
# Makefile
#
# This is a makefile for use with a 'c' version of the healpix toolkit. 
#
# Usage:
#
# To build the static library, type
#
# % make [static] [CC=<cc>] [OPT=<opt>] [AR=<ar>] [CFITSIO_INCDIR=<cfitsio_incdir>] [CFITSIO_LIBDIR=<cfitsio_libdir>]
#
# where
#  CC     is the C compiler you want to use (default: gcc)
#  OPT    compilation option
#  CFITSIO_INCDIR is where the header files for the 'cfitsio' library is kept
#  CFITSIO_LIBDIR is where the 'cfitsio' library archive is kept
#  AR is the command to create the archive with its index table
#      default = ar -rsv
#
# For example:
#  % make CFITSIO_INCDIR=/scr/kmg/include
#
# To build the shared(dynamical) library, type
#
# % make shared [CC=<cc>] [OPT=<opt>] [AR=<ar>] [CFITSIO_INCDIR=<cfitsio_incdir>] [CFITSIO_LIBDIR=<cfitsio_libdir>]
#
# For example:
#  % make shared CFITSIO_INCDIR=/scr/kmg/include
#
# To install the files, type
#
# % make install [INCDIR=<incdir>] [LIBDIR=<libdir>] [RANLIB=<ranlib>] 
#
# For example:
#  % make install LIBDIR=/home/kmg/lib INCDIR=/home/kmg/include RANLIB="ar -ts"
#
# where
#  INCDIR is the directory in which you want to install the header files
#  LIBDIR is the directory in which you want to install the library files
#  RANLIB is the 'ranlib' command to be used for the static library archive.
#         It defaults to 'ranlib', but if your
#         system does not have or need this, you should use something like
#         RANLIB="ar -ts" 
#
# You could also build the package by directly editing the Makefile
#  (not recommended unless you're going to do it a lot and don't think
#   you'll be keeping up with changes in the distribution).
# To build the library:
#  1) Edit the 'CC', 'LIBDIR' and 'INCDIR' lines in 'Makefile' to reflect your
#     system.
#  2) Type 'make'
#
# To install the library and header file:
#  1) Type 'make install'
#
##############################################################################
# You should not have to edit below this line ################################
#
#

AR = ar -rsv
CFITSIO_INCDIR = /opt/local/include/
CFITSIO_LIBDIR = /opt/local/lib/

PIC = 
#
SHLIB_LD =      $(CC) $(OPT) $(PIC) -shared
SHLIB_SUFFIX =  .so
#
DYLIB_LD =      $(CC) $(OPT) $(PIC) -dynamiclib
DYLIB_SUFFIX =  .dylib

# The sources in the package
MODS = ang2pix_nest ang2pix_ring \
	pix2ang_nest pix2ang_ring nest2ring ring2nest \
	mk_pix2xy mk_xy2pix nside2npix  npix2nside \
	ang2vec vec2ang pix2vec_nest pix2vec_ring vec2pix_nest vec2pix_ring \
	printerror get_fits_size write_healpix_map read_healpix_map

# static objects
OBJS = ${MODS:%=%.s.o}

# non-static objects
OBJD = ${MODS:%=%.d.o}

default: static
#
# Make the static library itself
static: libchealpix.a #tests

libchealpix.a : $(OBJS)
	$(AR) $@ $(OBJS)
#
#
# Make the shared library itself
shared: libchealpix$(SHLIB_SUFFIX) #tests

libchealpix$(SHLIB_SUFFIX) : $(OBJD)
	$(SHLIB_LD) -o $@ $(OBJD)

# Make the dynamic (Mac) library itself
dynamic: libchealpix$(DYLIB_SUFFIX) #tests

libchealpix$(DYLIB_SUFFIX) : $(OBJD)
	$(DYLIB_LD) -o $@ $(OBJD) -L$(CFITSIO_LIBDIR) -lcfitsio
#
# Install the static library (and the dynamic one)
install :  chealpix.h
	@if [ -f libchealpix.a ]; then \
		cp libchealpix.a $(LIBDIR)/. ; \
	fi; \
	if [ -f libchealpix$(SHLIB_SUFFIX) ]; then \
		cp libchealpix$(SHLIB_SUFFIX) $(LIBDIR)/. ; \
	fi; \
	if [ -f libchealpix$(DYLIB_SUFFIX) ]; then \
		cp libchealpix$(DYLIB_SUFFIX) $(LIBDIR)/. ; \
	fi; \
	cp chealpix.h $(INCDIR)/.

# Make the programs to test the package.

test_chealpix2 : test_chealpix2.c
	$(CC) $(OPT) -c -o test_chealpix2.o $<
	$(CC) $(OPT)    -o $@    test_chealpix2.o -L. -lchealpix -L$(CFITSIO_LIBDIR) $(WLRPATH) -lcfitsio -lm

test_cio : test_cio.c
	$(CC) $(OPT) -c -o test_cio.o $<          -I$(CFITSIO_INCDIR)
	$(CC) $(OPT)    -o $@         test_cio.o  -L. -lchealpix -L$(CFITSIO_LIBDIR) $(WLRPATH) -lcfitsio -lm

tests : test_chealpix2 test_cio
	./test_cio
	./test_chealpix2

#
# General compilation rules
# static objects
%.s.o : %.c
	$(CC) $(OPT)        -c -o $@ $< -I$(CFITSIO_INCDIR)

# non-static objects
%.d.o : %.c
	$(CC) $(OPT) $(PIC) -c -o $@ $< -I$(CFITSIO_INCDIR)

# Clean: remove intermediate files
clean :
	$(RM) *~ *.o core  \
		test_chealpix.dat test_chealpix.out test_output.fits test_map.fits libchealpix.*  \
		test_chealpix2 test_cio

# Tidy: remove library and include files
tidy : clean
	$(RM) $(LIBDIR)/libchealpix.* $(INCDIR)/chealpix.h

# Distclean: remove directories
distclean: tidy
	$(RM) -r $(LIBDIR) $(INCDIR)
