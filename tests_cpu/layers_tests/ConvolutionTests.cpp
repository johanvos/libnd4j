//
// @author raver119@gmail.com
//

#ifndef LIBND4J_CONVOLUTIONTESTS_H
#define LIBND4J_CONVOLUTIONTESTS_H

#include "testlayers.h"
#include <NDArray.h>
#include <Context.h>
#include <Node.h>
#include <graph/Variable.h>
#include <graph/VariableSpace.h>
#include <NDArrayFactory.h>
#include <ops/declarable/CustomOperations.h>
#include <ops/declarable/generic/helpers/convolutions.h>

using namespace nd4j;
using namespace nd4j::graph;

class ConvolutionTests : public testing::Test {
public:

};

TEST_F(ConvolutionTests, TestConv2D_1) {
    double _expB[]{664.0, 700.0, 736.0, 344.0, 808.0, 844.0, 880.0, 408.0, 952.0, 988.0, 1024.0, 472.0, 1096.0, 1132.0, 1168.0, 536.0, 466.0, 480.0, 494.0, 220.0, 1528.0, 1628.0, 1728.0, 856.0, 1928.0, 2028.0, 2128.0, 1048.0, 2328.0, 2428.0, 2528.0, 1240.0, 2728.0, 2828.0, 2928.0, 1432.0, 1346.0, 1392.0, 1438.0, 700.0, 2392.0, 2556.0, 2720.0, 1368.0, 3048.0, 3212.0, 3376.0, 1688.0, 3704.0, 3868.0, 4032.0, 2008.0, 4360.0, 4524.0, 4688.0, 2328.0, 2226.0, 2304.0, 2382.0, 1180.0};
    int _expS[]{4, 1, 3, 5, 4, 60, 20, 4, 1, 0, 1, 99};
    auto input = new NDArray<double>('c', {1, 2, 5, 4});
    auto weights = new NDArray<double> ('c', {3, 2, 2, 2});

    for (int e = 0; e < input->lengthOf(); e++)
        input->putScalar(e, e + 1);

    for (int e = 0; e < weights->lengthOf(); e++)
        weights->putScalar(e, e + 1);

    auto exp = new NDArray<double>(_expB, _expS);
    exp->triggerAllocationFlag(false, false);

    auto variableSpace = new VariableSpace<double>();
    variableSpace->putVariable(-1, input);
    variableSpace->putVariable(-2, weights);

    auto block = new Context<double>(1, variableSpace, false);  // not-in-place
    block->fillInputs({-1, -2});

    // 5,5 kernel
    block->getIArguments()->push_back(2);
    block->getIArguments()->push_back(2);

    // 1,1 stride
    block->getIArguments()->push_back(1);
    block->getIArguments()->push_back(1);

    // 0,0 padding
    block->getIArguments()->push_back(0);
    block->getIArguments()->push_back(0);

    // 1,1 dilation
    block->getIArguments()->push_back(1);
    block->getIArguments()->push_back(1);

    // same mode
    block->getIArguments()->push_back(1);

    nd4j::ops::conv2d<double> op;

    Nd4jStatus status = op.execute(block);
    ASSERT_EQ(ND4J_STATUS_OK, status);

    auto res = variableSpace->getVariable(1)->getNDArray();


    // checking output shape
    ASSERT_EQ(1, res->sizeAt(0));
    ASSERT_EQ(3, res->sizeAt(1));
    ASSERT_EQ(5, res->sizeAt(2));
    ASSERT_EQ(4, res->sizeAt(3));

    // basically the same as above
    ASSERT_TRUE(res->isSameShape(exp));

    // just for visual validation
    //exp->printBuffer("Expected");
    //res->printBuffer("Actual  ");
    //res->printShapeInfo("Result shape");

    // final check
    ASSERT_TRUE(res->equalsTo(exp));

    delete block;
    delete variableSpace;
    delete exp;
}


TEST_F(ConvolutionTests, TestAvgFF1) {
    auto input = new NDArray<float>('c', {4, 2, 1, 11, 11});

    input->assign(451.0);

    auto output = new NDArray<float>('c', {4, 2, 1, 10, 10});


    std::pair<int, int> pair0(1,0);
    std::pair<int, int> pair1(1,1);


    VariableSpace<float>* variableSpace = new VariableSpace<float>();
    variableSpace->putVariable(-1, input);

    variableSpace->putVariable(pair0, output);

    Context<float>* block = new Context<float>(1, variableSpace, false);  // not-in-place
    block->fillInputs({-1});

    // kernel params
    block->getIArguments()->push_back(1);
    block->getIArguments()->push_back(2);
    block->getIArguments()->push_back(2);

    // stride
    block->getIArguments()->push_back(1);
    block->getIArguments()->push_back(1);
    block->getIArguments()->push_back(1);

    // padding
    block->getIArguments()->push_back(0);
    block->getIArguments()->push_back(0);
    block->getIArguments()->push_back(0);

    // ceiling
    block->getIArguments()->push_back(1);

    // padding count
    block->getIArguments()->push_back(1);



    nd4j::ops::avgpool3d<float> avgpool3d;

    Nd4jStatus result = avgpool3d.execute(block);
    ASSERT_EQ(ND4J_STATUS_OK, result);

    //output.printBuffer("Result");

    ASSERT_NEAR(451.0f, output->template reduceNumber<simdOps::Mean<float>>(), 1e-5);

    delete variableSpace;
    delete block;
}


TEST_F(ConvolutionTests, TestFullConv3D_1) {
    auto input = new NDArray<float>('c', {4, 3, 3, 56, 56});
    auto weights = new NDArray<float>('f', {2, 3, 3, 5, 5});
    auto bias = new NDArray<float>('c', {1, 2});

    input->assign(1.0);
    weights->assign(2.0);
    bias->putScalar(0, 1.0f);
    bias->putScalar(1, 1.0f);

    auto output = new NDArray<float>('c', {4, 2, 1, 11, 11});

    VariableSpace<float>* variableSpace = new VariableSpace<float>();
    variableSpace->putVariable(-1, input);
    variableSpace->putVariable(-2, weights);
    variableSpace->putVariable(-3, bias);

    variableSpace->putVariable(1, output);
    Context<float>* block = new Context<float>(1, variableSpace, false);  // not-in-place
    block->fillInputs({-1, -2, -3});

    block->getIArguments()->push_back(1);
    block->getIArguments()->push_back(2);
    block->getIArguments()->push_back(5);
    block->getIArguments()->push_back(5);
    block->getIArguments()->push_back(0);
    block->getIArguments()->push_back(0);
    block->getIArguments()->push_back(0);

    nd4j::ops::conv3d<float> conv3d;


    delete variableSpace;
    delete block;
}


TEST_F(ConvolutionTests, SeparableConv2D_FF_NoBias_1) {
    float _expB[] = {10025.0f,    10350.0f,    10675.0f,    11000.0f,    11325.0f,    11650.0f,    13275.0f,    13600.0f,    13925.0f,    14250.0f,    14575.0f,    14900.0f,    16525.0f,    16850.0f,    17175.0f,    17500.0f,    17825.0f,    18150.0f,    19775.0f,    20100.0f,    20425.0f,    20750.0f,    21075.0f,    21400.0f,    23025.0f,    23350.0f,    23675.0f,    24000.0f,    24325.0f,    24650.0f,    26275.0f,    26600.0f,    26925.0f,    27250.0f,    27575.0f,    27900.0f,    38775.0f,    40350.0f,    41925.0f,    43500.0f,    45075.0f,    46650.0f,    54525.0f,    56100.0f,    57675.0f,    59250.0f,    60825.0f,    62400.0f,    70275.0f,    71850.0f,    73425.0f,    75000.0f,    76575.0f,    78150.0f,    86025.0f,    87600.0f,    89175.0f,    90750.0f,    92325.0f,    93900.0f,   101775.0f,   103350.0f,   104925.0f,    106500.0f,   108075.0f,   109650.0f,   117525.0f,   119100.0f,   120675.0f,   122250.0f,    123825.0f,   125400.0f,    67525.0f,    70350.0f,    73175.0f,    76000.0f,    78825.0f,    81650.0f,    95775.0f,    98600.0f,   101425.0f,   104250.0f,   107075.0f,   109900.0f,    124025.0f,   126850.0f,   129675.0f,   132500.0f,   135325.0f,   138150.0f,   152275.0f,    155100.0f,   157925.0f,   160750.0f,   163575.0f,   166400.0f,   180525.0f,   183350.0f,    186175.0f,   189000.0f,   191825.0f,   194650.0f,   208775.0f,   211600.0f,   214425.0f,    217250.0f,   220075.0f,   222900.0f,   119400.0f,   120350.0f,   121300.0f,   122250.0f,    123200.0f,   124150.0f,   128900.0f,   129850.0f,   130800.0f,   131750.0f,   132700.0f,    133650.0f,   138400.0f,   139350.0f,   140300.0f,   141250.0f,   142200.0f,   143150.0f,    147900.0f,   148850.0f,   149800.0f,   150750.0f,   151700.0f,   152650.0f,   157400.0f,    158350.0f,   159300.0f,   160250.0f,   161200.0f,   162150.0f,   166900.0f,   167850.0f,    168800.0f,   169750.0f,   170700.0f,   171650.0f,   273150.0f,   275350.0f,   277550.0f,    279750.0f,   281950.0f,   284150.0f,   295150.0f,   297350.0f,   299550.0f,   301750.0f,    303950.0f,   306150.0f,   317150.0f,   319350.0f,   321550.0f,   323750.0f,   325950.0f,    328150.0f,   339150.0f,   341350.0f,   343550.0f,   345750.0f,   347950.0f,   350150.0f,    361150.0f,   363350.0f,   365550.0f,   367750.0f,   369950.0f,   372150.0f,   383150.0f,    385350.0f,   387550.0f,   389750.0f,   391950.0f,   394150.0f,   426900.0f,   430350.0f,    433800.0f,   437250.0f,   440700.0f,   444150.0f,   461400.0f,   464850.0f,   468300.0f,    471750.0f,   475200.0f,   478650.0f,   495900.0f,   499350.0f,   502800.0f,   506250.0f,    509700.0f,   513150.0f,   530400.0f,   533850.0f,   537300.0f,   540750.0f,   544200.0f,    547650.0f,   564900.0f,   568350.0f,   571800.0f,   575250.0f,   578700.0f,   582150.0f,    599400.0f,   602850.0f,   606300.0f,   609750.0f,   613200.0f,   616650.0f,    75025.0f,    75350.0f,    75675.0f,    76000.0f,    76325.0f,    76650.0f,    78275.0f,    78600.0f,    78925.0f,    79250.0f,    79575.0f,    79900.0f,    81525.0f,    81850.0f,    82175.0f,    82500.0f,    82825.0f,    83150.0f,    84775.0f,    85100.0f,    85425.0f,    85750.0f,    86075.0f,    86400.0f,    88025.0f,    88350.0f,    88675.0f,    89000.0f,    89325.0f,    89650.0f,    91275.0f,    91600.0f,    91925.0f,    92250.0f,    92575.0f,    92900.0f,    353775.0f,   355350.0f,   356925.0f,   358500.0f,   360075.0f,   361650.0f,   369525.0f,    371100.0f,   372675.0f,   374250.0f,   375825.0f,   377400.0f,   385275.0f,   386850.0f,    388425.0f,   390000.0f,   391575.0f,   393150.0f,   401025.0f,   402600.0f,   404175.0f,    405750.0f,   407325.0f,   408900.0f,   416775.0f,   418350.0f,   419925.0f,   421500.0f,    423075.0f,   424650.0f,   432525.0f,   434100.0f,   435675.0f,   437250.0f,   438825.0f,    440400.0f,   632525.0f,   635350.0f,   638175.0f,   641000.0f,   643825.0f,   646650.0f,    660775.0f,   663600.0f,   666425.0f,   669250.0f,   672075.0f,   674900.0f,   689025.0f,    691850.0f,   694675.0f,   697500.0f,   700325.0f,   703150.0f,   717275.0f,   720100.0f,    722925.0f,   725750.0f,   728575.0f,   731400.0f,   745525.0f,   748350.0f,   751175.0f,    754000.0f,   756825.0f,   759650.0f,   773775.0f,   776600.0f,   779425.0f,   782250.0f,    785075.0f,   787900.0f,   309400.0f,   310350.0f,   311300.0f,   312250.0f,   313200.0f,    314150.0f,   318900.0f,   319850.0f,   320800.0f,   321750.0f,   322700.0f,   323650.0f,    328400.0f,   329350.0f,   330300.0f,   331250.0f,   332200.0f,   333150.0f,   337900.0f,    338850.0f,   339800.0f,   340750.0f,   341700.0f,   342650.0f,   347400.0f,   348350.0f,    349300.0f,   350250.0f,   351200.0f,   352150.0f,   356900.0f,   357850.0f,   358800.0f,    359750.0f,   360700.0f,   361650.0f,   713150.0f,   715350.0f,   717550.0f,   719750.0f,    721950.0f,   724150.0f,   735150.0f,   737350.0f,   739550.0f,   741750.0f,   743950.0f,    746150.0f,   757150.0f,   759350.0f,   761550.0f,   763750.0f,   765950.0f,   768150.0f,    779150.0f,   781350.0f,   783550.0f,   785750.0f,   787950.0f,   790150.0f,   801150.0f,    803350.0f,   805550.0f,   807750.0f,   809950.0f,   812150.0f,   823150.0f,   825350.0f,    827550.0f,   829750.0f,   831950.0f,   834150.0f,  1116900.0f,  1120350.0f,  1123800.0f,    1127250.0f,  1130700.0f,  1134150.0f,  1151400.0f,  1154850.0f,  1158300.0f,  1161750.0f,    1165200.0f,  1168650.0f,  1185900.0f,  1189350.0f,  1192800.0f,  1196250.0f,  1199700.0f,    1203150.0f,  1220400.0f,  1223850.0f,  1227300.0f,  1230750.0f,  1234200.0f,  1237650.0f,    1254900.0f,  1258350.0f,  1261800.0f,  1265250.0f,  1268700.0f,  1272150.0f,  1289400.0f,    1292850.0f,  1296300.0f,  1299750.0f,  1303200.0f,  1306650.0f,};
    int _expS[] = {4, 2, 6, 6, 6, 144, 36, 6, 1, 0, 1, 99};
    NDArray<float> exp(_expB, _expS);
    exp.triggerAllocationFlag(false, false);

    int sY = 1;
    int sX = 1;
    int pY = 0;
    int pX = 0;
    int iC = 2;
    int oC = 3;
    int kY = 5;
    int kX = 5;
    int iY = 10;
    int iX = 10;
    int B = 2;

    auto input = new NDArray<float>('c', {B, iC, iY, iX});
    for (int e = 0; e < input->lengthOf(); e++)
        input->putScalar(e, e+1);

    auto weights = new NDArray<float> ('c', {oC, iC, kY, kX});
    for (int e = 0; e < weights->lengthOf(); e++)
        weights->putScalar(e, e+1);

    auto variableSpace = new VariableSpace<float>();
    variableSpace->putVariable(-1, input);
    variableSpace->putVariable(-2, weights);

    auto block = new Context<float>(1, variableSpace, false);
    block->fillInputs({-1, -2});

    block->getIArguments()->push_back(kY);
    block->getIArguments()->push_back(kX);

    block->getIArguments()->push_back(sY);
    block->getIArguments()->push_back(sX);

    block->getIArguments()->push_back(pY);
    block->getIArguments()->push_back(pX);

    // dilation
    block->getIArguments()->push_back(1);
    block->getIArguments()->push_back(1);

    // NOT same mode
    block->getIArguments()->push_back(0);

    nd4j::ops::sconv2d<float> op;

    Nd4jStatus status = op.execute(block);

    ASSERT_EQ(ND4J_STATUS_OK, status);

    NDArray<float>* output = variableSpace->getVariable(1)->getNDArray();

    //exp.printShapeInfo("Expected shape");
    //output->printShapeInfo("Result shape");
    ASSERT_TRUE(exp.isSameShape(output));

        //exp.printBuffer("Expctd buffer");
    //output->printBuffer("Result buffer");
    ASSERT_TRUE(exp.equalsTo(output));

    delete block;
    delete variableSpace;
}

