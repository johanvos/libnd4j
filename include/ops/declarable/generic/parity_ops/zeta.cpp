//
// Created by Yurii Shyrma on 12.12.2017.
//

#include <ops/declarable/CustomOperations.h>
#include <ops/declarable/helpers/zeta.h>

namespace nd4j {
    namespace ops {


//////////////////////////////////////////////////////////////////////////
// calculate the Hurwitz zeta function
CONFIGURABLE_OP_IMPL(zeta, 2, 1, false, 0, 0) {

	NDArray<T>* x = INPUT_VARIABLE(0);
    NDArray<T>* q = INPUT_VARIABLE(1);

	NDArray<T>* output   = OUTPUT_VARIABLE(0);

    if(!x->isSameShape(q))
        throw "CONFIGURABLE_OP zeta: two input arrays must have the same shapes !";

    int arrLen = x->lengthOf();

    for(int i = 0; i < arrLen; ++i ) {
        
        if((*x)(i) <= (T)1.)
            throw "CONFIGURABLE_OP zeta: all elements of x array must be > 1 !";
        
        if((*q)(i) <= (T)0.)
            throw "CONFIGURABLE_OP zeta: all elements of q array must be > 0 !";
    }

    *output = helpers::zeta<T>(*x, *q);

    return ND4J_STATUS_OK;
}
DECLARE_SYN(Zeta, zeta);



}
}

