//
// Created by raver119 on 21.11.17.
//

#include "testlayers.h"
#include <memory>
#include <NDArray.h>
#include <NDArrayFactory.h>

using namespace nd4j;

//////////////////////////////////////////////////////////////////////
class NDArrayTest2 : public testing::Test {
public:

};


TEST_F(NDArrayTest2, Test_ByteVector_1) {
    NDArray<float> x('c', {10, 10});
    NDArrayFactory<float>::linspace(1, x);

    auto vec = x.asByteVector();

    auto restored = new NDArray<float>((float *)vec.data(), x.shapeInfo());
    restored->triggerAllocationFlag(false, false);

    ASSERT_TRUE(x.equalsTo(restored));

    delete restored;
}

TEST_F(NDArrayTest2, Test_ByteVector_2) {
    NDArray<float16> x('c', {10, 10});
    NDArrayFactory<float16>::linspace(1, x);

    auto vec = x.asByteVector();

    auto restored = new NDArray<float16>((float16 *)vec.data(), x.shapeInfo());
    restored->triggerAllocationFlag(false, false);

    ASSERT_TRUE(x.equalsTo(restored));

    delete restored;
}

TEST_F(NDArrayTest2, Test_ByteVector_3) {
    NDArray<double> x('c', {10, 10});
    NDArrayFactory<double>::linspace(1, x);

    auto vec = x.asByteVector();

    auto restored = new NDArray<double>((double *)vec.data(), x.shapeInfo());
    restored->triggerAllocationFlag(false, false);

    ASSERT_TRUE(x.equalsTo(restored));

    delete restored;
}

TEST_F(NDArrayTest2, Test_IndexReduce_1) {
    NDArray<float> x('c', {1, 5}, {1, 2, 3, 4, 5});

    float extras[] = {3.0, 0.0, 10};
    int idx = x.template indexReduceNumber<simdOps::FirstIndex<float>>(extras);

    ASSERT_EQ(2, idx);
}