TEST_F(ConvolutionTests, SeparableConv2D_BP_NoBias_1) {
    int sY = 1;
    int sX = 1;
    int pY = 0;
    int pX = 0;
    int iC = 2;
    int oC = 3;
    int kY = 5;
    int kX = 5;
    int iY = 10;
    int iX = 10;
    int B = 2;
    
    //double _expWB[] = {10658316.0,  10892892.0,  11127468.0,  11362044.0,  11596620.0,  11831196.0,     12065772.0,  12300348.0,  12534924.0,  12769500.0,  13004076.0,  13238652.0,     13473228.0,  13707804.0,  13942380.0,  14176956.0,  14411532.0,  14646108.0,     14880684.0,  15115260.0,  15349836.0,  15584412.0,  15818988.0,  16053564.0,     16288140.0,  25949820.0,  26371020.0,  26792220.0,  27213420.0,  27634620.0,     28055820.0,  28477020.0,  28898220.0,  29319420.0,  29740620.0,  30161820.0,     30583020.0,  31004220.0,  31425420.0,  31846620.0,  32267820.0,  32689020.0,     33110220.0,  33531420.0,  33952620.0,  34373820.0,  34795020.0,  35216220.0,     35637420.0,  36058620.0,  13039068.0,  13366956.0,  13694844.0,  14022732.0,     14350620.0,  14678508.0,  15006396.0,  15334284.0,  15662172.0,  15990060.0,     16317948.0,  16645836.0,  16973724.0,  17301612.0,  17629500.0,  17957388.0,     18285276.0,  18613164.0,  18941052.0,  19268940.0,  19596828.0,  19924716.0,     20252604.0,  20580492.0,  20908380.0,  30663372.0,  31177884.0,  31692396.0,     32206908.0,  32721420.0,  33235932.0,  33750444.0,  34264956.0,  34779468.0,     35293980.0,  35808492.0,  36323004.0,  36837516.0,  37352028.0,  37866540.0,     38381052.0,  38895564.0,  39410076.0,  39924588.0,  40439100.0,  40953612.0,     41468124.0,  41982636.0,  42497148.0,  43011660.0};
    double _expWB[] = {15371868.0f,  15699756.0f,  16027644.0f,  16355532.0f,  16683420.0f,  17011308.0f,    17339196.0f,  17667084.0f,  17994972.0f,  18322860.0f,  18650748.0f,  18978636.0f,    19306524.0f,  19634412.0f,  19962300.0f,  20290188.0f,  20618076.0f,  20945964.0f,    21273852.0f,  21601740.0f,  21929628.0f,  22257516.0f,  22585404.0f,  22913292.0f,    23241180.0f,  37709724.0f,  38317548.0f,  38925372.0f,  39533196.0f,  40141020.0f,    40748844.0f,  41356668.0f,  41964492.0f,  42572316.0f,  43180140.0f,  43787964.0f,    44395788.0f,  45003612.0f,  45611436.0f,  46219260.0f,  46827084.0f,  47434908.0f,    48042732.0f,  48650556.0f,  49258380.0f,  49866204.0f,  50474028.0f,  51081852.0f,    51689676.0f,  52297500.0f,  17752620.0f,  18173820.0f,  18595020.0f,  19016220.0f,    19437420.0f,  19858620.0f,  20279820.0f,  20701020.0f,  21122220.0f,  21543420.0f,    21964620.0f,  22385820.0f,  22807020.0f,  23228220.0f,  23649420.0f,  24070620.0f,    24491820.0f,  24913020.0f,  25334220.0f,  25755420.0f,  26176620.0f,  26597820.0f,    27019020.0f,  27440220.0f,  27861420.0f,  42423276.0f,  43124412.0f,  43825548.0f,    44526684.0f,  45227820.0f,  45928956.0f,  46630092.0f,  47331228.0f,  48032364.0f,    48733500.0f,  49434636.0f,  50135772.0f,  50836908.0f,  51538044.0f,  52239180.0f,    52940316.0f,  53641452.0f,  54342588.0f,  55043724.0f,  55744860.0f,  56445996.0f,    57147132.0f,  57848268.0f,  58549404.0f,  59250540.0f,  20133372.0f,  20647884.0f,    21162396.0f,  21676908.0f,  22191420.0f,  22705932.0f,  23220444.0f,  23734956.0f,    24249468.0f,  24763980.0f,  25278492.0f,  25793004.0f,  26307516.0f,  26822028.0f,    27336540.0f,  27851052.0f,  28365564.0f,  28880076.0f,  29394588.0f,  29909100.0f,    30423612.0f,  30938124.0f,  31452636.0f,  31967148.0f,  32481660.0f,  47136828.0f,    47931276.0f,  48725724.0f,  49520172.0f,  50314620.0f,  51109068.0f,  51903516.0f,    52697964.0f,  53492412.0f,  54286860.0f,  55081308.0f,  55875756.0f,  56670204.0f,    57464652.0f,  58259100.0f,  59053548.0f,  59847996.0f,  60642444.0f,  61436892.0f,    62231340.0f,  63025788.0f,  63820236.0f,  64614684.0f,  65409132.0f,  66203580.0f,};
    int _expWS[] = {4, 3, 2, 5, 5, 50, 25 ,5, 1,  0, 1, 99};
    
    double _expEB[] = {9261.0f,    18786.0f,    28578.0f,    38640.0f,    48975.0f,    49770.0f,    40386.0f,    30720.0f,    20769.0f,    10530.0f,    19995.0f,    40551.0f,    61674.0f,    83370.0f,    105645.0f,   107310.0f,    87054.0f,    66201.0f,    44745.0f,    22680.0f,    32292.0f,    65475.0f,    99558.0f,   134550.0f,   170460.0f,   173070.0f,   140364.0f,   106713.0f,    72108.0f,    36540.0f,    46242.0f,    93738.0f,   142500.0f,   192540.0f,   243870.0f,    247500.0f,   200676.0f,   152526.0f,   103038.0f,    52200.0f,    61935.0f,   125520.0f,    190770.0f,   257700.0f,   326325.0f,   331050.0f,   268350.0f,   203910.0f,   137715.0f,    69750.0f,    67425.0f,   136590.0f,   207510.0f,   280200.0f,   354675.0f,   359400.0f,    291210.0f,   221190.0f,   149325.0f,    75600.0f,    58146.0f,   117750.0f,   178824.0f,    241380.0f,   305430.0f,   309360.0f,   250572.0f,   190254.0f,   128394.0f,    64980.0f,    46854.0f,    94851.0f,   144000.0f,   194310.0f,   245790.0f,   248850.0f,   201492.0f,    152937.0f,   103176.0f,    52200.0f,    33459.0f,    67713.0f,   102768.0f,   138630.0f,    175305.0f,   177420.0f,   143610.0f,   108969.0f,    73491.0f,    37170.0f,    17871.0f,    36156.0f,    54858.0f,    73980.0f,    93525.0f,    94620.0f,    76566.0f,    58080.0f,    39159.0f,    19800.0f,    36660.0f,    73983.0f,   111972.0f,   150630.0f,   189960.0f,    191130.0f,   154272.0f,   116733.0f,    78510.0f,    39600.0f,    76863.0f,   155085.0f,    234672.0f,   315630.0f,   397965.0f,   400380.0f,   323106.0f,   244437.0f,   164367.0f,    82890.0f,   120699.0f,   243486.0f,   368370.0f,   495360.0f,   624465.0f,   628200.0f,    506862.0f,   383382.0f,   257751.0f,   129960.0f,   168258.0f,   339366.0f,   513336.0f,    690180.0f,   869910.0f,   875040.0f,   705900.0f,   533838.0f,   358842.0f,   180900.0f,    219630.0f,   442905.0f,   669840.0f,   900450.0f,  1134750.0f,  1141350.0f,   920580.0f,    696075.0f,   467820.0f,   235800.0f,   227370.0f,   458475.0f,   693330.0f,   931950.0f,    1174350.0f,  1180950.0f,   952440.0f,   720105.0f,   483930.0f,   243900.0f,   190242.0f,    383538.0f,   579900.0f,   779340.0f,   981870.0f,   987300.0f,   796116.0f,   601806.0f,    404358.0f,   203760.0f,   149031.0f,   300402.0f,   454122.0f,   610200.0f,   768645.0f,    772830.0f,   623070.0f,   470916.0f,   316359.0f,   159390.0f,   103647.0f,   208887.0f,    315726.0f,   424170.0f,   534225.0f,   537090.0f,   432942.0f,   327165.0f,   219753.0f,    110700.0f,    54000.0f,   108813.0f,   164442.0f,   220890.0f,   278160.0f,   279630.0f,    225372.0f,   170283.0f,   114360.0f,    57600.0f,    42309.0f,    85530.0f,   129666.0f,    174720.0f,   220695.0f,   221490.0f,   179058.0f,   135696.0f,    91401.0f,    46170.0f,    89331.0f,   180519.0f,   273570.0f,   368490.0f,   465285.0f,   466950.0f,   377358.0f,    285873.0f,   192489.0f,    97200.0f,   141156.0f,   285147.0f,   431982.0f,   581670.0f,    734220.0f,   736830.0f,   595260.0f,   450801.0f,   303444.0f,   153180.0f,   197874.0f,    399594.0f,   605172.0f,   814620.0f,  1027950.0f,  1031580.0f,   833124.0f,   630750.0f,    424446.0f,   214200.0f,   259575.0f,   524040.0f,   793410.0f,  1067700.0f,  1346925.0f,    1351650.0f,  1091310.0f,   825990.0f,   555675.0f,   280350.0f,   265065.0f,   535110.0f,    810150.0f,  1090200.0f,  1375275.0f,  1380000.0f,  1114170.0f,   843270.0f,   567285.0f,    286200.0f,   222738.0f,   449526.0f,   680376.0f,   915300.0f,  1154310.0f,  1158240.0f,    934860.0f,   707358.0f,   475722.0f,   239940.0f,   175158.0f,   353403.0f,   534744.0f,    719190.0f,   906750.0f,   909810.0f,   734148.0f,   555345.0f,   373392.0f,   188280.0f,    122235.0f,   246561.0f,   372984.0f,   501510.0f,   632145.0f,   634260.0f,   511674.0f,    386961.0f,   260115.0f,   131130.0f,    63879.0f,   128820.0f,   194826.0f,   261900.0f,    330045.0f,   331140.0f,   267078.0f,   201936.0f,   135711.0f,    68400.0f,    85908.0f,    173127.0f,   261660.0f,   351510.0f,   442680.0f,   443850.0f,   357744.0f,   270309.0f,    181542.0f,    91440.0f,   178599.0f,   359853.0f,   543768.0f,   730350.0f,   919605.0f,    922020.0f,   743010.0f,   561309.0f,   376911.0f,   189810.0f,   278163.0f,   560358.0f,    846594.0f,  1136880.0f,  1431225.0f,  1434960.0f,  1156158.0f,   873270.0f,   586287.0f,    295200.0f,   384690.0f,   774822.0f,  1170408.0f,  1571460.0f,  1977990.0f,  1983120.0f,    1597548.0f,  1206462.0f,   809850.0f,   407700.0f,   498270.0f,  1003425.0f,  1515480.0f,    2034450.0f,  2560350.0f,  2566950.0f,  2067540.0f,  1561155.0f,  1047780.0f,   527400.0f,    506010.0f,  1018995.0f,  1538970.0f,  2065950.0f,  2599950.0f,  2606550.0f,  2099400.0f,    1585185.0f,  1063890.0f,   535500.0f,   419634.0f,   844914.0f,  1275852.0f,  1712460.0f,    2154750.0f,  2160180.0f,  1739604.0f,  1313310.0f,   881286.0f,   443520.0f,   325935.0f,    656154.0f,   990666.0f,  1329480.0f,  1672605.0f,  1676790.0f,  1350126.0f,  1019124.0f,    683775.0f,   344070.0f,   224823.0f,   452535.0f,   683142.0f,   916650.0f,  1153065.0f,    1155930.0f,   930606.0f,   702357.0f,   471177.0f,   237060.0f,   116208.0f,   233877.0f,    353010.0f,   473610.0f,   595680.0f,   597150.0f,   480684.0f,   362739.0f,   243312.0f,    122400.0f,};
    int _expES[] = {4, 2, 2, 10, 10, 200, 100, 10, 1, 0, 1, 99};

    NDArray<double> expW(_expWB, _expWS);
    expW.triggerAllocationFlag(false, false);

    NDArray<double> expE(_expEB, _expES);
    expE.triggerAllocationFlag(false, false);

    auto input = new NDArray<double>('c', {B, iC, iY, iX});
    for (int e = 0; e < input->lengthOf(); e++)
        input->putScalar(e, e+1);

    auto weights = new NDArray<double> ('c', {oC, iC, kY, kX});
    for (int e = 0; e < weights->lengthOf(); e++)
        weights->putScalar(e, e+1);


    auto epsilonNext = new NDArray<double>('c', {B, iC * oC, 6, 6});
    for (int e = 0; e < epsilonNext->lengthOf(); e++)
        epsilonNext->putScalar(e, e+1);

    auto col = new NDArray<double>('c', {B, iC, kY, kX, 6, 6});
    for (int e = 0; e < col->lengthOf(); e++)
        col->putScalar(e, e+1);


    auto variableSpace = new VariableSpace<double>();
    variableSpace->putVariable(-1, input);
    variableSpace->putVariable(-3, weights);
    variableSpace->putVariable(-2, epsilonNext);

    auto block = new Context<double>(1, variableSpace, false);
    block->fillInputs({-1, -2, -3});

    block->getIArguments()->push_back(kY);
    block->getIArguments()->push_back(kX);

    block->getIArguments()->push_back(sY);
    block->getIArguments()->push_back(sX);

    block->getIArguments()->push_back(pY);
    block->getIArguments()->push_back(pX);

    // dilation
    block->getIArguments()->push_back(1);
    block->getIArguments()->push_back(1);

    // NOT same mode
    block->getIArguments()->push_back(0);


    variableSpace->getStash()->storeArray(1, "im2col", col);


    nd4j::ops::sconv2d_bp<double> op;

    Nd4jStatus status = op.execute(block);

    ASSERT_EQ(ND4J_STATUS_OK, status);

    std::pair<int, int> pe(1, 0);
    std::pair<int, int> pgw(1, 1);
    auto epsilon = variableSpace->getVariable(pe)->getNDArray();
    auto gradW = variableSpace->getVariable(pgw)->getNDArray();

    ASSERT_TRUE(expW.isSameShape(gradW));

    //expW.printBuffer("Expctd buffer");
    //gradW->printBuffer("Result buffer");
    ASSERT_TRUE(expW.equalsTo(gradW));


    ASSERT_TRUE(expE.isSameShape(epsilon));

    //    expE.printBuffer("Expctd buffer");
    //epsilon->printBuffer("Result buffer");
    ASSERT_TRUE(expE.equalsTo(epsilon));

    delete variableSpace;
    delete block;
}

