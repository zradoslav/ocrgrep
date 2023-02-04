SRC = $(wildcard *.cpp)
OBJ = $(SRC:%.cpp=%.o)

include config.mk

%.o: %.cpp
	@echo -e [CXX] '\t' $@
	@$(CXX) -o $@ -c $< $(CXXFLAGS) 

isbn_extract: $(OBJ)
	@echo -e [LD] '\t' $@
	@$(LD) -o $@ $^ $(LDFLAGS) $(LDLIBS)

clean:
	@echo -e [RM] '\t' $(OBJ) isbn_extract
	@rm -f $(OBJ) isbn_extract

.PHONY: clean
