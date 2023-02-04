VERSION = 0.1.0

CC  = cc
LD  = cc

CPPFLAGS = -DVERSION=\"${VERSION}\" -D_POSIX_C_SOURCE=200809L
CFLAGS   = -I/usr/local/include -Wall -Wunused $(CPPFLAGS) -O2 -s

LDFLAGS = -L/usr/local/lib
LDLIBS  = -lmupdf -lmupdf-third -lgumbo -ljbig2dec -lharfbuzz -lfreetype -lopenjp2 -ljpeg \
          -ldjvulibre \
          -ltesseract \
          -lz -lm