TEST_F(ConvolutionTests, deconv2D_FF_NoBias_1) {
    int _expS[] = {4, 2, 3, 8, 8, 192, 64, 8, 1, 0, 1, 99};
    double _expB[] = {6276.0,   12831.0,   19668.0,   26790.0,   27012.0,   20703.0,   14100.0,    7200.0,    13719.0,   28023.0,   42918.0,   58410.0,   58902.0,   45105.0,   30693.0,   15660.0,    22389.0,   45696.0,   69930.0,   95100.0,   95910.0,   73386.0,   49899.0,   25440.0,    32346.0,   65970.0,  100884.0,  137100.0,  138276.0,  105726.0,   71838.0,   36600.0,    33726.0,   68790.0,  105204.0,  142980.0,  144156.0,  110226.0,   74898.0,   38160.0,    27555.0,   56154.0,   85806.0,  116520.0,  117474.0,   89748.0,   60933.0,   31020.0,    19917.0,   40557.0,   61926.0,   84030.0,   84714.0,   64671.0,   43875.0,   22320.0,    10752.0,   21879.0,   33384.0,   45270.0,   45636.0,   34815.0,   23604.0,   12000.0,    7551.0,   15456.0,   23718.0,   32340.0,   32562.0,   24978.0,   17025.0,    8700.0,    16569.0,   33873.0,   51918.0,   70710.0,   71202.0,   54555.0,   37143.0,   18960.0,    27114.0,   55371.0,   84780.0,  115350.0,  116160.0,   88911.0,   60474.0,   30840.0,    39246.0,   80070.0,  122484.0,  166500.0,  167676.0,  128226.0,   87138.0,   44400.0,    40626.0,   82890.0,  126804.0,  172380.0,  173556.0,  132726.0,   90198.0,   45960.0,    33180.0,   67629.0,  103356.0,  140370.0,  141324.0,  107973.0,   73308.0,   37320.0,    23967.0,   48807.0,   74526.0,  101130.0,  101814.0,   77721.0,   52725.0,   26820.0,    12927.0,   26304.0,   40134.0,   54420.0,   54786.0,   41790.0,   28329.0,   14400.0,    8826.0,   18081.0,   27768.0,   37890.0,   38112.0,   29253.0,   19950.0,   10200.0,    19419.0,   39723.0,   60918.0,   83010.0,   83502.0,   64005.0,   43593.0,   22260.0,    31839.0,   65046.0,   99630.0,  135600.0,  136410.0,  104436.0,   71049.0,   36240.0,    46146.0,   94170.0,  144084.0,  195900.0,  197076.0,  150726.0,  102438.0,   52200.0,    47526.0,   96990.0,  148404.0,  201780.0,  202956.0,  155226.0,  105498.0,   53760.0,    38805.0,   79104.0,  120906.0,  164220.0,  165174.0,  126198.0,   85683.0,   43620.0,    28017.0,   57057.0,   87126.0,  118230.0,  118914.0,   90771.0,   61575.0,   31320.0,    15102.0,   30729.0,   46884.0,   63570.0,   63936.0,   48765.0,   33054.0,   16800.0,    17220.0,   34863.0,   52932.0,   71430.0,   72228.0,   54831.0,   36996.0,   18720.0,    36327.0,   73527.0,  111606.0,  150570.0,  152214.0,  115521.0,   77925.0,   39420.0,    57381.0,  116112.0,  176202.0,  237660.0,  240198.0,  182250.0,  122907.0,   62160.0,    80442.0,  162738.0,  246900.0,  332940.0,  336420.0,  255198.0,  172062.0,   87000.0,    84702.0,  171318.0,  259860.0,  350340.0,  353820.0,  268338.0,  180882.0,   91440.0,    66867.0,  135210.0,  205038.0,  276360.0,  279042.0,  211572.0,  142581.0,   72060.0,    46845.0,   94701.0,  143574.0,  193470.0,  195306.0,  148047.0,   99747.0,   50400.0,    24576.0,   49671.0,   75288.0,  101430.0,  102372.0,   77583.0,   52260.0,   26400.0,    22095.0,   44688.0,   67782.0,   91380.0,   92178.0,   69906.0,   47121.0,   23820.0,    46377.0,   93777.0,  142206.0,  191670.0,  193314.0,  146571.0,   98775.0,   49920.0,    72906.0,  147387.0,  223452.0,  301110.0,  303648.0,  230175.0,  155082.0,   78360.0,    101742.0,  205638.0,  311700.0,  419940.0,  423420.0,  320898.0,  216162.0,  109200.0,    106002.0,  214218.0,  324660.0,  437340.0,  440820.0,  334038.0,  224982.0,  113640.0,    83292.0,  168285.0,  254988.0,  343410.0,  346092.0,  262197.0,  176556.0,   89160.0,    58095.0,  117351.0,  177774.0,  239370.0,  241206.0,  182697.0,  122997.0,   62100.0,    30351.0,   61296.0,   92838.0,  124980.0,  125922.0,   95358.0,   64185.0,   32400.0,    26970.0,   54513.0,   82632.0,  111330.0,  112128.0,   84981.0,   57246.0,   28920.0,    56427.0,  114027.0,  172806.0,  232770.0,  234414.0,  177621.0,  119625.0,   60420.0,    88431.0,  178662.0,  270702.0,  364560.0,  367098.0,  278100.0,  187257.0,   94560.0,    123042.0,  248538.0,  376500.0,  506940.0,  510420.0,  386598.0,  260262.0,  131400.0,    127302.0,  257118.0,  389460.0,  524340.0,  527820.0,  399738.0,  269082.0,  135840.0,    99717.0,  201360.0,  304938.0,  410460.0,  413142.0,  312822.0,  210531.0,  106260.0,    69345.0,  140001.0,  211974.0,  285270.0,  287106.0,  217347.0,  146247.0,   73800.0,    36126.0,   72921.0,  110388.0,  148530.0,  149472.0,  113133.0,   76110.0,   38400.0,};
    NDArray<double> exp(_expB, _expS);
    exp.triggerAllocationFlag(false, false);


    auto input = new NDArray<double>('c', {2, 3, 4, 4});
    auto weights = new NDArray<double>('c', {3, 3, 5, 5});

    nd4j::NDArrayFactory<double>::linspace(1, *input);
    nd4j::NDArrayFactory<double>::linspace(1, *weights);

    auto variableSpace = new VariableSpace<double>();
    variableSpace->putVariable(-1, input);
    variableSpace->putVariable(-2, weights);

    auto block = new Context<double>(1, variableSpace, false);
    block->fillInputs({-1, -2});

    block->getIArguments()->push_back(5);
    block->getIArguments()->push_back(5);

    block->getIArguments()->push_back(1);
    block->getIArguments()->push_back(1);

    block->getIArguments()->push_back(0);
    block->getIArguments()->push_back(0);

    // dilation
    block->getIArguments()->push_back(1);
    block->getIArguments()->push_back(1);

    // NOT same mode
    block->getIArguments()->push_back(0);

    nd4j::ops::deconv2d<double> op;

    Nd4jStatus status = op.execute(block);

    ASSERT_EQ(ND4J_STATUS_OK, status);

    NDArray<double>* output = variableSpace->getVariable(1)->getNDArray();

    ASSERT_TRUE(exp.isSameShape(output));

    //    exp.printBuffer("Expctd buffer");
    //output->printBuffer("Result buffer");
    ASSERT_TRUE(exp.equalsTo(output));

    delete variableSpace;
    delete block;
}

TEST_F(ConvolutionTests, conv2D_BP_Bias_1) {
    printf("\n");
    double _expWGradB[] = {9312.0, 12580.0, 9528.0, 13168.0, 17712.0, 13360.0, 9960.0, 13348.0, 10032.0, 13344.0, 18148.0, 13848.0, 19312.0, 26160.0, 19888.0, 15144.0, 20452.0, 15504.0};
    int _expWGradS[] = {4, 2, 1, 3, 3, 9, 9, 3, 1, 0, 1, 99};
    NDArray<double> expWGrad(_expWGradB, _expWGradS);
    expWGrad.triggerAllocationFlag(false, false);

    double _expBGradB[] = {784.0, 1296.0};
    int _expBGradS[] = {2, 2, 1, 1, 1, 0, 1, 99};

    NDArray<double> expBGrad(_expBGradB, _expBGradS);
    expBGrad.triggerAllocationFlag(false, false);

    NDArray<double> input('c', {2, 1, 4, 4});
    NDArray<double> weights('c', {2, 1, 3, 3});
    NDArray<double> bias('c', {2, 1});
    NDArray<double> epsilonNext('c', {2, 2, 4, 4});


    double _expEpsB[] = {952.0, 1540.0, 1636.0, 1180.0, 1791.0, 2886.0, 3057.0, 2193.0, 2223.0, 3570.0, 3741.0, 2673.0, 1900.0, 3028.0, 3160.0, 2240.0, 2872.0, 4612.0, 4708.0, 3356.0, 5247.0, 8358.0, 8529.0, 6033.0, 5679.0, 9042.0, 9213.0, 6513.0, 4588.0, 7252.0, 7384.0, 5184.0};
    NDArray<double> expEps(_expEpsB, input.getShapeInfo());

    nd4j::NDArrayFactory<double>::linspace(1, input);
    nd4j::NDArrayFactory<double>::linspace(1, weights);
    nd4j::NDArrayFactory<double>::linspace(1, epsilonNext);

    nd4j::ops::conv2d_bp<double> op;

    auto results = op.execute({&input, &weights, &bias, &epsilonNext}, {},  {3, 3, 1, 1, 0, 0, 1, 1, 1});

    ASSERT_TRUE(results->size() == 3);

    auto epsilon = results->at(0);
    auto gradW = results->at(1);
    auto gradB = results->at(2);

    ASSERT_TRUE(expWGrad.isSameShape(gradW));

    //expWGrad.printBuffer("Expctd buffer");
    //  gradW->printBuffer("Result buffer");
    ASSERT_TRUE(expWGrad.equalsTo(gradW));


    ASSERT_TRUE(input.isSameShape(epsilon));

    //  expEps.printBuffer("Expctd buffer");
    //epsilon->printBuffer("Result buffer");
    ASSERT_TRUE(expEps.equalsTo(epsilon));

    ASSERT_TRUE(expBGrad.isSameShape(gradB));

    ASSERT_TRUE(expBGrad.equalsTo(gradB));

    delete results;
}


TEST_F(ConvolutionTests, conv2D_BP_NoBias_1) {
    printf("\n");
    double _expWGradB[] = {9312.0, 12580.0, 9528.0, 13168.0, 17712.0, 13360.0, 9960.0, 13348.0, 10032.0, 13344.0, 18148.0, 13848.0, 19312.0, 26160.0, 19888.0, 15144.0, 20452.0, 15504.0};
    int _expWGradS[] = {4, 2, 1, 3, 3, 9, 9, 3, 1, 0, 1, 99};
    NDArray<double> expWGrad(_expWGradB, _expWGradS);
    expWGrad.triggerAllocationFlag(false, false);

    NDArray<double> input('c', {2, 1, 4, 4});
    NDArray<double> weights('c', {2, 1, 3, 3});
    NDArray<double> epsilonNext('c', {2, 2, 4, 4});


    double _expEpsB[] = {952.0, 1540.0, 1636.0, 1180.0, 1791.0, 2886.0, 3057.0, 2193.0, 2223.0, 3570.0, 3741.0, 2673.0, 1900.0, 3028.0, 3160.0, 2240.0, 2872.0, 4612.0, 4708.0, 3356.0, 5247.0, 8358.0, 8529.0, 6033.0, 5679.0, 9042.0, 9213.0, 6513.0, 4588.0, 7252.0, 7384.0, 5184.0};
    NDArray<double> expEps(_expEpsB, input.getShapeInfo());

    nd4j::NDArrayFactory<double>::linspace(1, input);
    nd4j::NDArrayFactory<double>::linspace(1, weights);
    nd4j::NDArrayFactory<double>::linspace(1, epsilonNext);

    nd4j::ops::conv2d_bp<double> op;

    auto results = op.execute({&input, &weights, &epsilonNext}, {},  {3, 3, 1, 1, 0, 0, 1, 1, 1});

    ASSERT_TRUE(results->size() == 2);

    auto epsilon = results->at(0);
    auto gradW = results->at(1);

    ASSERT_TRUE(expWGrad.isSameShape(gradW));

    //expWGrad.printBuffer("Expctd buffer");
    //  gradW->printBuffer("Result buffer");
    ASSERT_TRUE(expWGrad.equalsTo(gradW));


    ASSERT_TRUE(input.isSameShape(epsilon));

    //  expEps.printBuffer("Expctd buffer");
    //epsilon->printBuffer("Result buffer");
    ASSERT_TRUE(expEps.equalsTo(epsilon));

    delete results;
}

