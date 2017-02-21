## Process this file with automake to produce Makefile.in

bin_PROGRAMS = mdview
INCLUDES = @X_CFLAGS@
mdview_LDADD = @X_LIBS@
mdview_SOURCES = mdview.h mdv_type.h \
	gtk.c gtk_convert.c gtk_info.c gtk_layer.c gtk_mark.c gtk_move.c \
	gtk_distance.c gtk_zoom.c gtkraw.c \
	command.c tremble.c object.c main.c \
	mdv.h mdv_stat.h mdv_stat.c \
	mdv_arg.c mdv_atom.h mdv_atom.c mdv_farg.h mdv_farg.c \
	mdv_data.h mdv_data.c mdv_draw.c mdv_form.h mdv_form.c \
	seekfile.h seekfile.c mdv_file.h mdv_file.c \
	mdv_text.h mdv_text.c mdv_util.h mdv_util.c \
	chunk.h chunk.c hash.h hash.c tavltree.h tavltree.c \
	str2int.h str2int.c stack.h stack.c machine.c machine.h
man_MANS = mdview.1
pkgdata_DATA = mdviewrc
EXTRA_DIST = pixmap/marked.xpm pixmap/pixmap_1.xpm pixmap/pixmap_14.xpm \
	pixmap/pixmap_1_1.xpm pixmap/pixmap_1_2.xpm pixmap/pixmap_1_3.xpm \
	pixmap/pixmap_2.xpm pixmap/pixmap_2_1.xpm pixmap/pixmap_2_2.xpm \
	pixmap/pixmap_2_3.xpm pixmap/pixmap_3.xpm pixmap/pixmap_3_1.xpm \
	pixmap/pixmap_3_2.xpm pixmap/pixmap_3_3.xpm pixmap/pixmap_4.xpm \
	pixmap/pixmap_44.xpm pixmap/pixmap_4_1.xpm pixmap/pixmap_4_2.xpm \
	pixmap/pixmap_4_3.xpm pixmap/pixmap_6.xpm pixmap/pixmap_61.xpm \
	pixmap/pixmap_66.xpm pixmap/pixmap_6_1.xpm pixmap/pixmap_6_2.xpm \
	pixmap/pixmap_6_3.xpm pixmap/pixmap_8.xpm pixmap/pixmap_8_1.xpm \
	pixmap/pixmap_8_2.xpm pixmap/pixmap_8_3.xpm pixmap/pixmap_help.xpm \
	pixmap/pixmap_minus.xpm pixmap/pixmap_plus.xpm \
	pixmap/pixmap_rotate.xpm pixmap/pixmap_stop.xpm \
	mdview.1 mdview.jp.1 mdviewrc mdview.spec
AM_CFLAGS = -DDEFARGFILE=\"${pkgdatadir}/mdviewrc\"
