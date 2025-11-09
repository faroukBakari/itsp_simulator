/*
 * xTools.h
 *
 *  Created on: 1 d�c. 2016
 *      Author: f.baccari
 */
#ifndef I_TOOLS_H_
#define I_TOOLS_H_

extern FILE* std_out;
extern FILE* std_err;

typedef void* p_void;
typedef	char* p_char;
typedef long cmp_func_type(const void*, const void*);

/* Defines*/
#define TRUE 1
#define FALSE 0
#define OK 1
#define FAULT 0
#define BIGFAULT -1
#define xNULL {0}

/*------------------MODE DEBUG------------------*/
extern int ACTIVE_DEBUG;
extern void DEBUG(FILE* stream, const char* format, ...);
#define TRACE(FILE, ...)	({fprintf(FILE, ##__VA_ARGS__); fflush(FILE);})
#define DEBUG_NOTIF(...)	({fprintf(std_out, ##__VA_ARGS__); fflush(std_err);})
#define DEBUG_WARN(FORMAT, ...)		({fprintf(std_err, FORMAT" (%s in %s at %d)\n", ##__VA_ARGS__, __func__, __FILE__, __LINE__); fflush(std_err);})
#define DEBUG_TOFILE(FILE, FORMAT, ...) ({\
	if(FILE){\
		fprintf(FILE, FORMAT" (%s in %s at %d)\n", ##__VA_ARGS__, __FILE__, __func__, __LINE__);\
		fflush(FILE);\
	}\
})
/*----------------------------------------------*/

/*********************************************************************************/
#define MAX(X,Y) (((X)<=(Y)) ? (Y) : (X))
#define MIN(X,Y) (((X)>=(Y)) ? (Y) : (X))
#define ABS(X) 	 ((X)>=0) 	 ? (X) : ({(0 - X);})

/*********************************************************************************/
#define GET_VALUE(X,TYP) (*((TYP*)((void*)(X))))
#define GET_MEMBER_VALUE(PTR,TYP,MEMBR,TYPMBR) GET_VALUE(((long int)(PTR) + ((long int)&(((TYP *)0)->MEMBR))),TYPMBR)

#ifndef offsetof
#define offsetof(TYPE,MEMBER) ((long int) ((long int)&(((TYPE*)0)->MEMBER)))
#endif

#define va_arg_addr(ap, size)({\
	va_arg (ap, char[size]);\
	(void*)ap;\
})

/*********************************************************************************/
#define lambda(return_type, function_body) ({ \
      return_type __fn__ function_body \
      __fn__;\
})

/*********************************************************************************/
#define NB_ARGS(...) \
		NB_ARGS_(__VA_ARGS__,NB_ARGS_RSEQ_N())
#define NB_ARGS_(...) \
		NB_ARGS_N(__VA_ARGS__)
#define NB_ARGS_N( \
          _1, _2, _3, _4, _5, _6, _7, _8, _9,_10, \
         _11,_12,_13,_14,_15,_16,_17,_18,_19,_20, \
         _21,_22,_23,_24,_25,_26,_27,_28,_29,_30, \
         _31,_32,_33,_34,_35,_36,_37,_38,_39,_40, \
         _41,_42,_43,_44,_45,_46,_47,_48,_49,_50, \
         _51,_52,_53,_54,_55,_56,_57,_58,_59,_60, \
         _61,_62,_63,N,...) N
#define NB_ARGS_RSEQ_N() \
         63,62,61,60,                   \
         59,58,57,56,55,54,53,52,51,50, \
         49,48,47,46,45,44,43,42,41,40, \
         39,38,37,36,35,34,33,32,31,30, \
         29,28,27,26,25,24,23,22,21,20, \
         19,18,17,16,15,14,13,12,11,10, \
         9,8,7,6,5,4,3,2,1,0

/*********************************************************************************/

extern long char_key_com(const void* k1, const void* k2);

extern long short_key_com(const void* k1, const void* k2);

extern long int_key_com(const void* k1, const void* k2);

extern long long_key_com(const void* k1, const void* k2);

extern long ullint_key_cmp(const void* k1, const void* k2);

extern long _12_key_com(const void* k1, const void* k2);

extern long _16_key_com(const void* k1, const void* k2);

extern long _24_key_com(const void* k1, const void* k2);

extern long _32_key_com(const void* k1, const void* k2);

extern long double_key_com(const void* k1, const void* k2);

extern long ptr_key_com(const void* k1, const void* k2);

extern long string_key_com(const void* k1, const void* k2);

/*********************************************************************************/

extern void* bsearch_next(void* element, void* first, void* last, long unsigned int obj_sz, cmp_func_type* cmp_func);

extern void* bsearch_custom(void* element, void* first, void* last, long unsigned int obj_sz, cmp_func_type* cmp_func);

extern void swap_(long unsigned int* a, long unsigned int*b);

extern void printtostring(char* buff, const char* format, ... );

#define swap(a,b) swap_((long unsigned int*)(&a), (long unsigned int*)(&b))

#define isequal_double(d1, d2, tol) __builtin_isgreater((d1 - d2) * (d1 - d2), tol * tol)

#endif
