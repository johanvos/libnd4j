//
// @author raver119@gmail.com
//

#include "testlayers.h"
#include <flatbuffers/flatbuffers.h>
#include <graph/generated/node_generated.h>
#include <graph/generated/graph_generated.h>
#include <graph/generated/result_generated.h>
#include <graph/Node.h>
#include <graph/Graph.h>
#include <GraphExecutioner.h>
#include <ops/declarable/CustomOperations.h>

using namespace nd4j;
using namespace nd4j::graph;

class FlatBuffersTest : public testing::Test {
public:
    int alpha = 0;

    int *cShape = new int[8]{2, 2, 2, 2, 1, 0, 1, 99};
    int *fShape = new int[8]{2, 2, 2, 1, 2, 0, 1, 102};


    ~FlatBuffersTest() {
        delete[] cShape;
        delete[] fShape;
    }
};

/**
 * Simple test that creates Node & reads it
 */
TEST_F(FlatBuffersTest, BasicTest1) {
    flatbuffers::FlatBufferBuilder builder(1024);

    auto name = builder.CreateString("wow");

    auto node = CreateFlatNode(builder, -1, name, OpType_TRANSFORM, 26, {0});

    builder.Finish(node);

    // now we have our buffer with data
    uint8_t *buf = builder.GetBufferPointer();
    int size = builder.GetSize();
    ASSERT_TRUE(size > 0);



    auto restored = GetFlatNode(buf);

    auto gA = new Node<float>(restored);
    auto gB = new Node<float>(restored);

    ASSERT_TRUE(gA->equals(gB));

    delete gA;
    delete gB;
}


TEST_F(FlatBuffersTest, FlatGraphTest1) {
    flatbuffers::FlatBufferBuilder builder(4096);

    auto array = new NDArray<float>(5, 5, 'c');
    array->assign(-2.0f);

    auto fShape = builder.CreateVector(array->getShapeInfoAsVector());
    auto fBuffer = builder.CreateVector(array->asByteVector());

    auto fArray = CreateFlatArray(builder, fShape, fBuffer, nd4j::graph::DataType::DataType_FLOAT);
    auto fVid = CreateIntPair(builder, -1);

    auto fVar = CreateFlatVariable(builder, fVid, 0, 0, fArray);

    std::vector<int> outputs1, outputs2, inputs1, inputs2;
    outputs1.push_back(2);
    outputs2.push_back(0);

    inputs1.push_back(-1);
    inputs2.push_back(1);


    auto vec1 = builder.CreateVector(outputs1);
    auto vec2 = builder.CreateVector(outputs2);

    auto in1 = builder.CreateVector(inputs1);
    auto in2 = builder.CreateVector(inputs2);

    auto name1 = builder.CreateString("wow1");
    auto name2 = builder.CreateString("wow2");

    auto node1 = CreateFlatNode(builder, 1, name1, OpType_TRANSFORM, 0, in1, 0, nd4j::graph::DataType::DataType_FLOAT, vec1);
    auto node2 = CreateFlatNode(builder, 2, name2, OpType_TRANSFORM, 2, in2, 0, nd4j::graph::DataType::DataType_FLOAT, vec2);

    std::vector<flatbuffers::Offset<FlatVariable>> variables_vector;
    variables_vector.push_back(fVar);

    std::vector<flatbuffers::Offset<FlatNode>> nodes_vector;

    nodes_vector.push_back(node1);
    nodes_vector.push_back(node2);

    auto nodes = builder.CreateVector(nodes_vector);

    auto variables = builder.CreateVector(variables_vector);

    FlatGraphBuilder graphBuilder(builder);

    graphBuilder.add_variables(variables);
    graphBuilder.add_id(119);
    graphBuilder.add_nodes(nodes);

    auto flatGraph = graphBuilder.Finish();

    builder.Finish(flatGraph);

    uint8_t *buf = builder.GetBufferPointer();
    int size = builder.GetSize();
    ASSERT_TRUE(size > 0);


    auto restoredGraph = GetFlatGraph(buf);
    ASSERT_EQ(119, restoredGraph->id());
    ASSERT_EQ(2, restoredGraph->nodes()->size());

    // checking op nodes
    ASSERT_EQ(0, restoredGraph->nodes()->Get(0)->opNum());
    ASSERT_EQ(2, restoredGraph->nodes()->Get(1)->opNum());
    ASSERT_EQ(0, restoredGraph->nodes()->Get(0)->opNum());

    // checking variables
    ASSERT_EQ(1, restoredGraph->variables()->size());
    ASSERT_EQ(-1, restoredGraph->variables()->Get(0)->id()->first());

    nd4j_printf("-------------------------\n","");

    Graph<float> graph(restoredGraph);

    ASSERT_EQ(2, graph.totalNodes());
    ASSERT_EQ(1, graph.rootNodes());


    auto vs = graph.getVariableSpace();

    ASSERT_EQ(OutputMode_IMPLICIT, graph.getExecutorConfiguration()->_outputMode);

    ASSERT_EQ(3, vs->totalEntries());
    ASSERT_EQ(1, vs->externalEntries());
    ASSERT_EQ(2, vs->internalEntries());

    auto var = vs->getVariable(-1)->getNDArray();

    ASSERT_TRUE(var != nullptr);

    ASSERT_EQ(-2.0, var->reduceNumber<simdOps::Mean<float>>());

    nd4j::graph::GraphExecutioner<float>::execute(&graph);

    auto result = (uint8_t *)nd4j::graph::GraphExecutioner<float>::executeFlatBuffer((Nd4jPointer) buf);

    auto flatResults = GetFlatResult(result);

    ASSERT_EQ(1, flatResults->variables()->size());
    ASSERT_TRUE(flatResults->variables()->Get(0)->name() != nullptr);
    ASSERT_TRUE(flatResults->variables()->Get(0)->name()->c_str() != nullptr);
    //nd4j_printf("VARNAME: %s\n", flatResults->variables()->Get(0)->name()->c_str());

    auto var0 = new Variable<float>(flatResults->variables()->Get(0));
    //auto var1 = new Variable<float>(flatResults->variables()->Get(1));

    ASSERT_NEAR(-0.4161468, var0->getNDArray()->reduceNumber<simdOps::Mean<float>>(), 1e-5);

    //ASSERT_TRUE(var->equalsTo(var0->getNDArray()));

    delete array;
    delete var0;
    delete[] result;
}

