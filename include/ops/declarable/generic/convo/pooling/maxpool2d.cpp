//
// Created by raver119 on 29/10/17.
//

#include <ops/declarable/CustomOperations.h>
#include <ops/declarable/generic/helpers/convolutions.h>

namespace nd4j {
    namespace ops {
        //////////////////////////////////////////////////////////////////////////
        CUSTOM_OP_IMPL(maxpool2d_bp, 2, 1, false, 0, 9) {

            NDArray<T>* input = INPUT_VARIABLE(0);
            REQUIRE_TRUE(input->rankOf() == 4, 0, "Input should have rank of 4, but got %i instead", input->rankOf());
            NDArray<T>* epsilon = INPUT_VARIABLE(1);
            NDArray<T>* outEpsilon = this->getZ(block);
            // 0,1 - kernel Height/Width; 2,3 - stride Height/Width; 4,5 - pad Height/Width; 6,7 - dilation Height/Width; 8 - same mode;
            std::vector<int> argI = *(block.getIArguments());

            int kH = argI[0];
            int kW = argI[1];
            int sH = argI[2];
            int sW = argI[3];
            int pH = argI[4];
            int pW = argI[5];
            int dH = argI[6];
            int dW = argI[7];
            int isSameMode = argI[8];

            int bS = input->getShapeInfo()[1];
            int iD = input->getShapeInfo()[2];
            int iH = input->getShapeInfo()[3];
            int iW = input->getShapeInfo()[4];

            // calculate output Height/Width
            int oH, oW;
            ConvolutionUtils<T>::calcOutHWpool2D(oH, oW, kH, kW, sH, sW, pH, pW, dH, dW, iH, iW, isSameMode);

            bool cOrderStrides = false;
            bool isEpsilonDup = false;
            if (epsilon->ordering() != 'c') {
                epsilon = epsilon->dup('c');
                cOrderStrides = true;
                isEpsilonDup = true;
            }

            int strideToCompare[] = {oH*oW, iD*oH*oW, oW, 1};
            if (!cOrderStrides && shape::strideDescendingCAscendingF(epsilon->getShapeInfo())) {
                cOrderStrides = true;
            }
            else if (!shape::strideEquals(strideToCompare, 4, epsilon->stridesOf(), epsilon->rankOf())) {
                epsilon = epsilon->dup('c');
                cOrderStrides = true;
                isEpsilonDup = true;
            }

            NDArray<T>* col6d = nullptr;
            NDArray<T>* col6dPermuted = nullptr;
            NDArray<T>* epsilon1d = nullptr;

            if (cOrderStrides) {
                col6d = new NDArray<T>('c', {bS, iD, oH, oW, kH, kW}, block.getWorkspace());
                col6dPermuted = col6d->permute({0, 1, 4, 5, 2, 3});
                epsilon1d = epsilon->reshape('c', {(int) epsilon->lengthOf(), 1}); //zero copy reshape
            }
            else {
                col6d = new NDArray<T>('c', {iD, bS, oH, oW, kH, kW}, block.getWorkspace());
                col6dPermuted = col6d->permute({1, 0, 4, 5, 2, 3});
                NDArray<T>* epsilonTemp = epsilon->permute({1, 0, 2, 3});
                epsilon1d = epsilonTemp->reshape('c', {(int) epsilon->lengthOf(), 1}); //Should be a zero-copy reshape always
                delete epsilonTemp;
            }

            NDArray<T>* col2d = col6d->reshape('c', {bS*iD*oH*oW, kH*kW});

            T extraParams1[] = {(T)kH, (T)kW, (T)sH, (T)sW, (T)pH, (T)pW, (T)dH, (T)dW};
            input->template applyTransform<simdOps::Im2col<T>>(col6dPermuted, extraParams1);

            //FIXME: this op should be moved to CustomOps
            T extraParams2[] = {(T)1.f, (T)1.f};
            col2d->template applyTransform<simdOps::IsMax<T>>(extraParams2);
            col2d->muliColumnVector(epsilon1d);

            T extraParams3[] = {(T) sH, (T)sW, (T)pH, (T)pW, (T)iH, (T)iW, (T)dH, (T)dW};   			// ??? zeros
            col6dPermuted->template applyTransform<simdOps::Col2Im<T>>(outEpsilon, extraParams3);

            STORE_RESULT(*outEpsilon);		// ???

            if(isEpsilonDup)
                delete epsilon;
            delete col6d;
            delete col6dPermuted;
            delete epsilon1d;
            delete col2d;

            return ND4J_STATUS_OK;
        }
        DECLARE_SYN(MaxPool2D_bp, maxpool2d_bp);
        DECLARE_SYN(MaxPool_bp, maxpool2d_bp);

