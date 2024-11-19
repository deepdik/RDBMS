#ifndef TEST_HELPER_H
#define TEST_HELPER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Variable for holding the name of the current test.

extern char *TEST_NAME;

// Abbreviated form for test details.
#define TESTINFO  __FILE__, TEST_NAME, __LINE__, __TIME__

// Examine the return code and terminate if it indicates an error.
#define TESTCHECK(code)						\
		do {									\
			int RC_INTERNAL = (code);						\
			if (RC_INTERNAL != RC_OK)						\
			{									\
				char *OUTPUT = errorOUTPUT(RC_INTERNAL);			\
				printf("[%s-%s-L%i-%s] FAILED: Operation returned error: %s\n",TESTINFO, OUTPUT); \
				free(OUTPUT);							\
				exit(1);							\
			}									\
		} while(0);

//Verify if two strings are identical.
#define ASSERTEQUALSSTRING(EXPECTED,REAL,OUTPUT)			\
		do {									\
			if (strcmp((EXPECTED),(REAL)) != 0)					\
			{									\
				printf("[%s-%s-L%i-%s] FAILED: EXPECTED <%s> but was <%s>: %s\n",TESTINFO, EXPECTED, REAL, OUTPUT); \
				exit(1);							\
			}									\
			printf("[%s-%s-L%i-%s] OK: EXPECTED <%s> and was <%s>: %s\n",TESTINFO, EXPECTED, REAL, OUTPUT); \
		} while(0)


//Verify if two integers are equal.
#define ASSERTEQUALSINT(EXPECTED,REAL,OUTPUT)			\
		do {									\
			if ((EXPECTED) != (REAL))					\
			{									\
				printf("[%s-%s-L%i-%s] FAILED: EXPECTED <%i> but was <%i>: %s\n",TESTINFO, EXPECTED, REAL, OUTPUT); \
				exit(1);							\
			}									\
			printf("[%s-%s-L%i-%s] OK: EXPECTED <%i> and was <%i>: %s\n",TESTINFO, EXPECTED, REAL, OUTPUT); \
		} while(0)

// Verify if two integers are equal.
#define ASSETTRUE(REAL,OUTPUT)					\
		do {									\
			if (!(REAL))							\
			{									\
				printf("[%s-%s-L%i-%s] FAILED: EXPECTED true: %s\n",TESTINFO, OUTPUT); \
				exit(1);							\
			}									\
			printf("[%s-%s-L%i-%s] OK: EXPECTED true: %s\n",TESTINFO, OUTPUT); \
		} while(0)


// Confirm that a function yields an error code.
#define ASSERTERROR(EXPECTED,OUTPUT)		\
		do {									\
			int RESULT = (EXPECTED);						\
			if (RESULT == (RC_OK))						\
			{									\
				printf("[%s-%s-L%i-%s] FAILED: EXPECTED an error: %s\n",TESTINFO, OUTPUT); \
				exit(1);							\
			}									\
			printf("[%s-%s-L%i-%s] OK: EXPECTED an error and was RC <%i>: %s\n",TESTINFO,  RESULT , OUTPUT); \
		} while(0)

// test worked
#define TESTDONE()							\
		do {									\
			printf("[%s-%s-L%i-%s] OK: finished test\n\n",TESTINFO); \
		} while (0)

#endif // TEST_HELPER_H
