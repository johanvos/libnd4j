//
// Created by raver119 on 29/10/17.
//


#include <ops/declarable/CustomOperations.h>
#include <helpers/ShapeUtils.h>

namespace nd4j {
    namespace ops {

//////////////////////////////////////////////////////////////////////////
// here iArgs is int vector of ordered set of dimensions to be permuted
        CUSTOM_OP_IMPL(permute, 1, 1, true, 0, -2) {
            NDArray<T> *x = INPUT_VARIABLE(0);

            bool replace = false;

            auto arguments = block.getIArguments();
            if (block.width() == 2 && arguments->size() == 0) {
                auto axis = INPUT_VARIABLE(1);
                for (int e = 0; e < axis->lengthOf(); e++) {
                    int ax = (int) axis->getScalar(e);
                    if (ax < 0)
                        ax += x->rankOf();

                    arguments->emplace_back(ax);
                }

                replace = true;
            } else if (arguments->size() == 0) {
                for (int e = x->rankOf() - 1; e >= 0; e--)
                    arguments->emplace_back(e);
            }

            if(block.isInplace()) {		// in-place
                x->permutei(*arguments);
                STORE_RESULT(x);
            } else {	
                if (!replace) {			// not-in-place        
                    NDArray<T>* output = OUTPUT_VARIABLE(0);
                    x->permute(*arguments, *output);

                    STORE_RESULT(output);
                } else {
                    auto output = x->dup();
                    output->permutei(*arguments);
                    
                    OVERWRITE_RESULT(output);
                }           
            }
        return ND4J_STATUS_OK;
        }

        DECLARE_SHAPE_FN(permute) {
            auto shapeList = new ShapeList();
            std::vector<int>* arguments = block.getIArguments();
            if (inputShape->size() == 1 && arguments->size() > 0) {
                int* outputShapeInfo = ShapeUtils<T>::evalPermShapeInfo(arguments->data(), arguments->size(), *INPUT_VARIABLE(0));
                shapeList->push_back(outputShapeInfo);
            } else if (inputShape->size() == 2) {
                // dead end
                int *newshape;
                ALLOCATE(newshape, block.getWorkspace(), shape::shapeInfoLength(inputShape->at(0)), int);
                memcpy(newshape, inputShape->at(0), shape::shapeInfoByteLength(inputShape->at(0)));
                shapeList->push_back(newshape);
            } else {
                int rank = shape::rank(inputShape->at(0));
                for (int e = rank - 1; e >= 0; e--)
                    arguments->emplace_back(e);

                int* outputShapeInfo = ShapeUtils<T>::evalPermShapeInfo(arguments->data(), arguments->size(), *INPUT_VARIABLE(0));
                shapeList->push_back(outputShapeInfo);
            }
    
            return shapeList;
        }
    }
}