TEST_F(FlatBuffersTest, ExecutionTest1) {
    auto gA = new Node<float>(OpType_TRANSFORM);

    float *c = new float[4] {-1, -2, -3, -4};
    auto *array = new NDArray<float>(c, cShape);

    float *e = new float[4] {1, 2, 3, 4};
    auto *exp = new NDArray<float>(e, cShape);

    //gA->execute(array, nullptr, array);

    //ASSERT_TRUE(exp->equalsTo(array));

    delete gA;
    delete[] c;
    delete array;
    delete[] e;
    delete exp;
}

/*
TEST_F(FlatBuffersTest, ExplicitOutputTest1) {
    flatbuffers::FlatBufferBuilder builder(4096);

    auto x = new NDArray<float>(5, 5, 'c');
    x->assign(-2.0f);

    auto fXShape = builder.CreateVector(x->getShapeInfoAsVector());
    auto fXBuffer = builder.CreateVector(x->asByteVector());
    auto fXArray = CreateFlatArray(builder, fXShape, fXBuffer);
    auto fXid = CreateIntPair(builder, -1);

    auto fXVar = CreateFlatVariable(builder, fXid, 0, 0, fXArray);


    auto y = new NDArray<float>(5, 5, 'c');
    y->assign(-1.0f);

    auto fYShape = builder.CreateVector(y->getShapeInfoAsVector());
    auto fYBuffer = builder.CreateVector(y->asByteVector());
    auto fYArray = CreateFlatArray(builder, fYShape, fYBuffer);
    auto fYid = CreateIntPair(builder, -2);

    auto fYVar = CreateFlatVariable(builder, fYid, 0, 0, fYArray);


    std::vector<flatbuffers::Offset<IntPair>> inputs1, outputs1, outputs;
    inputs1.push_back(CreateIntPair(builder, -1));
    inputs1.push_back(CreateIntPair(builder, -2));

    outputs.push_back(CreateIntPair(builder, -1));
    outputs.push_back(CreateIntPair(builder, -2));

    auto out1 = builder.CreateVector(outputs1);
    auto in1 = builder.CreateVector(inputs1);
    auto o = builder.CreateVector(outputs);

    auto name1 = builder.CreateString("wow1");

    auto node1 = CreateFlatNode(builder, 1, name1, OpType_TRANSFORM, 0, in1, 0, nd4j::graph::DataType::DataType_FLOAT);

    std::vector<flatbuffers::Offset<FlatVariable>> variables_vector;
    variables_vector.push_back(fXVar);
    variables_vector.push_back(fYVar);

    std::vector<flatbuffers::Offset<FlatNode>> nodes_vector;
    nodes_vector.push_back(node1);



    auto nodes = builder.CreateVector(nodes_vector);
    auto variables = builder.CreateVector(variables_vector);

    FlatGraphBuilder graphBuilder(builder);

    graphBuilder.add_variables(variables);
    graphBuilder.add_id(119);
    graphBuilder.add_nodes(nodes);
    graphBuilder.add_outputs(o);


    auto flatGraph = graphBuilder.Finish();
    builder.Finish(flatGraph);

    auto restoredGraph = new Graph<float>(GetFlatGraph(builder.GetBufferPointer()));

    GraphExecutioner<float>::execute(restoredGraph);

    auto results = restoredGraph->fetchOutputs();

    // IMPLICIT is default
    ASSERT_EQ(1, results->size());

    //ASSERT_NEAR(-2.0, results->at(0)->getNDArray()->reduceNumber<simdOps::Mean<float>>(), 1e-5);
    //ASSERT_NEAR(-1.0, results->at(1)->getNDArray()->reduceNumber<simdOps::Mean<float>>(), 1e-5);
    ASSERT_NEAR(-3.0, results->at(0)->getNDArray()->reduceNumber<simdOps::Mean<float>>(), 1e-5);

    //ASSERT_EQ(-1, results->at(0)->id());
    //ASSERT_EQ(-2, results->at(1)->id());

    delete restoredGraph;
    delete results;
    delete x;
    delete y;
}
*/

