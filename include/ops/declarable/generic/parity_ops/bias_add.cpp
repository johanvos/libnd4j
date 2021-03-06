//
//  @author raver119@gmail.com
//

#include <ops/declarable/CustomOperations.h>

namespace nd4j {
    namespace ops {
        //////////////////////////////////////////////////////////////////////////
        OP_IMPL(biasadd, 2, 1, true) {
            //REQUIRE_OK(this->validateInput2D(block));

            NDArray<T> *input = INPUT_VARIABLE(0);
            NDArray<T> *bias = INPUT_VARIABLE(1);

            REQUIRE_TRUE(bias->isRowVector(), 0, "Bias array should be a vector");

            NDArray<T> *z = OUTPUT_VARIABLE(0);

            if (input->isMatrix())
                input->addRowVector(bias, z);
            else {
                // TODO: we might want to use NDArray::applyTrueBroadcast here, like AddOp does
                std::vector<int> shape({-1, (int) bias->lengthOf()});
                //nd4j_debug("Reshaping to: [%i, %i]\n", -1, (int) bias->lengthOf());
                auto tArr = input->reshape(input->ordering(), shape);
                auto zArr = z->reshape(z->ordering(), shape);
                tArr->addRowVector(bias, zArr);

                delete tArr;
                delete zArr;
            }

            STORE_RESULT(*z);

            return ND4J_STATUS_OK;
        }
        DECLARE_SYN(bias_add, biasadd);


        CUSTOM_OP_IMPL(biasadd_bp, 3, 2, false, 0, 0) {
            auto input = INPUT_VARIABLE(0);
            auto bias = INPUT_VARIABLE(1);
            auto epsilonNext = INPUT_VARIABLE(2);

            auto epsilon = OUTPUT_VARIABLE(0);
            auto gradB = OUTPUT_VARIABLE(1);

            epsilon->assign(epsilonNext);

            // cnn case
            if (input->rankOf() == 4) {
                auto epsilonNext2d = epsilonNext->permute({1, 0, 2, 3});
                epsilonNext2d->reshapei('c', {(int) bias->lengthOf(), -1});

                auto sum = epsilonNext2d->sum({1});
                gradB->assign(sum);

                delete sum;
                delete epsilonNext2d;
            } else if (input->rankOf() == 2) {
                // regular fully-connected case
                auto sum = epsilonNext->sum({0});
                gradB->assign(sum);
                
                delete sum;
            }

            return ND4J_STATUS_OK;
        }
        DECLARE_SYN(BiasAddGrad, biasadd_bp);

        DECLARE_SHAPE_FN(biasadd_bp) {
            auto input = inputShape->at(0);
            auto bias = inputShape->at(1);

            int* epsShape;
            int* gradShape;
            ALLOCATE(epsShape, block.getWorkspace(), shape::shapeInfoLength(input), int);
            ALLOCATE(gradShape, block.getWorkspace(), shape::shapeInfoLength(bias), int);

            memcpy(epsShape, input, shape::shapeInfoByteLength(input));
            memcpy(gradShape, bias, shape::shapeInfoByteLength(gradShape));

            return new ShapeList({epsShape, gradShape});
        }
    }
}