TEST_F(ConvolutionTests, sconv2D_FF_NoBias_2) {
    double _expBFF[] = {10025.0f,   10350.0f,   10675.0f,   11000.0f,   11325.0f,   11650.0f,   13275.0f,   13600.0f,   13925.0f,   14250.0f,   14575.0f,   14900.0f,   16525.0f,   16850.0f,   17175.0f,   17500.0f,   17825.0f,   18150.0f,   19775.0f,   20100.0f,   20425.0f,   20750.0f,   21075.0f,   21400.0f,   23025.0f,   23350.0f,   23675.0f,   24000.0f,   24325.0f,   24650.0f,   26275.0f,   26600.0f,   26925.0f,   27250.0f,   27575.0f,   27900.0f,   53150.0f,   55350.0f,   57550.0f,   59750.0f,   61950.0f,   64150.0f,   75150.0f,   77350.0f,   79550.0f,   81750.0f,   83950.0f,   86150.0f,   97150.0f,   99350.0f,  101550.0f,  103750.0f,  105950.0f,  108150.0f,   119150.0f,  121350.0f,  123550.0f,  125750.0f,  127950.0f,  130150.0f,   141150.0f,  143350.0f,  145550.0f,  147750.0f,  149950.0f,  152150.0f,   163150.0f,  165350.0f,  167550.0f,  169750.0f,  171950.0f,  174150.0f,   119400.0f,  120350.0f,  121300.0f,  122250.0f,  123200.0f,  124150.0f,   128900.0f,  129850.0f,  130800.0f,  131750.0f,  132700.0f,  133650.0f,   138400.0f,  139350.0f,  140300.0f,  141250.0f,  142200.0f,  143150.0f,   147900.0f,  148850.0f,  149800.0f,  150750.0f,  151700.0f,  152650.0f,   157400.0f,  158350.0f,  159300.0f,  160250.0f,  161200.0f,  162150.0f,   166900.0f,  167850.0f,  168800.0f,  169750.0f,  170700.0f,  171650.0f,   350025.0f,  352850.0f,  355675.0f,  358500.0f,  361325.0f,  364150.0f,   378275.0f,  381100.0f,  383925.0f,  386750.0f,  389575.0f,  392400.0f,   406525.0f,  409350.0f,  412175.0f,  415000.0f,  417825.0f,  420650.0f,   434775.0f,  437600.0f,  440425.0f,  443250.0f,  446075.0f,  448900.0f,   463025.0f,  465850.0f,  468675.0f,  471500.0f,  474325.0f,  477150.0f,   491275.0f,  494100.0f,  496925.0f,  499750.0f,  502575.0f,  505400.0f,   353775.0f,  355350.0f,  356925.0f,  358500.0f,  360075.0f,  361650.0f,   369525.0f,  371100.0f,  372675.0f,  374250.0f,  375825.0f,  377400.0f,   385275.0f,  386850.0f,  388425.0f,  390000.0f,  391575.0f,  393150.0f,   401025.0f,  402600.0f,  404175.0f,  405750.0f,  407325.0f,  408900.0f,   416775.0f,  418350.0f,  419925.0f,  421500.0f,  423075.0f,  424650.0f,   432525.0f,  434100.0f,  435675.0f,  437250.0f,  438825.0f,  440400.0f,   771900.0f,  775350.0f,  778800.0f,  782250.0f,  785700.0f,  789150.0f,   806400.0f,  809850.0f,  813300.0f,  816750.0f,  820200.0f,  823650.0f,   840900.0f,  844350.0f,  847800.0f,  851250.0f,  854700.0f,  858150.0f,   875400.0f,  878850.0f,  882300.0f,  885750.0f,  889200.0f,  892650.0f,   909900.0f,  913350.0f,  916800.0f,  920250.0f,  923700.0f,  927150.0f,   944400.0f,  947850.0f,  951300.0f,  954750.0f,  958200.0f,  961650.0f,   107525.0f,  107850.0f,  108175.0f,  108500.0f,  108825.0f,  109150.0f,   110775.0f,  111100.0f,  111425.0f,  111750.0f,  112075.0f,  112400.0f,   114025.0f,  114350.0f,  114675.0f,  115000.0f,  115325.0f,  115650.0f,   117275.0f,  117600.0f,  117925.0f,  118250.0f,  118575.0f,  118900.0f,   120525.0f,  120850.0f,  121175.0f,  121500.0f,  121825.0f,  122150.0f,   123775.0f,  124100.0f,  124425.0f,  124750.0f,  125075.0f,  125400.0f,   713150.0f,  715350.0f,  717550.0f,  719750.0f,  721950.0f,  724150.0f,   735150.0f,  737350.0f,  739550.0f,  741750.0f,  743950.0f,  746150.0f,   757150.0f,  759350.0f,  761550.0f,  763750.0f,  765950.0f,  768150.0f,   779150.0f,  781350.0f,  783550.0f,  785750.0f,  787950.0f,  790150.0f,   801150.0f,  803350.0f,  805550.0f,  807750.0f,  809950.0f,  812150.0f,   823150.0f,  825350.0f,  827550.0f,  829750.0f,  831950.0f,  834150.0f,   404400.0f,  405350.0f,  406300.0f,  407250.0f,  408200.0f,  409150.0f,   413900.0f,  414850.0f,  415800.0f,  416750.0f,  417700.0f,  418650.0f,   423400.0f,  424350.0f,  425300.0f,  426250.0f,  427200.0f,  428150.0f,   432900.0f,  433850.0f,  434800.0f,  435750.0f,  436700.0f,  437650.0f,   442400.0f,  443350.0f,  444300.0f,  445250.0f,  446200.0f,  447150.0f,   451900.0f,  452850.0f,  453800.0f,  454750.0f,  455700.0f,  456650.0f,   1197525.0f, 1200350.0f, 1203175.0f, 1206000.0f, 1208825.0f, 1211650.0f,   1225775.0f, 1228600.0f, 1231425.0f, 1234250.0f, 1237075.0f, 1239900.0f,   1254025.0f, 1256850.0f, 1259675.0f, 1262500.0f, 1265325.0f, 1268150.0f,   1282275.0f, 1285100.0f, 1287925.0f, 1290750.0f, 1293575.0f, 1296400.0f,   1310525.0f, 1313350.0f, 1316175.0f, 1319000.0f, 1321825.0f, 1324650.0f,   1338775.0f, 1341600.0f, 1344425.0f, 1347250.0f, 1350075.0f, 1352900.0f,   826275.0f,  827850.0f,  829425.0f,  831000.0f,  832575.0f,  834150.0f,   842025.0f,  843600.0f,  845175.0f,  846750.0f,  848325.0f,  849900.0f,   857775.0f,  859350.0f,  860925.0f,  862500.0f,  864075.0f,  865650.0f,   873525.0f,  875100.0f,  876675.0f,  878250.0f,  879825.0f,  881400.0f,   889275.0f,  890850.0f,  892425.0f,  894000.0f,  895575.0f,  897150.0f,   905025.0f,  906600.0f,  908175.0f,  909750.0f,  911325.0f,  912900.0f,   1806900.0f, 1810350.0f, 1813800.0f, 1817250.0f, 1820700.0f, 1824150.0f,   1841400.0f, 1844850.0f, 1848300.0f, 1851750.0f, 1855200.0f, 1858650.0f,   1875900.0f, 1879350.0f, 1882800.0f, 1886250.0f, 1889700.0f, 1893150.0f,   1910400.0f, 1913850.0f, 1917300.0f, 1920750.0f, 1924200.0f, 1927650.0f,   1944900.0f, 1948350.0f, 1951800.0f, 1955250.0f, 1958700.0f, 1962150.0f,   1979400.0f, 1982850.0f, 1986300.0f, 1989750.0f, 1993200.0f, 1996650.};
    int _expSFF[] = {4, 2, 6, 6, 6, 216, 36, 6, 1, 0, 1, 99,};
    NDArray<double> expFF(_expBFF, _expSFF);
    expFF.triggerAllocationFlag(false, false);


    double _exp2BFF[] = {827.4900282f,   832.2350283f,   836.9800284f,   841.725028f,   846.4700287f,   851.2150288f,   874.9400293f,   879.6850294f,   884.4300295f,   889.1750296f,   893.9200297f,   898.665029f,   922.3900304f,   927.1350305f,   931.8800306f,   936.6250307f,   941.3700308f,   946.1150309f,   969.8400315f,   974.5850316f,   979.3300317f,   984.0750318f,   988.8200319f,   993.5650320f,   1017.2900326f,  1022.0350327f,  1026.7800328f,  1031.5250329f,   1036.2700330f,  1041.0150331f,  1064.7400337f,  1069.4850338f,   1074.2300339f,  1078.9750340f,  1083.7200341f,  1088.4650342f,   1822.4550553f,  1833.995055f,   1845.5350558f,  1857.075056f,   1868.6150563f,  1880.1550566f,  1937.8550578f,  1949.3950581f,   1960.9350583f,  1972.4750586f,  1984.015058f,   1995.5550591f,   2053.2550604f,  2064.7950606f,  2076.3350609f,  2087.8750611f,   2099.4150614f,  2110.955061f,   2168.6550629f,  2180.1950632f,   2191.7350634f,  2203.2750637f,  2214.8150639f,  2226.3550642f,   2284.0550655f,  2295.5950657f,  2307.1350660f,  2318.6750662f,   2330.2150665f,  2341.7550667f,  2399.4550680f,  2410.9950683f,   2422.5350685f,  2434.0750688f,  2445.6150690f,  2457.1550693f,   2817.419968f,   2835.7549686f,  2854.0899683f,  2872.4249680f,   2890.7599677f,  2909.0949674f,  3000.7699660f,  3019.104965f,   3037.4399655f,  3055.7749652f,  3074.1099649f,  3092.4449646f,   3184.1199632f,  3202.4549629f,  3220.789962f,   3239.1249624f,   3257.4599621f,  3275.7949618f,  3367.4699604f,  3385.8049601f,   3404.1399598f,  3422.474959f,   3440.8099593f,  3459.1449590f,   3550.8199576f,  3569.1549573f,  3587.4899570f,  3605.8249567f,   3624.1599565f,  3642.4949562f,  3734.1699548f,  3752.5049545f,   3770.8399542f,  3789.1749539f,  3807.5099536f,  3825.8449534f,   3812.385098f,   3837.5150988f,  3862.6450994f,  3887.7751000f,   3912.9051006f,  3938.0351012f,  4063.6851041f,  4088.8151047f,   4113.9451053f,  4139.0751059f,  4164.2051065f,  4189.3351071f,   4314.9851100f,  4340.1151106f,  4365.2451112f,  4390.3751118f,   4415.5051124f,  4440.6351130f,  4566.2851159f,  4591.4151165f,   4616.5451171f,  4641.6751177f,  4666.805118f,   4691.9351188f,   4817.5851218f,  4842.7151224f,  4867.8451230f,  4892.975123f,   4918.1051241f,  4943.2351247f,  5068.8851277f,  5094.0151283f,   5119.1451288f,  5144.2751294f,  5169.4051300f,  5194.5351306f,   4807.3499803f,  4839.2749801f,  4871.1999799f,  4903.1249797f,   4935.0499795f,  4966.9749793f,  5126.5999784f,  5158.5249782f,   5190.4499780f,  5222.3749778f,  5254.2999777f,  5286.2249775f,   5445.8499765f,  5477.774976f,   5509.6999762f,  5541.6249760f,   5573.5499758f,  5605.4749756f,  5765.0999747f,  5797.0249745f,   5828.9499743f,  5860.8749741f,  5892.7999739f,  5924.724973f,   6084.3499728f,  6116.2749726f,  6148.1999724f,  6180.1249723f,   6212.0499721f,  6243.9749719f,  6403.59997f,    6435.5249708f,   6467.4499706f,  6499.3749704f,  6531.2999702f,  6563.2249700f,   5802.3150007f,  5841.0350006f,  5879.7550005f,  5918.4750004f,   5957.195000f,   5995.9150003f,  6189.5149999f,  6228.2349998f,   6266.9549997f,  6305.6749996f,  6344.3949995f,  6383.114999f,   6576.7149990f,  6615.4349990f,  6654.1549989f,  6692.8749988f,   6731.5949987f,  6770.3149986f,  6963.9149982f,  7002.6349981f,   7041.3549981f,  7080.0749980f,  7118.7949979f,  7157.5149978f,   7351.1149974f,  7389.8349973f,  7428.5549972f,  7467.2749972f,   7505.9949971f,  7544.7149970f,  7738.3149966f,  7777.0349965f,   7815.7549964f,  7854.4749963f,  7893.1949963f,  7931.9149962f,   6797.2799488f,  6842.794948f,   6888.3099489f,  6933.8249490f,   6979.3399491f,  7024.8549492f,  7252.4299497f,  7297.9449498f,   7343.4599499f,  7388.9749500f,  7434.489950f,   7480.0049501f,   7707.5799506f,  7753.0949507f,  7798.6099508f,  7844.1249509f,   7889.6399510f,  7935.1549511f,  8162.7299515f,  8208.2449516f,   8253.7599517f,  8299.2749518f,  8344.7899519f,  8390.3049520f,   8617.8799525f,  8663.394952f,   8708.9099526f,  8754.4249527f,   8799.9399528f,  8845.4549529f,  9073.0299534f,  9118.5449535f,   9164.0599536f,  9209.5749537f,  9255.089953f,   9300.604953f,   7792.2451647f,  7844.5551655f,  7896.8651663f,  7949.1751671f,   8001.4851679f,  8053.7951686f,  8315.3451725f,  8367.6551733f,   8419.9651741f,  8472.2751749f,  8524.585175f,   8576.8951764f,   8838.4451803f,  8890.7551811f,  8943.0651819f,  8995.3751827f,   9047.6851834f,  9099.9951842f,  9361.5451881f,  9413.8551889f,   9466.1651897f,  9518.475190f,   9570.7851912f,  9623.0951920f,   9884.6451959f,  9936.9551967f,  9989.2651975f, 10041.5751982f,   10093.8851990f, 10146.1951998f, 10407.7452037f, 10460.0552045f,   10512.3652053f, 10564.6752060f, 10616.9852068f, 10669.2952076f,   8787.210074f,   8846.3150748f,  8905.4200750f,  8964.5250752f,   9023.6300755f,  9082.7350757f,  9378.2600768f,  9437.3650770f,   9496.4700773f,  9555.5750775f,  9614.6800777f,  9673.7850779f,   9969.3100791f, 10028.4150793f, 10087.5200795f, 10146.625079f,   10205.7300800f, 10264.8350802f, 10560.3600813f, 10619.465081f,   10678.5700818f, 10737.6750820f, 10796.7800822f, 10855.8850825f,   11151.4100836f, 11210.5150838f, 11269.6200840f, 11328.7250843f,   11387.8300845f, 11446.9350847f, 11742.4600858f, 11801.5650861f,   11860.6700863f, 11919.7750865f, 11978.880086f,  12037.9850870f,   9782.1750935f,  9848.0750935f,  9913.9750934f,  9979.8750934f,   10045.7750934f, 10111.6750933f, 10441.1750931f, 10507.0750931f,   10572.9750931f, 10638.8750930f, 10704.7750930f, 10770.6750930f,   11100.1750928f, 11166.0750927f, 11231.9750927f, 11297.8750927f,   11363.7750926f, 11429.6750926f, 11759.1750924f, 11825.0750924f,   11890.9750923f, 11956.8750923f, 12022.7750923f, 12088.6750922f,   12418.175092f,  12484.0750920f, 12549.9750920f, 12615.8750919f,   12681.7750919f, 12747.6750919f, 13077.1750917f, 13143.0750916f,   13208.9750916f, 13274.8750916f, 13340.7750915f, 13406.6750915f,   2250.990060f,   2255.7350610f,  2260.4800611f,  2265.2250612f,   2269.9700613f,  2274.7150614f,  2298.4400619f,  2303.185062f,   2307.9300622f,  2312.6750623f,  2317.4200624f,  2322.1650625f,   2345.8900630f,  2350.6350631f,  2355.380063f,   2360.1250634f,   2364.8700635f,  2369.6150636f,  2393.3400641f,  2398.0850642f,   2402.8300643f,  2407.5750644f,  2412.320064f,   2417.0650647f,   2440.7900652f,  2445.5350653f,  2450.2800654f,  2455.0250655f,   2459.7700656f,  2464.515065f,   2488.2400663f,  2492.9850664f,   2497.7300665f,  2502.4750666f,  2507.2200667f,  2511.9650668f,   5284.4551315f,  5295.9951318f,  5307.535132f,   5319.0751323f,   5330.6151326f,  5342.1551328f,  5399.8551341f,  5411.3951343f,   5422.9351346f,  5434.475134f,   5446.0151351f,  5457.5551354f,   5515.2551366f,  5526.7951369f,  5538.3351371f,  5549.8751374f,   5561.4151376f,  5572.9551379f,  5630.6551392f,  5642.1951394f,   5653.7351397f,  5665.2751399f,  5676.8151402f,  5688.3551404f,   5746.0551417f,  5757.5951420f,  5769.1351422f,  5780.6751425f,   5792.2151427f,  5803.7551430f,  5861.455144f,   5872.9951445f,   5884.5351448f,  5896.0751450f,  5907.6151453f,  5919.1551455f,   8317.919884f,   8336.2548841f,  8354.5898838f,  8372.9248835f,   8391.2598832f,  8409.59488f,    8501.2698815f,  8519.6048813f,   8537.9398810f,  8556.2748807f,  8574.6098804f,  8592.9448801f,   8684.6198787f,  8702.9548784f,  8721.2898782f,  8739.6248779f,   8757.9598776f,  8776.2948773f,  8867.9698759f,  8886.3048756f,   8904.6398753f,  8922.9748751f,  8941.3098748f,  8959.6448745f,   9051.3198731f,  9069.6548728f,  9087.9898725f,  9106.3248722f,   9124.6598720f,  9142.9948717f,  9234.6698703f,  9253.0048700f,   9271.3398697f,  9289.6748694f,  9308.0098691f,  9326.3448689f,   11351.3852747f, 11376.5152753f, 11401.6452759f, 11426.7752765f,   11451.9052771f, 11477.0352777f, 11602.6852806f, 11627.8152812f,   11652.9452818f, 11678.0752824f, 11703.2052830f, 11728.335283f,   11853.9852865f, 11879.1152871f, 11904.2452877f, 11929.3752883f,   11954.505288f,  11979.6352894f, 12105.2852924f, 12130.4152930f,   12155.545293f,  12180.6752941f, 12205.8052947f, 12230.9352953f,   12356.5852983f, 12381.715298f,  12406.8452994f, 12431.9753000f,   12457.1053006f, 12482.2353012f, 12607.8853041f, 12633.0153047f,   12658.1453053f, 12683.2753059f, 12708.4053065f, 12733.5353071f,   14384.8499244f, 14416.7749242f, 14448.6999240f, 14480.6249238f,   14512.549923f,  14544.4749235f, 14704.0999225f, 14736.024922f,   14767.9499222f, 14799.8749220f, 14831.7999218f, 14863.7249216f,   15023.3499207f, 15055.2749205f, 15087.1999203f, 15119.1249201f,   15151.0499199f, 15182.9749197f, 15342.5999188f, 15374.5249186f,   15406.4499184f, 15438.374918f,  15470.2999181f, 15502.2249179f,   15661.84991f,   15693.7749168f, 15725.6999166f, 15757.6249164f,   15789.5499162f, 15821.4749160f, 15981.0999151f, 16013.0249149f,   16044.9499147f, 16076.8749145f, 16108.7999143f, 16140.7249142f,   17418.314976f,  17457.0349761f, 17495.7549760f, 17534.4749759f,   17573.1949758f, 17611.9149757f, 17805.5149753f, 17844.234975f,   17882.9549752f, 17921.6749751f, 17960.3949750f, 17999.1149749f,   18192.7149745f, 18231.4349744f, 18270.154974f,  18308.8749743f,   18347.5949742f, 18386.3149741f, 18579.9149737f, 18618.6349736f,   18657.3549735f, 18696.074973f,  18734.7949734f, 18773.5149733f,   18967.1149729f, 19005.8349728f, 19044.5549727f, 19083.2749726f,   19121.994972f,  19160.7149725f, 19354.3149721f, 19393.0349720f,   19431.7549719f, 19470.4749718f, 19509.1949717f, 19547.914971f,   20451.7799765f, 20497.2949766f, 20542.8099767f, 20588.3249768f,   20633.8399769f, 20679.3549770f, 20906.929977f,  20952.4449775f,   20997.9599776f, 21043.4749777f, 21088.9899778f, 21134.5049779f,   21362.0799784f, 21407.5949785f, 21453.1099786f, 21498.624978f,   21544.139978f,  21589.6549788f, 21817.2299793f, 21862.7449794f,   21908.2599795f, 21953.7749796f, 21999.2899797f, 22044.8049798f,   22272.3799802f, 22317.8949803f, 22363.4099804f, 22408.9249805f,   22454.4399806f, 22499.9549807f, 22727.529981f,  22773.044981f,   22818.5599813f, 22864.0749814f, 22909.5899815f, 22955.1049816f,   23485.2453985f, 23537.555399f,  23589.8654000f, 23642.1754008f,   23694.4854016f, 23746.7954024f, 24008.3454063f, 24060.655407f,   24112.9654078f, 24165.2754086f, 24217.5854094f, 24269.8954102f,   24531.4454141f, 24583.7554148f, 24636.0654156f, 24688.3754164f,   24740.6854172f, 24792.99541f,   25054.545421f,  25106.8554226f,   25159.1654234f, 25211.4754242f, 25263.7854250f, 25316.0954257f,   25577.6454296f, 25629.9554304f, 25682.2654312f, 25734.5754320f,   25786.8854328f, 25839.1954335f, 26100.7454374f, 26153.0554382f,   26205.3654390f, 26257.6754398f, 26309.985440f,  26362.2954413f,   26518.7101423f, 26577.8151425f, 26636.920142f,  26696.0251430f,   26755.1301432f, 26814.2351434f, 27109.7601446f, 27168.8651448f,   27227.9701450f, 27287.0751452f, 27346.1801455f, 27405.2851457f,   27700.8101468f, 27759.9151470f, 27819.0201473f, 27878.1251475f,   27937.2301477f, 27996.33514f,   28291.8601491f, 28350.9651493f,   28410.0701495f, 28469.175149f,  28528.2801500f, 28587.3851502f,   28882.9101513f, 28942.0151516f, 29001.1201518f, 29060.2251520f,   29119.3301522f, 29178.4351525f, 29473.9601536f, 29533.0651538f,   29592.1701540f, 29651.2751543f, 29710.3801545f, 29769.4851547f,   29552.1750826f, 29618.0750825f, 29683.9750825f, 29749.8750825f,   29815.7750824f, 29881.6750824f, 30211.1750822f, 30277.0750822f,   30342.9750821f, 30408.8750821f, 30474.7750821f, 30540.6750820f,   30870.175081f,  30936.0750818f, 31001.9750818f, 31067.8750817f,   31133.7750817f, 31199.6750817f, 31529.1750815f, 31595.075081f,   31660.9750814f, 31726.8750814f, 31792.7750813f, 31858.6750813f,   32188.1750811f, 32254.0750811f, 32319.975081f,  32385.8750810f,   32451.7750810f, 32517.6750809f, 32847.1750808f, 32913.0750807f,   32978.9750807f, 33044.875080f,  33110.7750806f, 33176.67508062};
    int _exp2SFF[] = {4, 2, 10, 6, 6, 360, 36, 6, 1, 0, 1, 99,};
    NDArray<double> exp2FF(_exp2BFF, _exp2SFF);
    exp2FF.triggerAllocationFlag(false, false);

    NDArray<double> input('c', {2, 3, 10, 10});
    NDArray<double> weightsD('c', {2, 3, 5, 5});
    NDArray<double> weightsP('c', {10, 6, 1, 1});


    NDArrayFactory<double>::linspace(1, input);
    NDArrayFactory<double>::linspace(1, weightsD);
    NDArrayFactory<double>::linspace(1, weightsP);

    weightsP.template applyScalar<simdOps::Divide<double>>(10000.0);

    nd4j::ops::sconv2d<double> op;
    auto resultFF = op.execute({&input, &weightsD}, {}, {5, 5, 1, 1, 0, 0, 1, 1, 0});

    auto z = resultFF->at(0);
    //z->printShapeInfo("FF shape");

    ASSERT_TRUE(z->isSameShape(&expFF));

//    expFF.printBuffer("e");
    //z->printBuffer("z");
    ASSERT_TRUE(z->equalsTo(&expFF, 1));


    nd4j::ops::conv2d<double> op2d;
    auto result2D = op2d.execute({z, &weightsP}, {}, {1, 1, 1, 1, 0, 0, 1, 1, 0});

    auto z2d = result2D->at(0);
    //z2d->printShapeInfo("z2d shape");
    ASSERT_TRUE(z2d->isSameShape(&exp2FF));

    //exp2FF.printBuffer("e2d");
    //z2d->printBuffer("z2d");
    ASSERT_TRUE(z2d->equalsTo(&exp2FF));

    delete resultFF;
    delete result2D;
}

