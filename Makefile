VERSION	= 0.92

PREFIX = /usr/local
BIN = $(PREFIX)/bin
DATA_DIR = $(PREFIX)/share/vulcan

FONT_DIR = $(DATA_DIR)/fonts
MODEL_DIR = $(DATA_DIR)/models
TEXTURE_DIR = $(DATA_DIR)/textures

CC	= gcc
MV	= mv
LD	= gcc
YACC	= bison -y
LEX	= flex
CFLAGS	= -O2 -g -Wall \
	-I/usr/X11R6/include -I/usr/local/include -Ilib3d \
	-DDATA_DIR=\"$(DATA_DIR)\" -DFONT_DIR=\"$(FONT_DIR)\" \
	-DMODEL_DIR=\"$(MODEL_DIR)\" -DTEXTURE_DIR=\"$(TEXTURE_DIR)\"
YFLAGS	= -d
LFLAGS	= 
YFILES	= vrml.y modeldef.y fontdef.y
LFILES	= vrml.l fontdef.l modeldef.l
CFILES	= \
  render.c \
  hash_table.c \
  dict.c \
  vrml_tree.c \
  cylinder.c \
  move.c \
  pick.c \
  panic.c \
  model_dump.c \
  engine.c \
  game.c \
  graphics.c \
  ui.c \
  ui_util.c \
  ui_main_menu.c \
  ui_in_game_menu.c \
  ui_input_move.c \
  ui_animation.c \
  ui_options_menu.c \
  main-x11.c \
  font_render.c \
  menu.c \
  gl_util.c \
  image.c \
  yyerror.c \
  $(YFILES:.y=_y_tab.c) \
  $(LFILES:.l=_lex_yy.c)
OBJS	= $(CFILES:.c=.o)
LIBS	= -lGL -lGLU -lm -lX11 -lpng -l3d
LDFLAGS	= -L/usr/local/lib -L/usr/X11R6/lib -Llib3d
TARGET	= vulcan
TARBALL	= vulcan-$(VERSION).tar.gz
DIRS	= lib3d

all: $(TARGET)

$(TARGET): $(OBJS) lib3d.o
	$(LD) $(LDFLAGS) $(OBJS) -o $@ $(LIBS)

vrml_y_tab.c vrml_y_tab.h: vrml.y
	$(YACC) $(YFLAGS) -b $(<:.y=) $<
	$(MV) $(<:.y=).tab.c $(<:.y=_y_tab.c)
	$(MV) $(<:.y=).tab.h $(<:.y=_y_tab.h)

vrml_lex_yy.o: vrml_lex_yy.c vrml_lex_yy_i.h vrml_y_tab.h

vrml_lex_yy_i.h: vrml.l vrml_y_tab.h
	$(LEX) $(LFLAGS) -o$@ $<

fontdef_y_tab.c fontdef_y_tab.h: fontdef.y
	$(YACC) $(YFLAGS) -b $(<:.y=) $<
	$(MV) $(<:.y=).tab.c $(<:.y=_y_tab.c)
	$(MV) $(<:.y=).tab.h $(<:.y=_y_tab.h)

fontdef_lex_yy.o: fontdef_lex_yy.c fontdef_lex_yy_i.h fontdef_y_tab.h

fontdef_lex_yy_i.h: fontdef.l fontdef_y_tab.h
	$(LEX) $(LFLAGS) -o$@ $<

modeldef_y_tab.c modeldef_y_tab.h: modeldef.y
	$(YACC) $(YFLAGS) -b $(<:.y=) $<
	$(MV) $(<:.y=).tab.c $(<:.y=_y_tab.c)
	$(MV) $(<:.y=).tab.h $(<:.y=_y_tab.h)

modeldef_lex_yy.o: modeldef_lex_yy.c modeldef_lex_yy_i.h modeldef_y_tab.h

modeldef_lex_yy_i.h: modeldef.l modeldef_y_tab.h
	$(LEX) $(LFLAGS) -o$@ $<

.c.o:
	$(CC) $(CFLAGS) -c $<

lib3d.o:
	$(MAKE) -C lib3d
	@echo "LIB3D" > lib3d.o

chessmodels: chessmodels.o model_dump.o hash_table.o lib3d.o
	$(LD) -Llib3d -o $@ chessmodels.o model_dump.o hash_table.o -l3d -lm

clean:
	rm -f *.o *~ core* *.stackdump chessmodels \
	$(YFILES:.y=_y_tab.[ch]) $(LFILES:.l=_lex_yy_i.h) \
	$(TARGET) $(TARBALL) MANIFEST
	@for i in $(DIRS); do \
		$(MAKE) -C $$i $@ || exit 1; \
	done

MANIFEST: clean
	@find . -type f -print | grep -v RCS | grep -v page | grep -v temp | sed s:^\.:vulcan-$(VERSION): > MANIFEST

tarball: MANIFEST
	(cd ..; ln -s vulcan-dev vulcan-$(VERSION))
	(cd ..; xargs tar -czvf vulcan-dev/$(TARBALL) < vulcan-dev/MANIFEST)
	(cd ..; rm vulcan-$(VERSION))

installdirs: mkinstalldirs
	./mkinstalldirs -m 755 $(PREFIX) $(BIN) $(PREFIX)/share \
	$(DATA_DIR) $(FONT_DIR) $(MODEL_DIR) $(TEXTURE_DIR)

install: $(TARGET) chessmodels installdirs
	install -s -m 755 $(TARGET) $(BIN)
	cp data/fonts/* $(FONT_DIR)
	chmod 644 $(FONT_DIR)/*
	./chessmodels > $(MODEL_DIR)/chess-models.modeldef
	chmod 644 $(MODEL_DIR)/*
	cp data/textures/* $(TEXTURE_DIR)
	chmod 644 $(TEXTURE_DIR)/*

uninstall:
	rm -f $(BIN)/$(TARGET)
	rm -fr $(DATA_DIR)
