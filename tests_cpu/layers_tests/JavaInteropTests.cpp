//
// @author raver119@gmail.com
//

#include <NativeOps.h>
#include <NDArray.h>
#include <ops/declarable/CustomOperations.h>
#include <ops/declarable/OpRegistrator.h>
#include "testlayers.h"

using namespace nd4j;
using namespace nd4j::ops;

class JavaInteropTests : public testing::Test {
public:

};


TEST_F(JavaInteropTests, TestShapeExposure1) {
    NDArray<float> input('c', {1, 2, 5, 4});
    NDArray<float> weights('c', {3, 2, 2, 2});
    NDArray<float> exp('c', {1, 3, 5, 4});


    NativeOps nativeOps;

    nd4j::ops::conv2d<float> op;

    std::vector<float> tArgs({});
    std::vector<int> iArgs({2, 2, 1, 1, 0, 0, 1, 1, 1});


    Nd4jPointer ptrs[] = {(Nd4jPointer) input.getShapeInfo(), (Nd4jPointer) weights.getShapeInfo()};

    auto shapeList = nativeOps.calculateOutputShapesFloat(nullptr, op.getOpHash(), ptrs, 2, tArgs.data(), tArgs.size(), iArgs.data(), iArgs.size());

    //ASSERT_EQ(1, shapeList->size());

    ASSERT_EQ(exp.rankOf(), shape::rank((int *)shapeList[0]));
    ASSERT_EQ(exp.sizeAt(0), shape::shapeOf((int *)shapeList[0])[0]);
    ASSERT_EQ(exp.sizeAt(1), shape::shapeOf((int *)shapeList[0])[1]);
    ASSERT_EQ(exp.sizeAt(2), shape::shapeOf((int *)shapeList[0])[2]);
    ASSERT_EQ(exp.sizeAt(3), shape::shapeOf((int *)shapeList[0])[3]);

    int *ptr = (int *) shapeList[0];
    delete[] ptr;
    delete[] shapeList;
}


TEST_F(JavaInteropTests, TestSconv2d_1) {
    NDArray<float> input('c', {3, 8, 8, 8});
    NDArray<float> weightsD('c', {1, 3, 1, 1});
    NDArray<float> weightsP('c', {2, 3, 1, 1});
    NDArray<float> bias('c', {1, 2});
    NDArray<float> output('c', {3, 2, 8, 8});
    output.assign(0.0);

    NDArrayFactory<float>::linspace(1, input);
    NDArrayFactory<float>::linspace(1, weightsD);
    NDArrayFactory<float>::linspace(1, weightsP);
    NDArrayFactory<float>::linspace(1, bias);

    NDArray<float> expOutput('c', {3, 2, 8, 8});

    nd4j::ops::sconv2d<float> op;


    Nd4jPointer ptrsInBuffer[] = {(Nd4jPointer) input.getBuffer(), (Nd4jPointer) weightsD.getBuffer(), (Nd4jPointer) weightsP.getBuffer(), (Nd4jPointer) bias.getBuffer()};
    Nd4jPointer ptrsInShapes[] = {(Nd4jPointer) input.getShapeInfo(), (Nd4jPointer) weightsD.getShapeInfo(), (Nd4jPointer) weightsP.getShapeInfo(), (Nd4jPointer) bias.getShapeInfo()};


    Nd4jPointer ptrsOutBuffers[] = {(Nd4jPointer) output.getBuffer()};
    Nd4jPointer ptrsOutShapes[] = {(Nd4jPointer) output.getShapeInfo()};

    NativeOps nativeOps;

    int exp[] = {1, 1, 1, 1, 0, 0, 1, 1, 0};

    nativeOps.execCustomOpFloat(nullptr, op.getOpHash(), ptrsInBuffer, ptrsInShapes, 4, ptrsOutBuffers, ptrsOutShapes, 1,
                                nullptr, 0, exp, 9, false);

    //output.printBuffer("output");

    ASSERT_NEAR(17551, output.getScalar(0), 1e-5);
}

