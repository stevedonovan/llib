/*
* llib little C library
* BSD licence
* Copyright Steve Donovan, 2013
*/

/***
### Useful Array Macros.

These operate on llib arrays, supporting mapping, filtering and inline iteration.
With all these macros, the dummy variable for accessing the current array
value is `_` (underscore).  The map & filter macros all declare an array of
the given type, and create a reference-counted array if the name ends in 'R'.

See `test-array.c`.
*/

#ifndef _LLIB_ARRAY_H
#define _LLIB_ARRAY_H

#include "obj.h"

#ifdef __cplusplus
// Some C++11 magic needed to create a portable typeof...
// http://stackoverflow.com/questions/13202289/remove-reference-in-decltype-return-t-instead-of-t-where-t-is-the-decltype
#include <type_traits>
#define _T_(e) std::remove_reference<decltype(e)>::type
#else
// C99 extension supported by GCC, Clang, Intel
#define _T_ __typeof__
#endif

/// Iterate over an array.
// @param arr the array (can be an expression)
// @param expr the expression containing `_`
// @macro FORA
// @usage `FORA`(ints,printf("%i ",_));
#define FORA(A_,E_) {_T_(A_) a_ = A_; FOR(i,array_len(a_)) \
  { _T_(*a_) _ = a_[i]; E_; }}

/// Declare an array which is `[F(x)|x in A]`
// @param T the resulting type
// @param v the new array name
// @param F the expression containing `_`
// @param A the source array
// @macro MAPA
// @usage
//    char** ss = str_strings("one","two","three",NULL);
//    MAPA(int,ssl,strlen(_),ss);
//    assert(ssl[0] == 3);
//    assert(ssl[1] == 3);
//    assert(ssl[2] == 5);
#define MAPA(T_,v_,E_,A_) T_ *v_; {_T_(A_) a_ = A_; v_ = array_new(T_,array_len(a_)); \
 FOR(i_,array_len(a_)) { _T_(*a_) _ = a_[i_]; v_[i_] = E_; } }

/// Declare a reference array which is `[F(x)|x in A]`
// @param T the resulting type
// @param v the new array name
// @param expr the expression containing `_`
// @param arr the source array
// @macro MAPAR
// @usage `MAPAR`(char*,strs,str_fmt("%d",_),ints);
#define MAPAR(T_,v_,E_,A_) T_ *v_; {_T_(A_) a_ = A_; v_ = array_new_ref(T_,array_len(a_)); \
 FOR(i_,array_len(a_)) { _T_(*a_) _ = a_[i_]; v_[i_] = E_; } }

/// New array containing matching elements from `A`.
// @param T array type
// @param v the new array name
// @param A the input array
// @param expr the filter expression in `_`
// @macro FILTA
// @usage `FILTA`(int,non_zero,ints,_ > 0);
#define FILTA(T_,v_,A_,E_) T_ *v_; {_T_(A_) a_ = A_; T_**s_ = seq_new(T_); \
 FOR(i_,array_len(a_)) { _T_(*a_) _ = a_[i_]; if (E_) seq_add(s_,_); } \
 v_ = (T_*)seq_array_ref(s_); }

/// New reference array containing matching elements from `A`.
// @param T array type
// @param v the new array name
// @param A the input array
// @param expr the filter expression in `_`
// @macro FILTAR
// @usage `FILTAR`(char*,matches,lines,`str_starts_with`(_,"log:"));
 #define FILTAR(T_,v_,A_,E_) T_ *v_; {_T_(A_) a_ = A_; T_**s_ = seq_new_ref(T_); \
 FOR(i_,array_len(a_)) { _T_(*a_) _ = a_[i_]; if (E_) seq_add(s_,obj_ref(_)); } \
 v_ = (T_*)seq_array_ref(s_); }
 
 /// Index into array where expression matches value.
 // Declares a variable `v`, which will be -1 if no match is possible.
 // @param v new index
 // @param A the array
 // @param istart where to start searching (usually 0)
 // @param expr expression in `_`
 // @macro FINDA
 // @usage 
 //   char** exts = `str_strings`(".o",".d",".obj",NULL);
 //   str_t file = "bonzo.d";
 //   `FINDA`(idx,exts,0,`str_ends_with`(file,_));
 //   assert(idx == 1);
#define FINDA(v_,A_,istart_,E_) \
 int v_=-1; {_T_(A_) a_ = A_; \
for (int i_=istart_,n_=array_len(a_); i_<n_; i_++) { \
         _T_(*a_) _ = a_[i_]; if (E_) { v_=i_; break; } \
}}

/// Like `FINDA` but starts searching at end.
 // Declares a variable `v`, which will be -1 if no match is possible.
 // @param v new index
 // @param A the array
 // @param istart where to start searching (-ve from end, +v is valid index)
 // @param expr expression in `_`
 // @macro RFINDA
#define RFINDA(v_,A_,iend_,E_) \
 int v_=-1; {_T_(A_) a_ = A_; \
for (int i_= iend_<0 ? array_len(a_)+iend_: iend_; i_>=0; i_--) { \
         _T_(*a_) _ = a_[i_]; if (E_) { v_=i_; break; } \
}}

 /// Like `FINDA` but assumes array ends with `NULL`.
 // Declares a variable `v`, which will be -1 if no match is possible.
 // @param v new index
 // @param A the array
 // @param expr expression in `_`
 // @macro FINDZ
#define FINDZ(v_,A_,E_) \
 int v_=-1; {_T_(*A_) *a_ = A_;  \
 for (int i_=0; *a_; a_++,i_++) { _T_(*a_) _ = *a_; if (E_) { v_=i_; break; } }}

#endif