/*
TEST_F(FlatBuffersTest, ReadFile1) {

    uint8_t* data = nd4j::graph::readFlatBuffers("./resources/adam_sum.fb");

    auto fg = GetFlatGraph(data);
    auto restoredGraph = new Graph<float>(fg);

    ASSERT_EQ(1, restoredGraph->rootNodes());
    ASSERT_EQ(2, restoredGraph->totalNodes());

    auto ones = restoredGraph->getVariableSpace()->getVariable(-1)->getNDArray();

    ASSERT_EQ(4, ones->lengthOf());
    ASSERT_NEAR(4.0f, ones->template reduceNumber<simdOps::Sum<float>>(), 1e-5);

    Nd4jStatus status = GraphExecutioner<float>::execute(restoredGraph);
    ASSERT_EQ(ND4J_STATUS_OK, status);

    auto result = restoredGraph->getVariableSpace()->getVariable(2)->getNDArray();
    ASSERT_EQ(1, result->lengthOf());
    ASSERT_EQ(8, result->getScalar(0));

    delete[] data;
    delete restoredGraph;
}

TEST_F(FlatBuffersTest, ReadFile2) {
    uint8_t* data = nd4j::graph::readFlatBuffers("./resources/adam_sum.fb");
    Nd4jPointer result = GraphExecutioner<float>::executeFlatBuffer((Nd4jPointer) data);

    ResultSet<float> arrays(GetFlatResult(result));

    ASSERT_EQ(1, arrays.size());
    ASSERT_EQ(1, arrays.at(0)->lengthOf());
    ASSERT_EQ(8, arrays.at(0)->getScalar(0));

    delete[] data;
    delete[] (char *) result;
}

TEST_F(FlatBuffersTest, ReadFile3) {
    auto graph = GraphExecutioner<float>::importFromFlatBuffers("./resources/adam_sum.fb");
    Nd4jStatus status = GraphExecutioner<float>::execute(graph);

    ASSERT_EQ(ND4J_STATUS_OK, status);

    auto z = graph->getVariableSpace()->getVariable(2)->getNDArray();

    ASSERT_EQ(1, z->lengthOf());
    ASSERT_EQ(8, z->getScalar(0));

    delete graph;
}


TEST_F(FlatBuffersTest, ReadInception1) {
    auto graph = GraphExecutioner<float>::importFromFlatBuffers("./resources/inception.fb");

    Nd4jStatus status = GraphExecutioner<float>::execute(graph);

    ASSERT_EQ(ND4J_STATUS_OK, status);
    ASSERT_TRUE(graph->getVariableSpace()->hasVariable(227));

    auto lastNode = graph->getVariableSpace()->getVariable(227)->getNDArray();

    lastNode->printShapeInfo("Result shape");

    auto argMax = lastNode->argMax();

    //nd4j_printf("Predicted class: %i\n", (int) argMax);
    //nd4j_printf("Probability: %f\n", lastNode->getScalar(argMax));
    //nd4j_printf("Probability ipod: %f\n", lastNode->getScalar(980));
    //lastNode->printBuffer("Whole output");

    ASSERT_EQ(561, (int) argMax);

    delete graph;
}

TEST_F(FlatBuffersTest, ReadLoops_3argsWhile_1) {
    // TF graph:
    // https://gist.github.com/raver119/b86ef727e9a094aab386e2b35e878966
    auto graph = GraphExecutioner<float>::importFromFlatBuffers("./resources/three_args_while.fb");

    ASSERT_TRUE(graph != nullptr);

    //graph->printOut();

    NDArray<float> expPhi('c', {2, 2});

    ASSERT_TRUE(graph->getVariableSpace()->hasVariable(-1));
    ASSERT_TRUE(graph->getVariableSpace()->hasVariable(-2));

    auto phi = graph->getVariableSpace()->getVariable(-2)->getNDArray();
    auto constA = graph->getVariableSpace()->getVariable(-5)->getNDArray();
    auto lessY = graph->getVariableSpace()->getVariable(-6)->getNDArray();

    //constA->printBuffer("constA");
    //lessY->printBuffer("lessY");

    ASSERT_TRUE(expPhi.isSameShape(phi));

    Nd4jStatus status = GraphExecutioner<float>::execute(graph);

    ASSERT_EQ(ND4J_STATUS_OK, status);

    // now, we expect some values

    auto x = graph->getVariableSpace()->getVariable(20);
    auto y = graph->getVariableSpace()->getVariable(21);

    ASSERT_NEAR(110.0f, x->getNDArray()->meanNumber(), 1e-5);
    ASSERT_NEAR(33.0f, y->getNDArray()->meanNumber(), 1e-5);

    delete graph;
}



TEST_F(FlatBuffersTest, ReadTensorArrayLoop_1) {
    NDArray<float> exp('c', {5, 2}, {3., 6., 9., 12., 15., 18., 21., 24., 27., 30.});
    auto graph = GraphExecutioner<float>::importFromFlatBuffers("./resources/tensor_array_loop.fb");

    ASSERT_TRUE(graph != nullptr);

    //graph->printOut();

    Nd4jStatus status = GraphExecutioner<float>::execute(graph);

    ASSERT_EQ(ND4J_STATUS_OK, status);

    auto variableSpace = graph->getVariableSpace();

    ASSERT_TRUE(variableSpace->hasVariable(23,0));

    auto z = variableSpace->getVariable(23)->getNDArray();

    //z->printShapeInfo("z shape");
    //z->printIndexedBuffer("z buffer");

    ASSERT_TRUE(exp.isSameShape(z));
    ASSERT_TRUE(exp.equalsTo(z));

    delete graph;
}

*/

