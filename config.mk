VERSION = 0

CC  = cc
CXX = c++
LD  = $(CXX)

CPPFLAGS = -DVERSION=\"${VERSION}\"
CFLAGS   = -I/usr/local/include -Wall -Wunused $(CPPFLAGS) -O2 -s
CXXFLAGS = $(CFLAGS) -fno-rtti -fno-exceptions

LDFLAGS = -L/usr/local/lib
LDLIBS  = -lmupdf -lmupdf-third -lgumbo -ljbig2dec -lharfbuzz -lfreetype -lopenjp2 -ljpeg \
          -ldjvulibre \
          -ltesseract \
          -lz
