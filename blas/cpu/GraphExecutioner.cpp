//
// @author raver119@gmail.com
//

#include <graph/generated/node_generated.h>
#include <graph/generated/graph_generated.h>
#include <graph/generated/result_generated.h>

//#include <protobuf/core/framework/graph.pb.h>

#include <Variable.h>
#include <VariableSpace.h>
#include <Node.h>
#include <Scope.h>
#include <GraphExecutioner.h>
#include <graph/TimeHolder.h>
#include <loops/scalar.h>
#include <loops/pairwise_transform.h>
#include <loops/transform.h>
#include <ops/declarable/DeclarableOp.h>

//#include <google/protobuf/text_format.h>
//#include <google/protobuf/io/zero_copy_stream_impl.h>

#include <fcntl.h>

#include <chrono>
#include <ctime>
#include <graph/execution/LogicExecutor.h>
#include <array/DataTypeUtils.h>
#include <helpers/BitwiseUtils.h>
#include <generated/array_generated.h>

namespace nd4j{
namespace graph {

/**
 * This method executes given Node (as in Op within Node)
 *
 * Basically it just does DeclarableOp::execute(Block<T>), and ops to their job. However, there are some additional functionality.
 *
 * @param graph - Graph instance pointer
 * @param node - Node instance pointer, which will be executed
 * @param variableSpace - VariableSpace instance pointer - varspace specific to current Thread/Session
 * @return
 */
template <typename T>
 Nd4jStatus GraphExecutioner<T>::executeFlatNode(Graph<T> *graph, Node<T> *node, VariableSpace<T> *variableSpace) {
    OpType opType = node->opType();
    int opNum = node->opNum();

    if (opType == OpType_BOOLEAN) {
        nd4j_debug("Executing boolean graph node_%i", node->id());
    } else if (opType == OpType_LOGIC) {
        nd4j_debug("Executing logic graph node_%i", node->id());
    } else if (opType == OpType_GRAPH) {
        nd4j_debug("Executing embedded graph node_%i", node->id());
    } else if (opType != OpType_CUSTOM) {
        nd4j_debug("Executing node_%i{%i}\n", node->id(), opNum);
    } else {
        nd4j_debug("Executing node_%i{%s}\n", node->id(), node->getCustomOp()->getOpName()->c_str());
    }

    Context<T> context(node->getContextPrototype(), variableSpace);

    if (nd4j::Environment::getInstance()->isDebugAndVerbose()) {
        //nd4j_debug("Input variables: %i\n", node->input()->size());
        printf("       Inputs: {");
        for (int e = 0; e < node->input()->size(); e++) {
            printf("[%i:%i]", node->input()->at(e).first, node->input()->at(e).second);

            if (e < node->input()->size() - 1)
                printf(", ");
        }
        printf("}\n");
        fflush(stdout);
    }

    if (node->id() == 13)
        nd4j_debug("","");

    // if true - this is special case: Graph-in-Graph. 
    if (node->hasGraphEmbedded()) {
        auto embedded = node->getGraph();

        /**
         * basically, we should do following things here:
         * 1) fill embedded graph with input variables from this graph, if anything should be filled in
         * 2) invoke embedded graph
         * 3) announce its results as corresponding output variables in current VariableSpace
         */

        // enforcing IMPLICIT mode. or not... should we try to be smarter then user?
        //embedded->getExecutorConfiguration()->_outputMode = OutputMode_IMPLICIT;

        if (node->input()->size() != embedded->numberOfPlaceholders()) {
            nd4j_debug("Placeholders amount mismatch: %i expected, and %i available\n",node->input()->size(), embedded->numberOfPlaceholders());
            return ND4J_STATUS_BAD_INPUT;
        }

        // we need to propagate required variables to the embedded graph
        ResultSet<T> deletables;
        int cnt = 0;
        for (Variable<T>* v: *embedded->getPlaceholders()) {
            if (v->getName() != nullptr && v->getName()->size() > 0) {
                
                // trying symbolic lookup first
                if (variableSpace->hasVariable(v->getName())) {
                    // symbolic feeder
                    auto array = variableSpace->getVariable(v->getName())->getNDArray();
                    auto vr = array->dup();
//                    deletables.push_back(vr);
                    v->setNDArray(vr);
                } else {
                    nd4j_debug("Can't find variable [%s] in parent graph...", v->getName()->c_str());
                    return ND4J_STATUS_BAD_INPUT;
                    //throw "Can't find desired variable";
                }
            } else {
                // if we're not using symbolic lookup - we'll use sequential approach then
                auto p = node->input()->at(cnt);
                auto array = variableSpace->getVariable(p)->getNDArray();
                auto vr = array->dup();
                //deletables.push_back(vr);
                v->setNDArray(vr);
            }

            cnt++;
        }

        // executing embedded graph as independent one
        Nd4jStatus status = GraphExecutioner<T>::execute(embedded);
        if (status != ND4J_STATUS_OK)
            return status;

        //  now we should migrate its results to this node, as its own outputs
        cnt = 0;
        auto  outputs = embedded->fetchOutputs();

        for (auto v: *outputs){
            NDArray<T> *array = v->getNDArray();
            v->setNDArray(nullptr);

            std::pair<int,int> pair(node->id(), cnt++);

            auto var = variableSpace->getVariable(pair);

            //nd4j_printf("HasArray: [%i]; Removable: [%i]\n", var->hasNDArray(), var->isRemovable());
            var->setNDArray(array);
            var->markRemovable(true);
        }
        deletables.size();
        delete outputs;
        nd4j_debug("Embedded graph execution finished. %i variable(s) migrated\n", cnt);

    } else if (node->hasCustomOp()) {
        // if we have something to execute - lets just execute it.
        auto status = node->getCustomOp()->execute(&context);
        if (status != ND4J_STATUS_OK)
            return status;

        // propagate variables
        if (node->hasExternalOutputs()) {
            for (auto v: *node->output()) {
                if (variableSpace->hasExternalVariable(v.first)) {
                    variableSpace->getVariable(v.first)->getNDArray()->assign(variableSpace->getVariable(node->id())->getNDArray());
                }
            }
        }

        return status;
    }
    return ND4J_STATUS_OK;
}


/**
 * This method executes given Graph instance, and returns error code.
 *
 * @param graph
 * @return one of error codes defined in pointercast.h
 */
template <typename T>
Nd4jStatus GraphExecutioner<T>::execute(Graph<T> *graph) {
    graph->buildGraph();
    auto __variableSpace = graph->getVariableSpace();

    bool tempFlow = false;
    if (__variableSpace->flowPath() == nullptr) {
        tempFlow = true;
        __variableSpace->setFlowPath(new FlowPath());
    }
    auto flowPath = __variableSpace->flowPath();

    bool pe = graph->getExecutorConfiguration()->_executionMode == ExecutionMode_AUTO;

    // TODO: add code divergence support here
    // basically if at some point code diverges, code branch might be _DISABLED_, and all nodes within that branch will be disabled as well

    // we loop through op layers here
    for (int l = 0; l < (int) graph->getOnion()->size(); l++) {
        int layerSize = graph->getOnion()->count(l) == 1 ? graph->getOnion()->at(l)->size() : 0;

//#pragma omp parallel for if (layerSize > 1 && pe) schedule(dynamic) proc_bind(spread)
        for (int n = 0; n < layerSize; n++) {
            Node<T>* node = graph->getOnion()->at(l)->at(n);

            /**
             * If this LOGIC op, we'll use another execution model here
             */
            if (node->opType() == OpType_LOGIC) {
                auto status = LogicExecutor<T>::processNode(graph, node);

                if (status == ND4J_STATUS_OK)
                    continue;
                else
                    return status;
            }

            bool shouldSkip = false;
            // let's check for input nodes, if they are disabled or contain divergents
            for (int e = 0; e < node->input()->size(); e++) {
                auto inputId = node->input()->at(e);

                // we're skipping external variables here
                if (inputId.first < 0 || __variableSpace->hasExternalVariable(inputId.first))
                    continue;

                /**
                 * We can skip current node, in two cases:
                 * 1) If previous node was disabled
                 * 2) If previous node was divergent node (i.e. IF op) and code went other way
                 */
                Node<T>* prevNode = graph->getMapped()->at(inputId.first);
                if (!flowPath->isActive(inputId.first)) {
                    shouldSkip = true;
                    //node->setActive(false);
                    flowPath->markActive(node->id(), false);

                } else if (prevNode->isDivergencePoint()) {
                    if (flowPath->branch(inputId.first) != inputId.second) {
                        shouldSkip = true;
                        //node->setActive(false);
                        flowPath->markActive(node->id(), false);
                    }
                }
            }

            if (shouldSkip)
                continue;

            auto timeStart = std::chrono::system_clock::now();

            // actual node execution happens right here
            Nd4jStatus status = executeFlatNode(graph, node, __variableSpace);

            auto timeEnd = std::chrono::system_clock::now();

            auto outerTime = std::chrono::duration_cast<std::chrono::microseconds> (timeEnd - timeStart).count();


            flowPath->setOuterTime(node->id(), outerTime);

            if (status != ND4J_STATUS_OK)
                return status;


            // here we should handle divergent ops, and disable nodes accordingly
            if (node->isDivergencePoint()) {
                auto activeBranch = flowPath->branch(node->id());
                nd4j_debug("Active branch at node [%i]: %i\n", node->id(), activeBranch);

                // now we skip all branches except of this active one
            }

            if (nd4j::Environment::getInstance()->isDebugAndVerbose()) {
                auto array = __variableSpace->getVariable(node->id())->getNDArray();
                auto list = __variableSpace->getVariable(node->id())->getNDArrayList();
                if (array != nullptr) {
                    nd4j_debug("node_%i finished. result length: [%i]; meanNumber: [%f]\n", node->id(), (int) array->lengthOf(), array->meanNumber());
                } else if (list != nullptr) {
                    nd4j_debug("node_% is ListOp, skipping evaluation", node->id());
                }
            }
        }
    }

    if (tempFlow)
        delete flowPath;

    return ND4J_STATUS_OK;
}

/**
 * This method is provided for IPC: 
 * 1) it accepts pointer to FlatBuffers buffer
 * 2) restores Graph from it
 * 3) Executes this Graph
 * 4) Packs execution results into FlatBuffers (FlatResults instance)
 * 5) Returns pointer to FlatBuffer results buffer
 *
 */
template <typename T>
Nd4jPointer GraphExecutioner<T>::executeFlatBuffer(Nd4jPointer pointer) {
    uint8_t *buffer = reinterpret_cast<uint8_t *>(pointer);

    nd4j_debug("Trying to restore graph\n", 0);

    auto restoredGraph = GetFlatGraph(buffer);

    nd4j_debug("Graph restored\n", 0);

    // converting FlatGraph to internal representation
    auto nativeGraph = new Graph<T>(restoredGraph);

    FlowPath flowPath;
    nativeGraph->getVariableSpace()->setFlowPath(&flowPath);


    nd4j_debug("Going to execute graph\n", 0);

    // executing internal representation
    GraphExecutioner<T>::execute(nativeGraph);

    nd4j_debug("Building output...\n", 0);

    flatbuffers::FlatBufferBuilder builder(1024);

    // fetching time reports
    std::vector<flatbuffers::Offset<FlatTiming>> timings_vector;
    for (int e = 0; e < (int) nativeGraph->getAllNodes()->size(); e++) {
        Node<T> *node = nativeGraph->getAllNodes()->at(e);

        if (node->getContextPrototype() == nullptr)
            continue;

        auto pair = CreateLongPair(builder, flowPath.outerTime(node->id()), flowPath.innerTime(node->id()));
        if (node->getName() != nullptr) {
            auto name = builder.CreateString(node->getName()->c_str());
            auto fr = CreateFlatTiming(builder, node->id(), name, pair);
            timings_vector.push_back(fr);
        } else {
            auto fr = CreateFlatTiming(builder, node->id(), 0, pair);
            timings_vector.push_back(fr);
        }
    }


    // now, we'll prepare output, depending on given outputmode
    auto outputs = nativeGraph->fetchOutputs();
    std::vector<flatbuffers::Offset<FlatVariable>> variables_vector;
    for (int e = 0; e < (int) outputs->size(); e++) {
        auto var = outputs->at(e);

        NDArray<T>* array = var->getNDArray();
        auto byteVector = array->asByteVector();

        auto fBuffer = builder.CreateVector(byteVector);
        auto fShape = builder.CreateVector(array->getShapeInfoAsVector());

        nd4j::graph::ByteOrder bo = (nd4j::graph::ByteOrder) BitwiseUtils::asByteOrder();

        auto fArray = CreateFlatArray(builder, fShape, fBuffer, (nd4j::graph::DataType) DataTypeUtils::fromT<T>(), bo);

        auto fName = builder.CreateString(*(var->getName()));
        auto id = CreateIntPair(builder, var->id(), var->index());

        auto fv = CreateFlatVariable(builder, id, fName, 0, fArray);

        variables_vector.push_back(fv);
    }

    nd4j_debug("Returning %i variables back\n", variables_vector.size());

    auto varTimings = builder.CreateVector(timings_vector);
    auto varVectors = builder.CreateVector(variables_vector);
    auto result = CreateFlatResult(builder, restoredGraph->id(), varVectors, varTimings);
    builder.Finish(result);

    // we might want to keep this graph for future
    delete outputs;
    delete nativeGraph;

    char* res = new char[builder.GetSize()];
    memcpy(res, builder.GetBufferPointer(), builder.GetSize());

    return (Nd4jPointer) res; //builder.GetBufferPointer();
}


template <typename T>
Graph<T>* GraphExecutioner<T>::importFromTensorFlow(const char *fileName) {
    /*
    if (fileName == nullptr)
        return nullptr;

    int fd = open(fileName, O_RDONLY);

    if (fd < 0) {
        nd4j_printf("File not found: [%s]\n", fileName);
        return nullptr;
    }

    nd4j_verbose("Trying to load TF GraphDef from file [%s]\n", fileName);

    tensorflow::GraphDef graphDef;
    bool res = graphDef.ParseFromFileDescriptor(fd);

    // trying to read graph as text
    if(!res) {
        close(fd);
        fd = open(fileName, O_RDONLY);

        google::protobuf::io::FileInputStream fileInput(fd);
        fileInput.SetCloseOnDelete(true);

        if (!google::protobuf::TextFormat::Parse(&fileInput, &graphDef)) {
            nd4j_printf("Failed to read file\n","");
        } else {
            res = true;
        }
    }

    close(fd);

    if (!res)
        return nullptr;

    auto graph = new Graph<T>();
    auto variableSpace = graph->getVariableSpace();

    std::map<const std::string, int> variablesMap;

    int variablesCounter = 0;
    int nodesCounter = 0;
    nd4j_verbose("Number of nodes in graphDef: %i\n", graphDef.node_size());
    for (int n = 0; n < graphDef.node_size(); n++) {
        auto node = graphDef.node(n);

        // if that's external variable - we put it to variable space
        if (strcmp(TF_VAR, node.op().c_str()) == 0 || strcmp(TF_CONST, node.op().c_str()) == 0 || strcmp(TF_INPUT, node.op().c_str()) == 0) {
            nd4j_printf("Variable found: %s\n", node.name().c_str());
            auto variable = new Variable<T>();
            variable->setName(new std::string(node.name().c_str()));
            variable->setId(--variablesCounter);
            variableSpace->putVariable(variable->id(), variable);

            std::pair<const std::string, int> pair(node.name(), variable->id());
            variablesMap.insert(pair);

            // TODO: we might want to have something like that.
            // it basically just gives input validation option, since settles expectations for input
            if (strcmp(TF_INPUT, node.op().c_str()) == 0)
                continue;

            // checking shape, not applicable to input, since it can vary
            if (node.attr().count("shape")) {
                auto attr = node.attr().at("shape");
                int dims = attr.shape().dim_size();

                if (dims > 0) {
                    std::vector<int> __shape;

                    // we don't have rank1 arrays. vector is 2d.
                    if (dims == 1)
                        __shape.push_back(1);

                    // roll through dimensions
                    for (auto s: attr.shape().dim()) {
                        __shape.push_back((int) s.size()) ;
                    }

                    variable->setNDArray(new NDArray<T>('c', __shape));

                    nd4j_printf("Shape found: %i dims;\n", dims);
                    variable->getNDArray()->printShapeInfo();
                }
            }

            // checking tensor attached
            if (node.attr().count("value")) {
                auto attr = node.attr().at("value");

                // int
                if (attr.tensor().dtype() == ::tensorflow::DataType::DT_INT32) {
                    nd4j_verbose("Int size: %i\n", attr.tensor().int_val_size());

                    Nd4jIndex __length = 0;

                    nd4j_verbose("Tensor has shape: %i\n", attr.tensor().has_tensor_shape());
                    if (attr.tensor().has_tensor_shape()) {
                        auto shape = attr.tensor().tensor_shape();
                        int dims = shape.dim_size();

                        if (dims > 0) {
                            std::vector<int> __shape;
                            // we don't have rank1 arrays. vector is 2d.
                            if (dims == 1)
                                __shape.push_back(1);

                            // roll through dimensions
                            for (auto s: shape.dim()) {
                                __shape.push_back((int) s.size());
                            }

                            variable->setNDArray(new NDArray<T>('c', __shape));
                            __length = variable->getNDArray()->lengthOf();

                            nd4j_printf("Tensor shape found: %i dims;\n", dims);
                            variable->getNDArray()->printShapeInfo();
                        }
                    }

                    // it can be valueOf array
                    if (attr.tensor().int_val_size() == 1 && __length > 0) {
                        variable->getNDArray()->assign((T) attr.tensor().int_val(0));
                    }
                }
            }
        } else {
            nd4j_verbose("Node id: [%i]; name: [%s]; opName: [%s]\n", n + 1, node.name().c_str(),
                         node.op().c_str());

            nd4j::ops::DeclarableOp<T> *op = nd4j::ops::OpRegistrator::getInstance()->getOperationFloat(node.op().c_str());

            if (op == nullptr) {
                nd4j_verbose("Op wasn't found: %s\n", node.op().c_str());
                return nullptr;
            }

            auto jNode = new Node<T>();
            jNode->setName(node.name());
            jNode->setId(++nodesCounter);
            jNode->setCustomOp(op);
            jNode->setBlock(new Block<T>(jNode->id(), variableSpace));

            std::pair<const std::string, int> pair(node.name(), jNode->id());
            variablesMap.insert(pair);

            // multi-output nodes require special treatment
            for (int e = 0; e < op->getOpDescriptor()->getNumberOfOutputs(); e++) {
                std::string deepName(node.name());
                deepName += ":" + std::to_string(e);
                auto deepVar = new Variable<T>();
                deepVar->setName(&deepName);

                if (e > 0)
                    deepVar->setId(--variablesCounter);
                else
                    deepVar->setId(jNode->id());

                std::pair<const std::string, int> pair(deepName, deepVar->id());
                variablesMap.insert(pair);

                variableSpace->putVariable(deepVar->id(), deepVar);

                std::pair<int, int> nodepair(jNode->id(), e);
                variableSpace->putVariable(nodepair, deepVar);
            }


            printf("             Inputs: [");
            for (int i = 0; i < node.input_size(); i++) {
                nd4j_printf("Trying input: %s\n", node.input(i).c_str());

                // if this fails - we're probably on partial input :)
                if (!variablesMap.count(node.input(i)))
                    return nullptr;

                printf("%s (%i)", node.input(i).c_str(), variablesMap.at(node.input(i)));


                jNode->pickInput(variablesMap.at(node.input(i)));
                jNode->getBlock()->pickInput(variablesMap.at(node.input(i)));


                if (i < node.input_size() + 1)
                    printf(", ");
            }
            printf("]\n");

            graph->addNode(jNode);
        }
    }

    return graph;
     */
    return nullptr;
}

/**
*   This function returns file size for the given file name, or -1 if something went wrong
*/
long getFileSize(const char * filename) {
    struct stat stat_buf;
    int rc = stat(filename, &stat_buf);
    return rc == 0 ? stat_buf.st_size : -1;
}

/**
*   Helper function, that loads given filename into uint8_t array
*
*/
uint8_t* readFlatBuffers(const char * filename) {
    long fileLen = getFileSize(filename);
    if (fileLen < 0) {
        nd4j_printf("File [%s] wasn't found. Please check path and permissions\n", filename);
        throw "File not found";
    }

    nd4j_debug("File length: %i\n", fileLen);

    uint8_t * data = new uint8_t[fileLen];

    FILE *in = fopen(filename, "rb");
    int cnt = 0;

    while (cnt < fileLen) {
        fread(data + cnt, 1, 1, in);

        cnt++;
    }
    fclose(in);

    return data;
}


/**
*   This method reads given FlatBuffers file, and returns Graph instance
*
*   PLEASE NOTE: This method is mostly suited for tests and debugging/profiling
*/
template <typename T>
Graph<T>* GraphExecutioner<T>::importFromFlatBuffers(const char *filename) {
    uint8_t* data = readFlatBuffers(filename);

    auto fg = GetFlatGraph(data);
    auto restoredGraph = new Graph<T>(fg);

    delete[] data;
    
    return restoredGraph;
}


        template class ND4J_EXPORT GraphExecutioner<float>;
        template class ND4J_EXPORT GraphExecutioner<float16>;
        template class ND4J_EXPORT GraphExecutioner<double>;
    }
}