/*
TEST_F(FlatBuffersTest, ReadLoops_NestedWhile_1) {
    // TF graph:
    // https://gist.github.com/raver119/2aa49daf7ec09ed4ddddbc6262f213a0
    nd4j::ops::assign<float> op1;

    auto graph = GraphExecutioner<float>::importFromFlatBuffers("./resources/nested_while.fb");

    ASSERT_TRUE(graph != nullptr);

    Nd4jStatus status = GraphExecutioner<float>::execute(graph);

    ASSERT_EQ(ND4J_STATUS_OK, status);

    auto x = graph->getVariableSpace()->getVariable(28);
    auto y = graph->getVariableSpace()->getVariable(29);
    auto z = graph->getVariableSpace()->getVariable(11, 2);

    ASSERT_NEAR(110.0f, x->getNDArray()->meanNumber(), 1e-5);
    ASSERT_NEAR(33.0f, y->getNDArray()->meanNumber(), 1e-5);

    // we should have only 3 cycles in nested loop
    ASSERT_NEAR(30.0f, z->getNDArray()->meanNumber(), 1e-5);

    delete graph;
}
*/
/*

TEST_F(FlatBuffersTest, ReadTensorArray_1) {
    // TF graph: https://gist.github.com/raver119/3265923eed48feecc465d17ec842b6e2
    float _expB[] = {1.000000, 1.000000, 2.000000, 2.000000, 3.000000, 3.000000};
    NDArray<float> exp('c', {3, 2});
    exp.setBuffer(_expB);

    auto graph = GraphExecutioner<float>::importFromFlatBuffers("./resources/tensor_array.fb");

    ASSERT_TRUE(graph != nullptr);

    Nd4jStatus status = GraphExecutioner<float>::execute(graph);

    ASSERT_EQ(ND4J_STATUS_OK, status);

    ASSERT_TRUE(graph->getVariableSpace()->hasVariable(14));

    auto z = graph->getVariableSpace()->getVariable(14)->getNDArray();

    ASSERT_TRUE(exp.isSameShape(z));
    ASSERT_TRUE(exp.equalsTo(z));

    delete graph;
}

*/
/*
TEST_F(FlatBuffersTest, ReadStridedSlice_1) {
    // TF graph: https://gist.github.com/raver119/fc3bf2d31c91e465c635b24020fd798d
    auto graph = GraphExecutioner<float>::importFromFlatBuffers("./resources/tensor_slice.fb");

    ASSERT_TRUE(graph != nullptr);

    Nd4jStatus status = GraphExecutioner<float>::execute(graph);

    ASSERT_EQ(ND4J_STATUS_OK, status);

    ASSERT_TRUE(graph->getVariableSpace()->hasVariable(7));

    auto z = graph->getVariableSpace()->getVariable(7)->getNDArray();

    ASSERT_NEAR(73.5f, z->getScalar(0), 1e-5);

    delete graph;
}
*/


