//
//  @author raver119@gmail.com
//

#include <helpers/ShapeUtils.h>
#include <ops/declarable/CustomOperations.h>

namespace nd4j {
    namespace ops {
        CUSTOM_OP_IMPL(Where, 1, 1, false, 0, 0) {
            auto condition = INPUT_VARIABLE(0);

            if (block.width() == 3) {
                auto x = INPUT_VARIABLE(1);
                auto y = INPUT_VARIABLE(2);

                auto z = OUTPUT_VARIABLE(0);

                REQUIRE_TRUE(x->isSameShape(y), 0, "X and Y must have equal shapes");

                // if cond matches x/y shape - we have per-element mask
                if (condition->isSameShape(x)) {
                    // FIXME: for perf it might be better to issue memcpy here, and fill only mismatched values from either X or Y
#pragma omp parallel for
                    for (int e = 0; e < condition->lengthOf(); e++) {
                        T v = condition->getIndexedScalar(e);
                        T r = v == (T) 0.0f ? y->getIndexedScalar(e) : x->getIndexedScalar(e);
                        z->putIndexedScalar(e, r);
                    }
                } else {
                    REQUIRE_TRUE(condition->lengthOf() == x->sizeAt(0), 0, "Condition length should be equal to the dim0 of x/y to act as TAD-mask, but got %d instead", condition->lengthOf());

                    auto dims = ShapeUtils<T>::convertAxisToTadTarget(x->rankOf(), {0});
                    auto tadsX = NDArrayFactory<T>::allTensorsAlongDimension(x, dims);
                    auto tadsY = NDArrayFactory<T>::allTensorsAlongDimension(y, dims);
                    auto tadsZ = NDArrayFactory<T>::allTensorsAlongDimension(z, dims);

                    for (int e = 0; e < tadsX->size(); e++) {
                        T v = condition->getIndexedScalar(e);
                        
                        if (v == (T) 0.0f)
                            tadsZ->at(e)->assign(tadsY->at(e));
                        else
                            tadsZ->at(e)->assign(tadsX->at(e));
                    }

                    delete tadsX;
                    delete tadsY;
                    delete tadsZ;
                }
            } else {
                // in this case we return 2D matrix, which basically contains coordinates fo true

                REQUIRE_TRUE(block.width() == 1, 0, "Where op takes either 1 or 3 operands, But got %d operands instead", block.width());

                int width = condition->rankOf(); 

                std::vector<int> dims = ShapeUtils<T>::convertAxisToTadTarget(width, {0});

                NDArrayList<T> list(0, true);
                int cnt = 0;

                int idx[MAX_RANK];
                for (int e = 0; e < condition->lengthOf(); e++) {
                    shape::ind2subC(condition->rankOf(), condition->shapeOf(), e, idx);

                    Nd4jIndex offset = shape::getOffset(0, condition->shapeOf(), condition->stridesOf(), idx, condition->rankOf());
                    T v = condition->buffer()[offset];
                    if (v != (T) 0.0f) {
                        auto array = new NDArray<T>('c', {1, condition->rankOf()});
                        for (int f = 0; f < condition->rankOf(); f++)
                            array->putIndexedScalar(f, (T) idx[f]);
                            
                        list.write(cnt++, array);
                    }
                }

                auto result = list.stack();
                OVERWRITE_RESULT(result);
            }

            return ND4J_STATUS_OK;
        }
        DECLARE_SYN(where, Where);

        DECLARE_SHAPE_FN(Where) {
            if (block.width() == 3) {
                auto inShape = inputShape->at(1);
                int *newshape;
                ALLOCATE(newshape, block.getWorkspace(), shape::shapeInfoLength(inShape), int);
                memcpy(newshape, inShape, shape::shapeInfoByteLength(inShape));

                return new ShapeList(newshape);
            } else {
                // FIXME: we can't estimate result here in this case
                auto inShape = inputShape->at(0);

                int *newshape;
                ALLOCATE(newshape, block.getWorkspace(), shape::shapeInfoLength(2), int);
            
                newshape[0] = 2;
                newshape[1] = 10;
                newshape[2] = 10;
                newshape[3] = 1;
                newshape[4] = 1;
                newshape[5] = 0;
                newshape[6] = 1;
                newshape[7] = 99;

                return new ShapeList(newshape);
            }
        }
    }
}