TEST_F(JavaInteropTests, TestSconv2d_2) {
    NDArray<float> input('c', {3, 3, 8, 8});
    NDArray<float> weightsD('c', {1, 3, 1, 1});
    NDArray<float> output('c', {3, 3, 8, 8});
    output.assign(0.0);

    NDArrayFactory<float>::linspace(1, input);
    NDArrayFactory<float>::linspace(1, weightsD);

    NDArray<float> expOutput('c', {3, 3, 8, 8});

    nd4j::ops::sconv2d<float> op;


    Nd4jPointer ptrsInBuffer[] = {(Nd4jPointer) input.getBuffer(), (Nd4jPointer) weightsD.getBuffer()};
    Nd4jPointer ptrsInShapes[] = {(Nd4jPointer) input.getShapeInfo(), (Nd4jPointer) weightsD.getShapeInfo()};


    Nd4jPointer ptrsOutBuffers[] = {(Nd4jPointer) output.getBuffer()};
    Nd4jPointer ptrsOutShapes[] = {(Nd4jPointer) output.getShapeInfo()};

    NativeOps nativeOps;

    int exp[] = {1, 1, 1, 1, 0, 0, 1, 1, 0};

    nativeOps.execCustomOpFloat(nullptr, op.getOpHash(), ptrsInBuffer, ptrsInShapes, 2, ptrsOutBuffers, ptrsOutShapes, 1, nullptr, 0, exp, 9, false);

    //output.printBuffer("output");

    ASSERT_NEAR(1, output.getScalar(0), 1e-5);
}


TEST_F(JavaInteropTests, TestPooling2d_1) {
    NDArray<float> input('c', {1, 2, 4, 5});
    NDArray<float> output('c', {1, 2, 4, 5});


    Nd4jPointer ptrsInBuffer[] = {(Nd4jPointer) input.getBuffer()};
    Nd4jPointer ptrsInShapes[] = {(Nd4jPointer) input.getShapeInfo()};

    Nd4jPointer ptrsOutBuffers[] = {(Nd4jPointer) output.getBuffer()};
    Nd4jPointer ptrsOutShapes[] = {(Nd4jPointer) output.getShapeInfo()};

    std::vector<int> iArgs({2, 2, 1, 1, 0, 0, 1, 1, 1 , 1, 1});

    nd4j::ops::pooling2d<float> op;

    NativeOps nativeOps;

    Nd4jStatus status = nativeOps.execCustomOpFloat(nullptr, op.getOpHash(), ptrsInBuffer, ptrsInShapes, 1, ptrsOutBuffers, ptrsOutShapes, 1, nullptr, 0, iArgs.data(), 11, false);
    ASSERT_EQ(ND4J_STATUS_OK, status);

}