TEST_F(ConvolutionTests, sconv2D_FF_pointwise_1) {
    double _expBFF[] = {108.9405008,  109.5920008,  110.2435008,  110.8950008,   111.5465008,  112.1980008,  115.4555008,  116.1070008,   116.7585008,  117.410000,   118.061500,   118.7130009,   121.9705009,  122.6220009,  123.2735009,  123.9250009,   124.5765009,  125.2280009,  128.4855009,  129.1370009,   129.7885009,  130.4400009,  131.09150,    131.74300,   135.0005010,  135.6520010,  136.3035010,  136.9550010,   137.6065010,  138.2580010,  141.5155010,  142.1670010,   142.8185010,  143.4700010,  144.1215010,  144.7730010,   248.9617514,  250.670751,   252.3797515,  254.0887515,   255.7977515,  257.5067515,  266.0517515,  267.7607515,   269.469751,   271.1787516,  272.8877516,  274.5967516,   283.1417516,  284.8507516,  286.5597516,  288.268751,   289.9777517,  291.6867517,  300.2317517,  301.9407517,   303.6497517,  305.3587517,  307.067751,   308.7767518,   317.3217518,  319.0307518,  320.7397518,  322.4487518,   324.157751,   325.866751,   334.4117519,  336.1207519,   337.8297519,  339.5387519,  341.2477519,  342.95675,   388.9829964,  391.7494964,  394.5159964,  397.2824964,   400.048996,   402.8154963,  416.647996,   419.4144962,   422.1809962,  424.9474962,  427.7139962,  430.4804962,   444.3129961,  447.0794961,  449.8459961,  452.6124960,   455.3789960,  458.1454960,  471.9779959,  474.7444959,   477.5109959,  480.2774959,  483.0439959,  485.8104958,   499.6429958,  502.4094957,  505.1759957,  507.9424957,   510.7089957,  513.4754957,  527.3079956,  530.0744956,   532.8409956,  535.607495,   538.3739955,  541.1404955,   529.0042487,  532.8282487,  536.6522487,  540.4762487,   544.3002487,  548.1242487,  567.2442487,  571.068248,   574.892248,   578.716248,   582.540248,   586.3642486,   605.4842486,  609.3082486,  613.1322486,  616.9562486,   620.7802486,  624.6042486,  643.7242486,  647.5482486,   651.3722486,  655.1962486,  659.0202486,  662.8442486,   681.9642486,  685.7882486,  689.6122486,  693.4362486,   697.2602486,  701.0842486,  720.2042486,  724.0282486,   727.852248,   731.676248,   735.500248,   739.324248,   669.0255044,  673.9070044,  678.7885044,  683.6700044,   688.5515044,  693.4330044,  717.8405044,  722.7220044,   727.6035044,  732.4850044,  737.3665044,  742.2480044,   766.6555043,  771.5370043,  776.4185043,  781.3000043,   786.1815043,  791.0630043,  815.4705043,  820.3520043,   825.2335043,  830.1150043,  834.9965043,  839.8780043,   864.2855042,  869.1670042,  874.0485042,  878.9300042,   883.8115042,  888.6930042,  913.1005042,  917.9820042,   922.8635042,  927.7450042,  932.6265042,  937.5080042,   809.0467424,  814.9857424,  820.9247424,  826.8637423,   832.8027423,  838.7417423,  868.4367421,  874.3757421,   880.3147420,  886.2537420,  892.1927420,  898.13174,   927.8267418,  933.7657418,  939.7047417,  945.6437417,   951.5827417,  957.5217416,  987.2167415,  993.155741,   999.0947414, 1005.0337414, 1010.972741,  1016.9117413,   1046.6067412, 1052.5457411, 1058.4847411, 1064.4237411,   1070.3627410, 1076.3017410, 1105.996740,  1111.9357408,   1117.8747408, 1123.8137408, 1129.7527407, 1135.6917407,   949.0679815,  956.0644814,  963.060981,   970.0574813,   977.0539812,  984.0504811, 1019.0329807, 1026.0294807,   1033.0259806, 1040.0224805, 1047.0189804, 1054.0154804,   1088.9979800, 1095.9944799, 1102.9909798, 1109.987479,   1116.9839797, 1123.9804796, 1158.9629792, 1165.9594791,   1172.9559791, 1179.9524790, 1186.9489789, 1193.9454788,   1228.9279785, 1235.9244784, 1242.9209783, 1249.9174782,   1256.913978,  1263.9104781, 1298.8929777, 1305.8894776,   1312.8859775, 1319.8824775, 1326.8789774, 1333.8754773,   1089.0892560, 1097.1432561, 1105.1972562, 1113.251256,   1121.3052563, 1129.3592564, 1169.6292568, 1177.6832568,   1185.7372569, 1193.7912570, 1201.845257,  1209.8992571,   1250.1692575, 1258.2232576, 1266.2772576, 1274.3312577,   1282.3852578, 1290.4392579, 1330.7092582, 1338.7632583,   1346.8172584, 1354.8712584, 1362.9252585, 1370.9792586,   1411.24925,   1419.3032590, 1427.3572591, 1435.4112592,   1443.465259,  1451.5192593, 1491.7892597, 1499.8432598,   1507.8972598, 1515.9512599, 1524.0052600, 1532.059260,   1229.1105073, 1238.2220073, 1247.3335073, 1256.4450073,   1265.5565073, 1274.668007,  1320.2255074, 1329.3370074,   1338.4485074, 1347.5600075, 1356.6715075, 1365.7830075,   1411.340507,  1420.4520076, 1429.5635076, 1438.6750076,   1447.7865076, 1456.8980076, 1502.4555077, 1511.5670077,   1520.6785077, 1529.7900077, 1538.9015077, 1548.013007,   1593.5705078, 1602.6820078, 1611.793507,  1620.9050079,   1630.0165079, 1639.1280079, 1684.6855080, 1693.7970080,   1702.9085080, 1712.0200080, 1721.1315080, 1730.2430080,   1369.1317613, 1379.3007614, 1389.4697614, 1399.6387615,   1409.8077615, 1419.976761,  1470.8217618, 1480.9907618,   1491.159761,  1501.3287619, 1511.4977619, 1521.6667620,   1572.5117622, 1582.6807622, 1592.8497623, 1603.0187623,   1613.1877624, 1623.3567624, 1674.2017626, 1684.3707627,   1694.5397627, 1704.7087628, 1714.8777628, 1725.046762,   1775.8917631, 1786.0607631, 1796.229763,  1806.3987632,   1816.5677632, 1826.7367633, 1877.5817635, 1887.7507635,   1897.9197636, 1908.0887636, 1918.2577637, 1928.4267637,   304.3905022,  305.0420022,  305.6935022,  306.3450022,   306.9965022,  307.6480022,  310.9055022,  311.5570022,   312.208502,   312.860002,   313.5115023,  314.1630023,   317.4205023,  318.0720023,  318.7235023,  319.3750023,   320.0265023,  320.6780023,  323.9355023,  324.5870023,   325.2385023,  325.8900023,  326.541502,   327.193002,   330.4505024,  331.1020024,  331.7535024,  332.4050024,   333.0565024,  333.7080024,  336.9655024,  337.6170024,   338.2685024,  338.9200024,  339.5715024,  340.223002,   761.6617542,  763.3707542,  765.0797542,  766.7887542,   768.4977542,  770.206754,   778.7517543,  780.4607543,   782.1697543,  783.8787543,  785.5877543,  787.2967543,   795.8417544,  797.5507544,  799.2597544,  800.9687544,   802.6777544,  804.3867544,  812.9317545,  814.6407545,   816.3497545,  818.0587545,  819.7677545,  821.4767545,   830.0217546,  831.7307546,  833.4397546,  835.1487546,   836.8577546,  838.5667546,  847.1117547,  848.8207547,   850.5297547,  852.2387547,  853.9477547,  855.6567547,   1218.9329915, 1221.6994915, 1224.4659915, 1227.232491,   1229.9989914, 1232.7654914, 1246.5979913, 1249.3644913,   1252.1309913, 1254.8974913, 1257.6639913, 1260.430491,   1274.2629912, 1277.029491,  1279.7959911, 1282.5624911,   1285.3289911, 1288.0954911, 1301.9279910, 1304.6944910,   1307.4609910, 1310.22749,   1312.9939909, 1315.7604909,   1329.5929908, 1332.3594908, 1335.1259908, 1337.8924908,   1340.6589908, 1343.4254908, 1357.2579907, 1360.0244907,   1362.7909906, 1365.5574906, 1368.3239906, 1371.0904906,   1676.2042479, 1680.0282479, 1683.8522479, 1687.6762479,   1691.5002479, 1695.3242479, 1714.4442479, 1718.2682479,   1722.0922479, 1725.9162479, 1729.7402479, 1733.5642479,   1752.6842479, 1756.5082479, 1760.3322479, 1764.1562479,   1767.9802479, 1771.8042479, 1790.9242479, 1794.7482479,   1798.5722479, 1802.3962479, 1806.2202479, 1810.044247,   1829.1642478, 1832.9882478, 1836.8122478, 1840.6362478,   1844.4602478, 1848.2842478, 1867.4042478, 1871.2282478,   1875.0522478, 1878.8762478, 1882.7002478, 1886.5242478,   2133.4755029, 2138.3570029, 2143.2385029, 2148.1200029,   2153.0015029, 2157.8830029, 2182.2905028, 2187.1720028,   2192.0535028, 2196.9350028, 2201.8165028, 2206.6980028,   2231.1055028, 2235.9870028, 2240.8685028, 2245.7500028,   2250.6315028, 2255.5130028, 2279.9205027, 2284.8020027,   2289.6835027, 2294.5650027, 2299.4465027, 2304.3280027,   2328.7355027, 2333.6170027, 2338.4985027, 2343.3800027,   2348.2615027, 2353.1430027, 2377.5505026, 2382.4320026,   2387.3135026, 2392.1950026, 2397.0765026, 2401.9580026,   2590.7467330, 2596.6857330, 2602.6247329, 2608.5637329,   2614.5027329, 2620.441732,  2650.1367327, 2656.0757327,   2662.0147326, 2667.9537326, 2673.8927326, 2679.8317325,   2709.5267324, 2715.465732,  2721.4047323, 2727.3437323,   2733.282732,  2739.2217322, 2768.9167321, 2774.8557320,   2780.7947320, 2786.7337320, 2792.6727319, 2798.6117319,   2828.306731,  2834.2457317, 2840.1847317, 2846.1237317,   2852.0627316, 2858.0017316, 2887.6967314, 2893.6357314,   2899.5747314, 2905.5137313, 2911.4527313, 2917.3917313,   3048.0179587, 3055.0144586, 3062.0109585, 3069.0074584,   3076.0039584, 3083.0004583, 3117.9829579, 3124.9794578,   3131.9759578, 3138.9724577, 3145.9689576, 3152.9654575,   3187.947957,  3194.9444571, 3201.9409570, 3208.9374569,   3215.933956,  3222.9304568, 3257.9129564, 3264.9094563,   3271.9059562, 3278.9024562, 3285.8989561, 3292.8954560,   3327.8779556, 3334.874455,  3341.8709555, 3348.8674554,   3355.8639553, 3362.860455,  3397.8429549, 3404.8394548,   3411.8359547, 3418.8324546, 3425.8289546, 3432.8254545,   3505.28927,   3513.3432780, 3521.3972781, 3529.4512782,   3537.5052782, 3545.5592783, 3585.8292787, 3593.8832788,   3601.9372788, 3609.9912789, 3618.0452790, 3626.099279,   3666.3692794, 3674.4232795, 3682.4772796, 3690.5312796,   3698.5852797, 3706.6392798, 3746.9092801, 3754.9632802,   3763.0172803, 3771.0712804, 3779.1252804, 3787.1792805,   3827.4492809, 3835.50328,   3843.5572810, 3851.6112811,   3859.6652812, 3867.7192812, 3907.9892816, 3916.0432817,   3924.097281,  3932.1512818, 3940.2052819, 3948.2592820,   3962.5605113, 3971.6720113, 3980.783511,  3989.8950114,   3999.0065114, 4008.1180114, 4053.6755115, 4062.7870115,   4071.8985115, 4081.0100115, 4090.1215115, 4099.2330115,   4144.7905116, 4153.9020116, 4163.0135116, 4172.1250116,   4181.236511,  4190.3480117, 4235.9055117, 4245.0170117,   4254.128511,  4263.2400118, 4272.3515118, 4281.4630118,   4327.0205119, 4336.1320119, 4345.2435119, 4354.3550119,   4363.4665119, 4372.5780119, 4418.1355120, 4427.2470120,   4436.3585120, 4445.4700120, 4454.581512,  4463.6930121,   4419.8317743, 4430.0007744, 4440.1697744, 4450.338774,   4460.5077745, 4470.6767745, 4521.521774,  4531.6907748,   4541.8597748, 4552.0287749, 4562.1977749, 4572.3667750,   4623.2117752, 4633.3807752, 4643.5497753, 4653.7187753,   4663.8877754, 4674.0567754, 4724.9017756, 4735.0707757,   4745.2397757, 4755.4087757, 4765.5777758, 4775.7467758,   4826.591776,  4836.7607761, 4846.9297761, 4857.0987762,   4867.2677762, 4877.4367763, 4928.2817765, 4938.4507765,   4948.6197766, 4958.7887766, 4968.957776,  4979.12677675};
    int _expSFF[] = {4, 2, 10, 6, 6, 360, 36, 6, 1, 0, 1, 99,};
    NDArray<double> expFF(_expBFF, _expSFF);
    expFF.triggerAllocationFlag(false, false);

    NDArray<double> input('c', {2, 3, 10, 10});
    NDArray<double> weightsD('c', {5, 3, 5, 5});
    NDArray<double> weightsP('c', {10, 15, 1, 1});

    NDArrayFactory<double>::linspace(1, input);
    NDArrayFactory<double>::linspace(1, weightsD);
    NDArrayFactory<double>::linspace(1, weightsP);

    input.template applyScalar<simdOps::Divide<double>>(100.0);
    weightsD.template applyScalar<simdOps::Divide<double>>(100.0);
    weightsP.template applyScalar<simdOps::Divide<double>>(100.0);

    nd4j::ops::sconv2d<double> op;

    auto resultFF = op.execute({&input, &weightsD, &weightsP}, {}, {5, 5, 1, 1, 0, 0, 1, 1, 0});

    auto z = resultFF->at(0);
    //z->printShapeInfo("FF shape");


    ASSERT_TRUE(z->isSameShape(&expFF));

    //expFF.printBuffer("e");
    //z->printBuffer("z");
    ASSERT_TRUE(z->equalsTo(&expFF, 1e-3));

    delete resultFF;
}


