#
# $Id$
#

LIBNAME = apollo

LIB_PATH = $(LIB_DIR)\$(LIBNAME)$(LIBEXT)

#
# LIB rules
#

LIB_OBJS = \
   $(OBJ_DIR)\apollo$(OBJEXT) \
   $(OBJ_DIR)\apollo1$(OBJEXT) \

all: \
   $(LIB_PATH) \