TEST_F(JavaInteropTests, TestMaxPooling2d_1) {
    NDArray<float> input('c', {1, 2, 4, 5});
    NDArray<float> output('c', {1, 2, 4, 5});
    NDArrayFactory<float>::linspace(1, input);


    Nd4jPointer ptrsInBuffer[] = {(Nd4jPointer) input.getBuffer()};
    Nd4jPointer ptrsInShapes[] = {(Nd4jPointer) input.getShapeInfo()};

    Nd4jPointer ptrsOutBuffers[] = {(Nd4jPointer) output.getBuffer()};
    Nd4jPointer ptrsOutShapes[] = {(Nd4jPointer) output.getShapeInfo()};

    std::vector<int> iArgs({2, 2, 1, 1, 0, 0, 1, 1, 1});

    nd4j::ops::maxpool2d<float> op;

    NativeOps nativeOps;

    Nd4jStatus status = nativeOps.execCustomOpFloat(nullptr, op.getOpHash(), ptrsInBuffer, ptrsInShapes, 1, ptrsOutBuffers, ptrsOutShapes, 1, nullptr, 0, iArgs.data(), 9, false);
    ASSERT_EQ(ND4J_STATUS_OK, status);

}
TEST_F(JavaInteropTests, TestCol2Im_1) {
    /*
        o.d.n.l.c.ConvolutionLayer - eps shape: [6, 1, 2, 2, 2, 4, 5, 160, 4, 2, 1, 40, 8, 0, -1, 99]
        o.d.n.l.c.ConvolutionLayer - epsNext shape: [4, 1, 2, 4, 5, 20, 20, 5, 1, 0, 1, 99]
        o.d.n.l.c.ConvolutionLayer - Strides: [1, 1]
        o.d.n.l.c.ConvolutionLayer - Padding: [0, 0]
        o.d.n.l.c.ConvolutionLayer - Input: [4,5]
        o.d.n.l.c.ConvolutionLayer - Dilation: [1, 1]
     */
    NDArray<float> input('c', {1, 2, 2, 2, 4, 5});
    NDArray<float> output('c', {1, 2, 4, 5});
    NDArrayFactory<float>::linspace(1, input);

    Nd4jPointer ptrsInBuffer[] = {(Nd4jPointer) input.getBuffer()};
    Nd4jPointer ptrsInShapes[] = {(Nd4jPointer) input.getShapeInfo()};


    Nd4jPointer ptrsOutBuffers[] = {(Nd4jPointer) output.getBuffer()};
    Nd4jPointer ptrsOutShapes[] = {(Nd4jPointer) output.getShapeInfo()};

    nd4j::ops::col2im<float> op;

    NativeOps nativeOps;

    int exp[] = {1, 1, 1, 1, 4, 5, 1, 1, 1};

    nativeOps.execCustomOpFloat(nullptr, op.getOpHash(), ptrsInBuffer, ptrsInShapes, 1, ptrsOutBuffers, ptrsOutShapes, 1, nullptr, 0, exp, 9, false);

    ASSERT_TRUE(output.meanNumber() > 0.0);
}

TEST_F(JavaInteropTests, TestPNorm_1) {
    /*
        o.d.n.l.c.s.SubsamplingLayer - input: [4, 1, 3, 4, 4, 16, 16, 4, 1, 0, 1, 99]
        o.d.n.l.c.s.SubsamplingLayer - output: [4, 1, 3, 3, 3, 27, 9, 3, 1, 0, 1, 99]
        o.d.n.l.c.s.SubsamplingLayer - Kernel: [2, 2]
        o.d.n.l.c.s.SubsamplingLayer - Strides: [1, 1]
        o.d.n.l.c.s.SubsamplingLayer - Pad: [0, 0]
        o.d.n.l.c.s.SubsamplingLayer - Dilation: [1, 1]
        o.d.n.l.c.s.SubsamplingLayer - Same: false
        o.d.n.l.c.s.SubsamplingLayer - pnorm: 2
     */
    NDArray<float> input('c', {1, 3, 4, 4});
    NDArray<float> output('c', {1, 3, 3, 3});
    NDArrayFactory<float>::linspace(1, input);

    NativeOps nativeOps;

    nd4j::ops::pnormpool2d<float> op;

    int exp[] = {2, 2, 1, 1, 0, 0, 1, 1, 0, 2};

    Nd4jPointer ptrsInBuffer[] = {(Nd4jPointer) input.getBuffer()};
    Nd4jPointer ptrsInShapes[] = {(Nd4jPointer) input.getShapeInfo()};

    Nd4jPointer ptrsOutBuffers[] = {(Nd4jPointer) output.getBuffer()};
    Nd4jPointer ptrsOutShapes[] = {(Nd4jPointer) output.getShapeInfo()};

    nativeOps.execCustomOpFloat(nullptr, op.getOpHash(), ptrsInBuffer, ptrsInShapes, 1, ptrsOutBuffers, ptrsOutShapes, 1, nullptr, 0, exp, 10, false);

    ASSERT_TRUE(output.meanNumber() > 0.0);
}

