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
*/

#ifndef _LLIB_ARRAY_H
#define _LLIB_ARRAY_H

#include "obj.h"

#ifdef __cplusplus
#define _T_ decltype
#else
#define _T_ __typeof__
#endif

/// Iterate over an array.
// @param arr the array (can be an expression)
// @param expr the expression containing `_`
// @macro FORA
// @usage FORA(ints,printf("%i ",_));
#define FORA(A_,E_) {_T_(A_) a_ = A_; FOR(i,array_len(a_)) \
  { _T_(*a_) _ = a_[i]; E_; }}

/// Declare an array which is `[F(x)|x in A]`
// @param T the resulting type
// @param v the new array name
// @param F the expression containing `_`
// @param A the source array
// @macro MAPA
// @usage MAPA(int,ints,ceil(_),floats);
#define MAPA(T_,v_,E_,A_) T_ *v_; {_T_(A_) a_ = A_; v_ = array_new(T_,array_len(a_)); \
 FOR(i_,array_len(a_)) { _T_(*a_) _ = a_[i_]; v_[i_] = E_; } }

/// Declare a reference array which is `[F(x)|x in A]`
// @param T the resulting type
// @param v the new array name
// @param expr the expression containing `_`
// @param arr the source array
// @macro MAPAR
// @usage MAPAR(char*,strs,str_fmt("%d",_),ints);
#define MAPAR(T_,v_,E_,A_) T_ *v_; {_T_(A_) a_ = A_; v_ = array_new_ref(T_,array_len(a_)); \
 FOR(i_,array_len(a_)) { _T_(*a_) _ = a_[i_]; v_[i_] = E_; } }

/// New array containing matching elements from `A`.
// @param T array type
// @param v the new array name
// @param A the input array
// @param expr the filter expression in `_`
// @macro FILTA
// @usage FILTA(int,non_zero,ints,_ > 0);
#define FILTA(T_,v_,A_,E_) T_ *v_; {_T_(A_) a_ = A_; T_**s_ = seq_new(T_); \
 FOR(i_,array_len(a_)) { _T_(*a_) _ = a_[i_]; if (E_) seq_add(s_,_); } \
 v_ = (T_*)seq_array_ref(s_); }

/// New reference array containing matching elements from `A`.
// @param T array type
// @param v the new array name
// @param A the input array
// @param expr the filter expression in `_`
// @macro FILTAR
// @usage FILTAR(char*,matches,lines,str_starts_with(_,"log:"));
 #define FILTAR(T_,v_,A_,E_) T_ *v_; {_T_(A_) a_ = A_; T_**s_ = seq_new_ref(T_); \
 FOR(i_,array_len(a_)) { _T_(*a_) _ = a_[i_]; if (E_) seq_add(s_,obj_ref(_)); } \
 v_ = (T_*)seq_array_ref(s_); }

#endif