TEST_F(ConvolutionTests, sconv2D_BP_pointwise_1) {
    double _expGradWpB[] = {1603.7102981f,  10645.6278024f,   5975.4227995f,  17697.0903052f,    12133.6353024f,  26535.0528052f,   1779.221097f,   11795.5686029f,    6721.9835994f,  19904.0811062f,  13775.2461029f,  30123.0936062f,    1954.7318976f,  12945.5094033f,   7468.5443993f,  22111.071907f,    15416.8569033f,  33711.134407f,    2130.2426974f,  14095.4502038f,    8215.1051992f,  24318.0627081f,  17058.4677038f,  37299.1752081f,    2305.7534972f,  15245.3910042f,   8961.6659991f,  26525.0535091f,    18700.0785042f,  40887.2160091f,   2481.2642970f,  16395.3318047f,    9708.2267991f,  28732.0443100f,  20341.6893047f,  44475.2568100f,    2656.7750968f,  17545.2726051f,  10454.7875990f,  30939.0351110f,    21983.3001051f,  48063.2976110f,   2832.2858966f,  18695.2134056f,    11201.3483989f,  33146.0259119f,  23624.9109056f,  51651.3384119f,    3007.7966964f,  19845.1542060f,  11947.9091988f,  35353.0167129f,    25266.5217060f,  55239.3792129f,   3183.3074962f,  20995.095006f,    12694.4699987f,  37560.007513f,   26908.132506f,   58827.4200139};
    int _expGradWpS[] {4, 10, 6, 1, 1, 6, 1, 1, 1, 0, 1, 99};
    NDArray<double> expGWP(_expGradWpB, _expGradWpS);
    expGWP.triggerAllocationFlag(false, false);


    double _expGradWdB[] = {2074.21032f, 2082.76104f, 2091.31176f, 2099.86248f, 2108.4132f, 2159.71752f, 2168.26824f, 2176.81896f, 2185.36968f, 2193.9204f, 2245.22472f, 2253.77544f, 2262.32616f, 2270.87688f, 2279.4276f, 2330.73192f, 2339.28264f, 2347.83336f, 2356.38408f, 2364.9348f, 2416.23912f, 2424.78984f, 2433.34056f, 2441.89128f, 2450.442f, 3112.99344f, 3122.06328f, 3131.13312f, 3140.20296f, 3149.2728f, 3203.69184f, 3212.76168f, 3221.83152f, 3230.90136f, 3239.9712f, 3294.39024f, 3303.46008f, 3312.52992f, 3321.59976f, 3330.6696f, 3385.08864f, 3394.15848f, 3403.22832f, 3412.29816f, 3421.368f, 3475.78704f, 3484.85688f, 3493.92672f, 3502.99656f, 3512.0664f, 4255.60056f, 4265.18952f, 4274.77848f, 4284.36744f, 4293.9564f, 4351.49016f, 4361.07912f, 4370.66808f, 4380.25704f, 4389.846f, 4447.37976f, 4456.96872f, 4466.55768f, 4476.14664f, 4485.7356f, 4543.26936f, 4552.85832f, 4562.44728f, 4572.03624f, 4581.6252f, 4639.15896f, 4648.74792f, 4658.33688f, 4667.92584f, 4677.5148f, 2140.10988f, 2148.92016f, 2157.73044f, 2166.54072f, 2175.351f, 2228.21268f, 2237.02296f, 2245.83324f, 2254.64352f, 2263.4538f, 2316.31548f, 2325.12576f, 2333.93604f, 2342.74632f, 2351.5566f, 2404.41828f, 2413.22856f, 2422.03884f, 2430.84912f, 2439.6594f, 2492.52108f, 2501.33136f, 2510.14164f, 2518.95192f, 2527.7622f, 3204.849f, 3214.1784f, 3223.5078f, 3232.8372f, 3242.1666f, 3298.143f, 3307.4724f, 3316.8018f, 3326.1312f, 3335.4606f, 3391.437f, 3400.7664f, 3410.0958f, 3419.4252f, 3428.7546f, 3484.731f, 3494.0604f, 3503.3898f, 3512.7192f, 3522.0486f, 3578.025f, 3587.3544f, 3596.6838f, 3606.0132f, 3615.3426f, 4373.41212f, 4383.26064f, 4393.10916f, 4402.95768f, 4412.8062f, 4471.89732f, 4481.74584f, 4491.59436f, 4501.44288f, 4511.2914f, 4570.38252f, 4580.23104f, 4590.07956f, 4599.92808f, 4609.7766f, 4668.86772f, 4678.71624f, 4688.56476f, 4698.41328f, 4708.2618f, 4767.35292f, 4777.20144f, 4787.04996f, 4796.89848f, 4806.747};
    int _expGradWdS[] = {4, 2, 3, 5, 5, 75, 25, 5, 1, 0, 1, 99};
    NDArray<double> expGWD(_expGradWdB, _expGradWdS);
    expGWD.triggerAllocationFlag(false, false);


    double _expEB[] = {5.0103f, 10.17147f, 15.48408f, 20.9487f, 26.5659f, 26.6832f, 21.65628f, 16.47507f, 11.139f, 5.6475f, 10.79727f, 21.90255f, 33.31698f, 45.0417f, 57.07785f, 57.3267f, 46.49334f, 35.34513f, 23.88093f, 12.0996f, 17.37801f, 35.22744f, 53.55f, 72.3474f, 91.62135f, 92.016f, 74.57958f, 56.66148f, 38.25999f, 19.3734f, 24.76962f, 50.18034f, 76.23444f, 102.9342f, 130.2819f, 130.8366f, 105.9834f, 80.47542f, 54.31038f, 27.486f, 32.9892f, 66.79545f, 101.4216f, 136.8705f, 173.145f, 173.874f, 140.7732f, 106.83825f, 72.0663f, 36.4545f, 33.8298f, 68.49375f, 103.9947f, 140.3355f, 177.519f, 178.248f, 144.3066f, 109.51395f, 73.8672f, 37.3635f, 28.85658f, 58.39302f, 88.6116f, 119.5146f, 151.1043f, 151.716f, 122.76444f, 93.11934f, 62.77842f, 31.7394f, 23.00409f, 46.52748f, 70.57188f, 95.139f, 120.23055f, 120.7107f, 97.6311f, 74.02194f, 49.88151f, 25.2081f, 16.25523f, 32.86293f, 49.82424f, 67.1403f, 84.81225f, 85.1466f, 68.83818f, 52.17045f, 35.14227f, 17.7525f, 8.5929f, 17.36517f, 26.31738f, 35.4501f, 44.7639f, 44.9382f, 36.31728f, 27.51357f, 18.5265f, 9.3555f, 8.63807f, 17.45032f, 26.43736f, 35.5998f, 44.93825f, 45.1399f, 36.46882f, 27.6199f, 18.59253f, 9.3861f, 18.18615f, 36.72737f, 55.62488f, 74.8799f, 94.49365f, 94.9122f, 76.65698f, 58.03937f, 39.05815f, 19.7121f, 28.66254f, 57.86775f, 87.61746f, 117.9135f, 148.7577f, 149.4084f, 120.63768f, 91.31331f, 61.43346f, 30.9963f, 40.08554f, 80.90806f, 122.47f, 164.7738f, 207.8219f, 208.72f, 168.48412f, 127.49662f, 85.75506f, 43.257f, 52.47345f, 105.8849f, 160.2374f, 215.534f, 271.77775f, 272.9385f, 220.2695f, 166.6442f, 112.05955f, 56.5125f, 53.82975f, 108.6158f, 164.3612f, 221.069f, 278.74225f, 279.903f, 225.8777f, 170.8778f, 114.90025f, 57.942f, 45.14002f, 91.0585f, 137.75788f, 185.2406f, 233.5091f, 234.4682f, 189.16564f, 143.06998f, 96.17878f, 48.4896f, 35.43048f, 71.45487f, 108.075f, 145.2927f, 183.1098f, 183.852f, 148.29504f, 112.13319f, 75.36462f, 37.9875f, 24.68283f, 49.76831f, 75.25766f, 101.1521f, 127.45285f, 127.9629f, 103.1927f, 78.01253f, 52.42117f, 26.4174f, 12.87877f, 25.96222f, 39.25096f, 52.7456f, 66.44675f, 66.7094f, 53.78542f, 40.6531f, 27.31183f, 13.761f, 12.59184f, 25.38317f, 38.37464f, 51.5669f, 64.9606f, 65.2566f, 52.61336f, 39.76673f, 26.71606f, 13.4607f, 26.23903f, 52.88419f, 79.93678f, 107.3981f, 135.26945f, 135.8777f, 109.53262f, 82.77361f, 55.59937f, 28.0086f, 40.96107f, 82.54206f, 124.74492f, 167.5716f, 211.02405f, 211.9608f, 170.83578f, 129.07914f, 86.68893f, 43.6632f, 56.77746f, 114.39578f, 172.85756f, 232.1654f, 292.3219f, 293.6034f, 236.60084f, 178.74182f, 120.02374f, 60.444f, 73.7077f, 148.48435f, 224.3332f, 301.2575f, 379.2605f, 380.903f, 306.9058f, 231.82015f, 155.6428f, 78.3705f, 75.6397f, 152.36785f, 230.1877f, 309.1025f, 389.1155f, 390.758f, 314.8288f, 237.79165f, 159.6433f, 80.3805f, 62.89546f, 126.67598f, 191.34416f, 256.9026f, 323.3539f, 324.7004f, 261.56684f, 197.53262f, 132.59514f, 66.7518f, 48.97887f, 98.63226f, 148.96212f, 199.9704f, 251.65905f, 252.6933f, 203.53098f, 153.68244f, 103.14573f, 51.9189f, 33.87043f, 68.19769f, 102.98308f, 138.2279f, 173.93345f, 174.6392f, 140.64322f, 106.18261f, 71.25607f, 35.8623f, 17.55064f, 35.33327f, 53.34854f, 71.5971f, 90.0796f, 90.4406f, 72.82556f, 54.97463f, 36.88716f, 18.5625f, 13.0455f, 26.44707f, 40.20528f, 54.3207f, 68.7939f, 68.9112f, 55.84908f, 42.42747f, 28.6458f, 14.5035f, 27.89367f, 56.50575f, 85.83738f, 115.8897f, 146.66385f, 146.9127f, 118.98294f, 90.32793f, 60.94653f, 30.8376f, 44.56161f, 90.21024f, 136.9476f, 184.7754f, 233.69535f, 234.09f, 189.46998f, 143.75268f, 96.93639f, 49.0194f, 63.06642f, 127.59474f, 193.58724f, 261.0462f, 329.9739f, 330.5286f, 267.3786f, 202.75302f, 136.64958f, 69.066f, 83.4252f, 168.69345f, 255.8076f, 344.7705f, 435.585f, 436.314f, 352.7772f, 267.38025f, 180.1203f, 90.9945f, 84.2658f, 170.39175f, 258.3807f, 348.2355f, 439.959f, 440.688f, 356.3106f, 270.05595f, 181.9212f, 91.9035f, 71.25738f, 144.01542f, 218.2764f, 294.0426f, 371.3163f, 371.928f, 300.57564f, 227.70894f, 153.32562f, 77.4234f, 56.34369f, 113.82228f, 172.43748f, 232.191f, 293.08455f, 293.5647f, 237.1455f, 179.58114f, 120.86991f, 61.0101f, 39.50763f, 79.77813f, 120.81264f, 162.6123f, 205.17825f, 205.5126f, 165.95178f, 125.62125f, 84.51987f, 42.6465f, 20.7321f, 41.84877f, 63.35058f, 85.2381f, 107.5119f, 107.6862f, 86.92608f, 65.77797f, 44.2413f, 22.3155f, 22.71767f, 45.82912f, 69.33496f, 93.2358f, 117.53225f, 117.7339f, 94.98322f, 71.8351f, 48.28893f, 24.3441f, 47.44335f, 95.68097f, 144.71408f, 194.5439f, 245.17165f, 245.5902f, 198.07778f, 149.76377f, 100.64695f, 50.7261f, 74.19534f, 149.59215f, 226.19226f, 303.9975f, 383.0097f, 383.6604f, 309.35688f, 233.84091f, 157.11066f, 79.1643f, 102.99194f, 207.59926f, 313.8244f, 421.6698f, 531.1379f, 532.036f, 428.89372f, 324.12142f, 217.71666f, 109.677f, 133.85145f, 269.7389f, 407.6654f, 547.634f, 689.64775f, 690.8085f, 556.7615f, 420.6602f, 282.50155f, 142.2825f, 135.20775f, 272.4698f, 411.7892f, 553.169f, 696.61225f, 697.773f, 562.3697f, 424.8938f, 285.34225f, 143.712f, 112.43842f, 226.5337f, 342.28828f, 459.7046f, 578.7851f, 579.7442f, 467.14324f, 352.87078f, 236.92438f, 119.3016f, 87.55128f, 176.35527f, 266.4138f, 357.7287f, 450.3018f, 451.044f, 363.36624f, 274.42479f, 184.21782f, 92.7435f, 60.52803f, 121.89791f, 184.11086f, 247.1681f, 311.07085f, 311.5809f, 250.9655f, 189.50093f, 127.18597f, 64.0194f, 31.35037f, 63.12502f, 95.32456f, 127.9496f, 161.00075f, 161.2634f, 129.86782f, 98.0443f, 65.79223f, 33.111f, 33.43584f, 67.30517f, 101.60864f, 136.3469f, 171.5206f, 171.8166f, 138.32936f, 104.40473f, 70.04206f, 35.2407f, 69.09703f, 139.06819f, 209.91478f, 281.6381f, 354.23945f, 354.8477f, 285.64462f, 215.55961f, 144.59137f, 72.7386f, 107.00307f, 215.32806f, 324.97692f, 435.9516f, 548.25405f, 549.1908f, 442.02378f, 333.52314f, 223.68693f, 112.5132f, 147.17346f, 296.12378f, 446.85356f, 599.3654f, 753.6619f, 754.9434f, 607.54484f, 458.35382f, 307.36774f, 154.584f, 189.6277f, 381.49435f, 575.6032f, 771.9575f, 970.5605f, 972.203f, 782.2858f, 590.11015f, 395.6728f, 198.9705f, 191.5597f, 385.37785f, 581.4577f, 779.8025f, 980.4155f, 982.058f, 790.2088f, 596.08165f, 399.6733f, 200.9805f, 157.97146f, 317.76398f, 479.38016f, 642.8226f, 808.0939f, 809.4404f, 651.23084f, 491.18462f, 329.29914f, 165.5718f, 122.04087f, 245.45826f, 370.25412f, 496.4304f, 623.98905f, 625.0233f, 502.79898f, 379.18644f, 254.18373f, 127.7889f, 83.74843f, 168.42169f, 254.02108f, 340.5479f, 428.00345f, 428.7092f, 344.83522f, 260.02861f, 174.28807f, 87.6123f, 43.07464f, 86.61527f, 130.62254f, 175.0971f, 220.0396f, 220.4006f, 177.26156f, 133.65263f, 89.57316f, 45.0225f };
    int _expES[] = {4, 2, 3, 10, 10, 300, 100, 10, 1, 0, 1, 99};
    NDArray<double> expE(_expEB, _expES);
    expE.triggerAllocationFlag(false, false);

    NDArray<double> input('c', {2, 3, 10, 10});
    NDArray<double> weightsD('c', {2, 3, 5, 5});
    NDArray<double> weightsP('c', {10, 6, 1, 1});

    NDArray<double> epsilon('c', {2, 3, 10, 10});
    NDArray<double> epsilonNext('c', {2, 10, 6, 6});

    NDArrayFactory<double>::linspace(1, input);
    NDArrayFactory<double>::linspace(1, weightsD);
    NDArrayFactory<double>::linspace(1, weightsP);
    NDArrayFactory<double>::linspace(1, epsilonNext);

    input.template applyScalar<simdOps::Divide<double>>(100.0);
    weightsD.template applyScalar<simdOps::Divide<double>>(100.0);
    weightsP.template applyScalar<simdOps::Divide<double>>(100.0);
    epsilonNext.template applyScalar<simdOps::Divide<double>>(100.0);

    nd4j::ops::sconv2d_bp<double> op;
    auto resultBP = op.execute({&input, &epsilonNext, &weightsD, &weightsP },{}, {5, 5, 1, 1, 0, 0, 1, 1, 0});

    ASSERT_EQ(3, resultBP->size());

    auto _epsilon = resultBP->at(0);
    auto _gradWD = resultBP->at(1);
    auto _gradWP = resultBP->at(2);

    //_gradWP->printBuffer("gradWP");

    ASSERT_TRUE(_gradWP->isSameShape(&expGWP));
    ASSERT_TRUE(_gradWP->isSameShape(&weightsP));

    ASSERT_TRUE(_gradWP->equalsTo(&expGWP));

    //_gradWD->printShapeInfo("gradWD shape");
    //_gradWD->printBuffer("gradWD");

    ASSERT_TRUE(_gradWD->isSameShape(&expGWD));
    ASSERT_TRUE(_gradWD->isSameShape(&weightsD));

    ASSERT_TRUE(_gradWD->equalsTo(&expGWD));

    ASSERT_TRUE(_epsilon->isSameShape(&input));
    ASSERT_TRUE(_epsilon->isSameShape(&expE));

    ASSERT_TRUE(_epsilon->equalsTo(&expE));

    delete resultBP;
}

