#
# $Id$
#

LIBNAME = hb_btree

LIB_PATH = $(LIB_DIR)\$(LIBNAME)$(LIBEXT)

#
# LIB rules
#

LIB_OBJS = \
   $(OBJ_DIR)\hb_btree$(OBJEXT) \
   $(OBJ_DIR)\tbtree$(OBJEXT) \

all: \
   $(LIB_PATH) \