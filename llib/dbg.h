#ifndef __LLIB_DBG_H
#define __LLIB_DBG_H

#define DUMP(V,fmt) printf(#V " = " fmt "\n",V)
#define DI(V) DUMP(V,"%d")
#define DS(V) DUMP(V,"'%s'")
#define DF(V) DUMP(V,"%f")
#define DUMPA(T,A,fmt) printf(#A ": "); FOR_ARR(T,P_,A) printf(fmt " ",*P_); printf("\n")
#endif

