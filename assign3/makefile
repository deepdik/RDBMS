CC = gcc
CFLAGS  =  -Wall

ifeq ($(OS), Windows_NT)
	RM = del
else
	RM = rm -f
endif


test_assign3: test_assign3_1.c storage_mgr.c dberror.c expr.c rm_serializer.c record_mgr.c buffer_mgr_stat.c buffer_mgr.c
	$(CC) $(CFLAGS) -o $@ $^

test_expr: test_expr.c storage_mgr.c dberror.c expr.c rm_serializer.c record_mgr.c buffer_mgr_stat.c buffer_mgr.c
	$(CC) $(CFLAGS) -o $@ $^

clean: 
	$(RM) test_assign3 test_expr

run_test_assign3:
ifeq ($(OS),Windows_NT)
	.\test_assign3
else
	./test_assign3
endif

run_test_expr:
ifeq ($(OS),Windows_NT)
	.\test_expr
else
	./test_expr
endif