TEST_F(FlatBuffersTest, ReduceDim_1) {
    NDArray<float> exp('c', {3, 1});
    exp.assign(3.0);


    auto graph = GraphExecutioner<float>::importFromFlatBuffers("./resources/reduce_dim.fb");
    auto variableSpace = graph->getVariableSpace();


    ASSERT_TRUE(variableSpace->hasVariable(1));
    ASSERT_TRUE(variableSpace->hasVariable(2));

    auto x = variableSpace->getVariable(1)->getNDArray();
    auto y = variableSpace->getVariable(2)->getNDArray();

    x->printShapeInfo("x shape");

    Nd4jStatus status = GraphExecutioner<float>::execute(graph);

    ASSERT_EQ(ND4J_STATUS_OK, status);

    ASSERT_TRUE(variableSpace->hasVariable(3));

    auto result = variableSpace->getVariable(3)->getNDArray();

    ASSERT_TRUE(exp.isSameShape(result));
    ASSERT_TRUE(exp.equalsTo(result));

    delete graph;
}

TEST_F(FlatBuffersTest, Ae_00) {
    nd4j::ops::rank<float> op1;

    auto graph = GraphExecutioner<float>::importFromFlatBuffers("./resources/ae_00.fb");

    graph->printOut();
/*
    auto result = GraphExecutioner<float>::execute(graph);
    ASSERT_EQ(ND4J_STATUS_OK, result);
*/
    delete graph;
}


TEST_F(FlatBuffersTest, transpose) {
    nd4j::ops::rank<float> op1;

    auto graph = GraphExecutioner<float>::importFromFlatBuffers("./resources/transpose.fb");

    graph->printOut();

    //auto result = GraphExecutioner<float>::execute(graph);
    //ASSERT_EQ(ND4J_STATUS_OK, result);

    delete graph;
}


/*
TEST_F(FlatBuffersTest, ReadLoops_SimpleWhile_1) {
    // TF graph:
    // https://gist.github.com/raver119/2aa49daf7ec09ed4ddddbc6262f213a0
    auto graph = GraphExecutioner<float>::importFromFlatBuffers("./resources/simple_while.fb");

    ASSERT_TRUE(graph != nullptr);

    Nd4jStatus status = GraphExecutioner<float>::execute(graph);

    ASSERT_EQ(ND4J_STATUS_OK, status);

    delete graph;
}

 */