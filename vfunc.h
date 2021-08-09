#ifndef __VFUNC_H__

// Overloading Macro on Number of Arguments (but not type though)
// http://stackoverflow.com/questions/11761703/overloading-macro-on-number-of-arguments
//
// __NARG__(...) gets number of arguments with __NARG__ (up to 63)
// N.B.: - Object-like Macro definitions take effect at the place they are defined.
//       - A function-like macro is only expanded if its name appears followed by a parenthesis after it.
//         It is expanded at invocation only, and not at the place it is defined.
#  define __NARG__(...)  __NARG_I_(__VA_ARGS__,__RSEQ_N())
#  define __NARG_I_(...) __ARG_N(__VA_ARGS__)
#  define __ARG_N( \
      _1, _2, _3, _4, _5, _6, _7, _8, _9,_10, \
     _11,_12,_13,_14,_15,_16,_17,_18,_19,_20, \
     _21,_22,_23,_24,_25,_26,_27,_28,_29,_30, \
     _31,_32,_33,_34,_35,_36,_37,_38,_39,_40, \
     _41,_42,_43,_44,_45,_46,_47,_48,_49,_50, \
     _51,_52,_53,_54,_55,_56,_57,_58,_59,_60, \
     _61,_62,_63,N,...) N
#  define __RSEQ_N() \
     63,62,61,60,                   \
     59,58,57,56,55,54,53,52,51,50, \
     49,48,47,46,45,44,43,42,41,40, \
     39,38,37,36,35,34,33,32,31,30, \
     29,28,27,26,25,24,23,22,21,20, \
     19,18,17,16,15,14,13,12,11,10, \
     9,8,7,6,5,4,3,2,1,0

// General definition for any function name
// N.B.: When a macro parameter is used with a leading ‘#’, the preprocessor replaces it with the literal text of the actual argument, converted to a string constant. Unlike normal parameter replacement, the argument is not macro-expanded first.
//       If one wants to stringify the result of expansion of a macro argument, he has to use two levels of macros.
//       If either of the tokens next to an ‘##’ is a parameter name, it is replaced by its actual argument before ‘##’ executes.
//       As with stringification, the actual argument is not macro-expanded first.
//       If one wants to concatenate the result of expansion of a macro arguments, she has to use two levels of macros.
#  define _VFUNC_(name, n) name##n
#  define _VFUNC(name, n) _VFUNC_(name, n)
#  define VFUNC(func, ...) _VFUNC(func, __NARG__(__VA_ARGS__)) (__VA_ARGS__)

#endif