TEST_F(JavaInteropTests, TestUpsampling_1) {
    /*
        o.d.n.l.c.u.Upsampling2D - Input shape: [4, 1, 3, 4, 4, 16, 16, 4, 1, 0, 1, 99]
        o.d.n.l.c.u.Upsampling2D - Output shape: [4, 1, 3, 8, 8, 192, 64, 8, 1, 0, 1, 99]
        o.d.n.l.c.u.Upsampling2D - size: 2
     */

    NDArray<float> input('c', {1, 3, 4, 4});
    NDArray<float> output('c', {1, 3, 8, 8});
    NDArrayFactory<float>::linspace(1, input);

    NativeOps nativeOps;

    nd4j::ops::upsampling2d<float> op;

    int exp[] = {2};

    Nd4jPointer ptrsInBuffer[] = {(Nd4jPointer) input.getBuffer()};
    Nd4jPointer ptrsInShapes[] = {(Nd4jPointer) input.getShapeInfo()};

    Nd4jPointer ptrsOutBuffers[] = {(Nd4jPointer) output.getBuffer()};
    Nd4jPointer ptrsOutShapes[] = {(Nd4jPointer) output.getShapeInfo()};

    nativeOps.execCustomOpFloat(nullptr, op.getOpHash(), ptrsInBuffer, ptrsInShapes, 1, ptrsOutBuffers, ptrsOutShapes, 1, nullptr, 0, exp, 1, false);

    ASSERT_TRUE(output.meanNumber() > 0.0);
}


TEST_F(JavaInteropTests, TestInplace_1) {
    NDArray<float> input('c', {10, 10});
    //NDArray<float> exp('c', {10, 10});
    NDArrayFactory<float>::linspace(1, input);

    NativeOps nativeOps;

    nd4j::ops::clipbyvalue<float> op;

    float extras[] = {-1.0f, 1.0f};

    Nd4jPointer ptrsInBuffer[] = {(Nd4jPointer) input.getBuffer()};
    Nd4jPointer ptrsInShapes[] = {(Nd4jPointer) input.getShapeInfo()};


    Nd4jStatus result = nativeOps.execCustomOpFloat(nullptr, op.getOpHash(), ptrsInBuffer, ptrsInShapes, 1, nullptr, nullptr, 0, extras, 2, nullptr, 0, true);

    ASSERT_EQ(ND4J_STATUS_OK, result);

    ASSERT_NEAR(1.0, input.meanNumber(), 1e-5);
}

TEST_F(JavaInteropTests, Test_Synonyms_1) {
    auto op = OpRegistrator::getInstance()->getOperationHalf("RDiv");
    auto opRef = OpRegistrator::getInstance()->getOperationHalf("reversedivide");
    std::string nameExp("reversedivide");

    ASSERT_TRUE(op != nullptr);
    ASSERT_TRUE(opRef != nullptr);

    std::string name = *(op->getOpName());
    std::string nameRef = *(opRef->getOpName());

    ASSERT_EQ(nameExp, nameRef);
    ASSERT_EQ(nameRef, name);
}

TEST_F(JavaInteropTests, Test_Synonyms_2) {
    auto op = OpRegistrator::getInstance()->getOperationFloat("RDiv");
    auto opRef = OpRegistrator::getInstance()->getOperationFloat("reversedivide");
    std::string nameExp("reversedivide");

    ASSERT_TRUE(op != nullptr);
    ASSERT_TRUE(opRef != nullptr);

    std::string name = *(op->getOpName());
    std::string nameRef = *(opRef->getOpName());

    ASSERT_EQ(nameExp, nameRef);
    ASSERT_EQ(nameRef, name);
}

TEST_F(JavaInteropTests, Test_Synonyms_3) {
    auto op = OpRegistrator::getInstance()->getOperationDouble("RDiv");
    auto opRef = OpRegistrator::getInstance()->getOperationDouble("reversedivide");
    std::string nameExp("reversedivide");

    ASSERT_TRUE(op != nullptr);
    ASSERT_TRUE(opRef != nullptr);

    std::string name = *(op->getOpName());
    std::string nameRef = *(opRef->getOpName());

    ASSERT_EQ(nameExp, nameRef);
    ASSERT_EQ(nameRef, name);
}