SRC := bytecode.c file_io.c main.c native.c parse.c vm.c obj.c
OBJ := $(SRC:%.c=.obj.%.o)
SRCX := astjit.cpp
OBJX := $(SRCX:%.cpp=.obj.%.o)

IFLAGS := -I../include/
CFLAGS := -O2 -g 
LDFLAGS := -lrt -flto

all: ../ekanz

../ekanz: $(OBJ) $(OBJX)
	g++ -o ../ekanz $(OBJ) $(OBJX) $(LDFLAGS) 

$(OBJ): .obj.%.o:%.c
	gcc $(IFLAGS) $(CFLAGS) -c $< -o $@
$(OBJX): .obj.%.o:%.cpp
	g++ $(IFLAGS) $(CFLAGS) -c $< -o $@ -std=c++11 
clean:
	rm .obj.*.o