TEST_F(ConvolutionTests, TestSconvCrash_max_1) {
    NDArray<double> input('c', {3, 3, 8, 8});
    NDArray<double> weightsD('c', {1, 3, 1, 1});
    NDArray<double> weightsP('c', {2, 3, 1, 1});
    NDArray<double> bias('c', {1, 2});
    NDArray<double> output('c', {3, 2, 8, 8});
    output.assign(0.0);

    NDArrayFactory<double>::linspace(1, input);
    NDArrayFactory<double>::linspace(1, weightsD);
    NDArrayFactory<double>::linspace(1, weightsP);
    NDArrayFactory<double>::linspace(1, bias);

    NDArray<double> expOutput('c', {3, 2, 8, 8});

    nd4j::ops::sconv2d<double> op;
    Nd4jStatus status = op.execute({&input, &weightsD, &weightsP, &bias}, {&output}, {},  {1, 1, 1, 1, 0, 0, 1, 1, 0});
    auto result = op.execute({&input, &weightsD, &weightsP, &bias}, {},  {1, 1, 1, 1, 0, 0, 1, 1, 0});

    auto z = result->at(0);

    //printf("\n");
    //output.printBuffer("output");
    //z->printBuffer("z");


    //ASSERT_TRUE(expOutput.isSameShape(z));

    delete result;
}

