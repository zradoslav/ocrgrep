SRC = $(wildcard *.c)
OBJ = $(SRC:%.c=%.o)

include config.mk

%.o: %.c
	@echo [CC] $@
	@$(CC) -o $@ -c $< $(CFLAGS)

ocrgrep: $(OBJ)
	@echo [LD] $@
	@$(LD) -o $@ $^ $(LDFLAGS) $(LDLIBS)

clean:
	@echo [RM] $(OBJ) ocrgrep
	@rm -f $(OBJ) ocrgrep

.PHONY: clean