        DECLARE_SHAPE_FN(maxpool2d_bp) {

            int* newShapeInfo = nullptr;
            ALLOCATE(newShapeInfo, block.getWorkspace(), shape::shapeInfoLength(inputShape->at(0)), int);
            memcpy(newShapeInfo, inputShape->at(0), shape::shapeInfoByteLength(inputShape->at(0)));
            return new ShapeList(newShapeInfo);
        }


        //////////////////////////////////////////////////////////////////////////
        // maxpool2d corresponds to poolingMode=0
        CUSTOM_OP_IMPL(maxpool2d, 1, 1, false, 0, 9) {

            NDArray<T> *x = INPUT_VARIABLE(0);

            REQUIRE_TRUE(x->rankOf() == 4, 0, "Input should have rank of 4, but got %i instead", x->rankOf());

            const int bSize = x->sizeAt(0);
            const int inD = x->sizeAt(1);
            const int inY = x->sizeAt(2);
            const int inX = x->sizeAt(3);

            std::vector<int> argI = *(block.getIArguments());
            auto z = this->getZ(block);

            int pY = argI[4];
            int pX = argI[5];

            const bool isSameMode = INT_ARG(8) > 0;
            if (isSameMode)
                ConvolutionUtils<T>::_calcPadding2D(pY, pX, z->sizeAt(2), z->sizeAt(3), inY, inX, argI[0], argI[1], argI[2], argI[3], argI[6], argI[7]);            // 0,1 - kernel Height/Width; 2,3 - stride Height/Width; 4,5 - pad Height/Width; 6,7 - dilation Height/Width; 8 - same mode;

            // 0,1 - kernel Height/Width; 2,3 - stride Height/Width; 4,5 - pad Height/Width; 6,7 - dilation Height/Width; 8,9 - poolingMode; 10 - divisor;
            std::vector<T> argT = {(T)argI[0], (T)argI[1], (T)argI[2], (T)argI[3], (T) argI[4], (T) argI[5], (T)argI[6], (T)argI[7], (T)0.f, (T)0.f, (T)1.f};
            //std::vector<T> argT = {(T) argI[0], (T) argI[1], (T) argI[2], (T) argI[3], (T) argI[4], (T) argI[5], (T)argI[6], (T)argI[7], (T)1.f, (T)1.f, (T)1.f};

            x->template applyTransform<simdOps::Pooling2D<T>>(z, argT.data());

            STORE_RESULT(*z);

            //z->printShapeInfo("MaxPool2D result shape");

            return ND4J_STATUS_OK;
        }
        DECLARE_SYN(MaxPool2D, maxpool2d);
        DECLARE_SYN(MaxPool, maxpool2d);
        DECLARE_SYN(maxpool, maxpool2d);

        DECLARE_SHAPE_FN(maxpool2d) {
            //NDArray<T> *x = block.getVariables().at(0)->getNDArray();
            int* inShape = inputShape->at(0);
            int* shapeOf = shape::shapeOf(inShape);
            // 0 - number of dimensions; 1,2 - kernel Height/Width; 3,4 - stride Height/Width; 5,6 - pad Height/Width; 7,8 - dilation Height/Width; 9,10 - input Height/Width; 11 - batch size; 12 - input depth; 13 - same mode;
            std::vector<int> argI = *(block.getIArguments());
            int kH = argI[0];
            int kW = argI[1];
            int sH = argI[2];
            int sW = argI[3];
            int pH = argI[4];
            int pW = argI[5];
            int dH = argI[6];
            int dW = argI[7];
            int isSameMode = argI[8];

            int bS = shapeOf[0];
            int iD = shapeOf[1];
            int iH = shapeOf[2];
            int iW = shapeOf[3];

            char order = shape::order(inShape); // output order must be equal to input order

            // calculate output Height/Width
            int oH, oW;
            ConvolutionUtils<T>::calcOutHWpool2D(oH, oW, kH, kW, sH, sW, pH, pW, dH, dW, iH, iW, isSameMode);

            const bool bisSameMode = INT_ARG(8) > 0;
            if (bisSameMode)
                ConvolutionUtils<T>::_calcPadding2D(pH, pW, oH, oW, iH, iW, argI[0], argI[1], argI[2], argI[3], argI[6], argI[7]);

            // allocate memory for new shape
            int* newShapeInfo = nullptr;
            ALLOCATE(newShapeInfo, block.getWorkspace(), 12, int);
            newShapeInfo[0] = 4;		// rank
            newShapeInfo[1] = bS;
            newShapeInfo[2] = iD;
            newShapeInfo[3] = oH;
            newShapeInfo[4] = oW;
            shape::updateStrides(newShapeInfo, order);

            return new ShapeList(newShapeInfo);
        }
    }
}