TEST_F(ConvolutionTests, Test_im2col_col2im_1) {
    int kY = 5;
    int kX = 5;
    int sY = 1;
    int sX = 1;
    int pY = 0;
    int pX = 0;
    int dY = 1;
    int dX = 1;
    int inY = 28;
    int inX = 28;

    bool isSameMode = true;

    NDArray<double> x('c', {2, 1, inY, inX});
    NDArrayFactory<double>::linspace(1, x);

    int oY, oX;

    nd4j::ops::ConvolutionUtils<double>::calcOutHWpool2D(oY, oX, kY, kX, sY, sX, pY, pX, dY, dX, inY, inX, isSameMode);

    if (isSameMode)
        nd4j::ops::ConvolutionUtils<double>::_calcPadding2D(pY, pX, oY, oX, inY, inX, kY, kX, sY, sX, dY, dX);

    NDArray<double> im2col0('c', {2, 1, kY, kX, oY, oX});

    std::vector<double> args2col({(double) kY, (double) kX, (double) sY, (double) sX, (double) pY, (double) pX, (double) dY, (double) dX, isSameMode ? (double) 1 : (double) 0});
    x.template applyTransform<simdOps::Im2col<double>>(&im2col0, args2col.data());

    nd4j::ops::im2col<double> op;
    auto result2col = op.execute({&x}, {}, {kY, kX, sY, sX, pY, pX, dY, dX, isSameMode ? 1 : 0});

    auto im2col1 = result2col->at(0);

    ASSERT_TRUE(im2col1->isSameShape(&im2col0));
    ASSERT_TRUE(im2col1->equalsTo(&im2col0));


    std::vector<double> args2im({ (double) sY, (double) sX, (double) pY, (double) pX, (double) inY, (double) inX, (double) dY, (double) dX, isSameMode ? (double) 1 : (double) 0});
    NDArray<double> col2im0('c', {2, 1, inY, inX});
    im2col0.template applyTransform<simdOps::Col2Im<double>>(&col2im0, args2im.data());

    nd4j::ops::col2im<double> op2im;
    auto result2im = op2im.execute({im2col1}, {}, {sY, sX, pY, pX, inY, inX, dY, dX, isSameMode ? 1 : 0});
    auto col2im1 = result2im->at(0);

    ASSERT_TRUE(col2im1->isSameShape(&col2im0));
    ASSERT_TRUE(col2im1->equalsTo(&col2im0));

    delete result2col;
    delete result2im;
}

TEST_F(ConvolutionTests, TestSconvCrash_max_2) {
    NDArray<double> input('c', {3, 3, 16, 16});
    NDArray<double> weightsD('c', {1, 3, 2, 2});
    NDArray<double> weightsP('c', {2, 3, 1, 1});
    NDArray<double> bias('c', {1, 2});

    NDArray<double> epsilonNext('c', {3, 2, 14, 14});

    NDArray<double> epsilon('c', {3, 3, 16, 16});

    nd4j::ops::sconv2d_bp<double> op;
    auto result = op.execute({&input, &epsilonNext, &weightsD, &weightsP}, {}, {2, 2, 1, 1, 0, 0, 2, 2, 0});

    auto eps = result->at(0);
    auto gWD = result->at(1);
    auto gWP = result->at(2);


    ASSERT_TRUE(epsilon.isSameShape(eps));

    delete result;
}

TEST_F(ConvolutionTests, TestDeconv_bp_1) {
    double _expb[] = {  35.f,   38.f,   41.f,   44.f,   47.f,   50.f,   53.f,   56.f,   59.f,   62.f,   65.f,    68.f,   71.f,   74.f,   77.f,   80.f,   71.f,   78.f,   85.f,   92.f,   99.f,  106.f,    113.f,  120.f,  127.f,  134.f,  141.f,  148.f,  155.f,  162.f,  169.f,  176.f,  107.f,    118.f,  129.f,  140.f,  151.f,  162.f,  173.f,  184.f,  195.f,  206.f,  217.f,  228.f,    239.f,  250.f,  261.f,  272.f,  131.f,  134.f,  137.f,  140.f,  143.f,  146.f,  149.f,    152.f,  155.f,  158.f,  161.f,  164.f,  167.f,  170.f,  173.f,  176.f,  295.f,  302.f,    309.f,  316.f,  323.f,  330.f,  337.f,  344.f,  351.f,  358.f,  365.f,  372.f,  379.f,    386.f,  393.f,  400.f,  459.f,  470.f,  481.f,  492.f,  503.f,  514.f,  525.f,  536.f,    547.f,  558.f,  569.f,  580.f,  591.f,  602.f,  613.f,  624.f,  227.f,  230.f,  233.f,    236.f,  239.f,  242.f,  245.f,  248.f,  251.f,  254.f,  257.f,  260.f,  263.f,  266.f,    269.f,  272.f,  519.f,  526.f,  533.f,  540.f,  547.f,  554.f,  561.f,  568.f,  575.f,    582.f,  589.f,  596.f,  603.f,  610.f,  617.f,  624.f,  811.f,  822.f,  833.f,  844.f,    855.f,  866.f,  877.f,  888.f,  899.f,  910.f,  921.f,  932.f,  943.f,  954.f,  965.f,    976.f,};
    NDArray<double> expEpsilon('c', {3, 3, 4, 4});
    expEpsilon.setBuffer(_expb);

    double _expwb[] = { 160008.f,  203400.f,  191112.f,  246792.f,  222216.f,  290184.f,};
    NDArray<double> expGradW('c', {3, 2, 1, 1});
    expGradW.setBuffer(_expwb);

    double _expbb[] = {1944.f,  2712.f};
    NDArray<double> expGradB('c', {1, 2});
    expGradB.setBuffer(_expbb);

    NDArray<double> input('c', {3, 3, 4, 4});
    NDArray<double> bias('c', {1, 2});
    NDArray<double> weights('c',{3, 2, 1, 1});
    NDArray<double> epsilon('c', {3, 2, 4, 4});

    /*
        Input shape (3, 3, 4, 4)
        Weights shape (3, 2, 1, 1)
        Epsilon shape (3, 2, 4, 4)
     */

    NDArrayFactory<double>::linspace(1, input);
    NDArrayFactory<double>::linspace(1, weights);
    NDArrayFactory<double>::linspace(1, bias);
    NDArrayFactory<double>::linspace(1, epsilon);

    nd4j::ops::deconv2d_bp<double> op;

    auto result = op.execute({&input, &weights, &bias, &epsilon}, {}, {1, 1, 1, 1, 0, 0, 1, 1, 1});

    ASSERT_EQ(ND4J_STATUS_OK, result->status());

    auto expNext = result->at(0);

    ASSERT_TRUE(expEpsilon.isSameShape(expNext));
    ASSERT_TRUE(expEpsilon.equalsTo(expNext));

    auto gradW = result->at(1);

    ASSERT_TRUE(expGradW.isSameShape(gradW));
    ASSERT_TRUE(expGradW.equalsTo(gradW));

    auto gradB = result->at(2);

    ASSERT_TRUE(expGradB.isSameShape(gradB));
    ASSERT_TRUE(expGradB.equalsTo(gradB));

    delete result;
}

TEST_F(ConvolutionTests, TestDeconv_bp_2) {
    /*
     Input shape:
    [3, 3, 14, 14]
    Output shape:
    [3, 2, 15, 15]
    Weights shape:
    [3, 2, 2, 2]
    Bias shape:
    [1, 2]
    weight shape:
    [3, 2, 2, 2]
    weight grad shape:
    [3, 2, 2, 2]
    bias grad shape:
    [1, 2]
    input epsilon shape:
    [3, 2, 15, 15]
    output epsilon shape:
    [3, 3, 14, 14]
     */
/*
    NDArray<double> input('c', {3, 3, 14, 14});
    NDArray<double> bias('c', {1, 2});
    NDArray<double> weights('c',{3, 2, 2, 2});
    NDArray<double> epsilon('c', {3, 2, 15, 15});


    NDArrayFactory<double>::linspace(1, input);
    NDArrayFactory<double>::linspace(1, weights);
    NDArrayFactory<double>::linspace(1, bias);
    NDArrayFactory<double>::linspace(1, epsilon);

    nd4j::ops::deconv2d_bp<double> op;

    auto result = op.execute({&input, &weights, &bias, &epsilon}, {}, {2, 2, 1, 1, 0, 0, 2, 2, 0});
    ASSERT_EQ(ND4J_STATUS_OK, result->status());


    delete result;*/
}

TEST_F(ConvolutionTests, TestDeconv_ff_2) {

    double expB[] = {218.f,   227.f,   236.f,   245.f,   254.f,   263.f,   272.f,   281.f,   290.f,   299.f,    308.f,   317.f,   326.f,   335.f,   344.f,   353.f,   270.f,   282.f,   294.f,   306.f,    318.f,   330.f,   342.f,   354.f,   366.f,   378.f,   390.f,   402.f,   414.f,   426.f,    438.f,   450.f,   650.f,   659.f,   668.f,   677.f,   686.f,   695.f,   704.f,   713.f,    722.f,   731.f,   740.f,   749.f,   758.f,   767.f,   776.f,   785.f,   846.f,   858.f,    870.f,   882.f,   894.f,   906.f,   918.f,   930.f,   942.f,   954.f,   966.f,   978.f,    990.f,  1002.f,  1014.f,  1026.f,  1082.f,  1091.f,  1100.f,  1109.f,  1118.f,  1127.f,    1136.f,  1145.f,  1154.f,  1163.f,  1172.f,  1181.f,  1190.f,  1199.f,  1208.f,  1217.f,    1422.f,  1434.f,  1446.f,  1458.f,  1470.f,  1482.f,  1494.f,  1506.f,  1518.f,  1530.f,    1542.f,  1554.f,  1566.f,  1578.f,  1590.f,  1602.f,};

    NDArray<double> exp('c', {3, 2, 4, 4});
    exp.setBuffer(expB);

    NDArray<double> input('c', {3, 3, 4, 4});
    NDArray<double> weights('c',{3, 2, 1, 1});
    NDArray<double> bias('c', {1, 2});

    NDArrayFactory<double>::linspace(1, input);
    NDArrayFactory<double>::linspace(1, weights);
    NDArrayFactory<double>::linspace(1, bias);

    nd4j::ops::deconv2d<double> op;

    auto result = op.execute({&input, &weights, &bias}, {}, {1, 1, 1, 1, 0, 0, 1, 1, 1});

    ASSERT_EQ(ND4J_STATUS_OK, result->status());

    auto output = result->at(0);

    ASSERT_TRUE(exp.isSameShape(output));
    ASSERT_TRUE(exp.equalsTo(output));

    delete result;
}

TEST_F(ConvolutionTests, Test_Conv1D_ff_1) {
    NDArray<double> input('c', {2, 2, 6});
    NDArray<double> weights('c', {3, 2, 2});
    NDArray<double> bias('c', {1, 3});
    NDArray<double> expFF('c', {2, 3, 5}, {59.0, 69.0, 79.0, 89.0, 99.0, 132.0, 158.0, 184.0, 210.0, 236.0, 205.0, 247.0, 289.0, 331.0, 373.0, 179.0, 189.0, 199.0, 209.0, 219.0, 444.0, 470.0, 496.0, 522.0, 548.0, 709.0, 751.0, 793.0, 835.0, 877.0});
    NDArray<double> expEps('c', {2, 2, 6}, {130.0, 293.0, 326.0, 359.0, 392.0, 220.0, 166.0, 371.0, 416.0, 461.0, 506.0, 280.0, 355.0, 788.0, 821.0, 854.0, 887.0, 490.0, 481.0, 1046.0, 1091.0, 1136.0, 1181.0, 640.0});
    NDArray<double> expGW('c', {3, 2, 2}, {1415.0, 1520.0, 2045.0, 2150.0, 1865.0, 2020.0, 2795.0, 2950.0, 2315.0, 2520.0, 3545.0, 3750.0});
    NDArray<double> expGB('c', {1, 3}, {105.0, 155.0, 205.0});


    NDArrayFactory<double>::linspace(1, input);
    NDArrayFactory<double>::linspace(1, weights);
    NDArrayFactory<double>::linspace(1, bias);

    nd4j::ops::conv1d<double> op;
    auto result_FF = op.execute({&input, &weights, &bias}, {}, {2, 1, 0});

    ASSERT_EQ(ND4J_STATUS_OK, result_FF->status());

    auto z = result_FF->at(0);


    ASSERT_TRUE(expFF.isSameShape(z));
    ASSERT_TRUE(expFF.equalsTo(z));


    nd4j::ops::conv1d_bp<double> op_bp;

    auto epsilonNxt = z->dup();
    NDArrayFactory<double>::linspace(1, *epsilonNxt);

    auto result_BP = op_bp.execute({&input, &weights, &bias, epsilonNxt}, {}, {2, 1, 0});
    ASSERT_EQ(ND4J_STATUS_OK, result_BP->status());

    auto eps = result_BP->at(0);
    auto gradW = result_BP->at(1);
    auto gradB = result_BP->at(2);

    ASSERT_TRUE(expEps.isSameShape(eps));
    ASSERT_TRUE(expGW.isSameShape(gradW));
    ASSERT_TRUE(expGB.isSameShape(gradB));

    ASSERT_TRUE(expEps.equalsTo(eps));
    ASSERT_TRUE(expGW.equalsTo(gradW));
    ASSERT_TRUE(expGB.equalsTo(gradB));

    delete result_FF;
    delete result_BP;
    delete epsilonNxt;
}


TEST_F(ConvolutionTests, Test_Conv1D_ff_2) {
    NDArray<double> input('c', {2, 2, 6});
    NDArray<double> weights('c', {3, 2, 2});

    NDArrayFactory<double>::linspace(1, input);
    NDArrayFactory<double>::linspace(1, weights);

    nd4j::ops::conv1d<double> op;
    auto result = op.execute({&input, &weights}, {}, {2, 1, 0, 1});

    ASSERT_EQ(ND4J_STATUS_OK, result->status());

    auto z = result->at(0);

    delete result;
}


#endif //LIBND4J_CONVOLUTIONTESTS_H
