SRC := bytecode.c file_io.c main.c native.c parse.c
OBJ := $(SRC:%.c=.obj.%.o)

IFLAGS := -I../include/

all: ../ekanz

../ekanz: $(OBJ)
	gcc -o ../ekanz $(OBJ)

$(OBJ): .obj.%.o:%.c
	gcc $(IFLAGS) -g -c $< -o $@