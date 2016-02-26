CC	= gcc
MV	= mv
LD	= gcc
CFLAGS	= -O2 -g -Wall
YFLAGS	= -d
LFLAGS	= 
YFILES	= vrml.y modeldef.y fontdef.y
LFILES	= vrml.l fontdef.l modeldef.l
CFILES	= \
  body.c \
  bsptree.c \
  collision.c \
  matrix.c \
  mesh.c \
  model.c \
  plane.c \
  quaternion.c \
  ray.c \
  vector.c \
  list.c
OBJS	= $(CFILES:.c=.o)
LIBS	= -lm
LDFLAGS	= 
TARGET	= lib3d.a

all: $(TARGET)

$(TARGET): $(OBJS)
	ar cr $@ $(OBJS)

.c.o:
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(OBJS) $(TARGET)
