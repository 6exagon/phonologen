CC = clang
CFLAGS = -O3 -march=native
LINKER = ld
LFLAGS = -lc

CSRC = $(wildcard phonologen/*.c)
OBJ = $(patsubst phonologen/%.c,out/%.o,$(CSRC))

phonologen: build $(OBJ)
	$(LINKER) $(LFLAGS) -o build/phonologen $(filter-out $<,$^)

build:
	mkdir build

out/%.o: phonologen/%.c out
	$(CC) $(CFLAGS) -c -o $@ $<

out:
	mkdir out

clean:
	rm -rf build out