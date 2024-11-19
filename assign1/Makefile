CC = gcc
CFLAGS  =  -Wall

ifeq ($(OS), Windows_NT)
	RM = del
else
	RM = rm -f
endif

test_assign1: test_assign1_1.c storage_mgr.c dberror.c 
	$(CC) $(CFLAGS) -o $@ $^

clean: 
	$(RM) test_assign1

run_storage_manager:
ifeq ($(OS),Windows_NT)
	.\test_assign1
else
	./test_assign1
endif
