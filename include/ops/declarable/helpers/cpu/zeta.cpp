//
// Created by Yurii Shyrma on 12.12.2017
//


#include <DataTypeUtils.h>
#include<ops/declarable/helpers/zeta.h>

namespace nd4j {
namespace ops {
namespace helpers {

const int maxIter = 1000000;							// max number of loop iterations 
const double machep =  1.11022302462515654042e-16;		

// expansion coefficients for Euler-Maclaurin summation formula (2k)! / B2k, where B2k are Bernoulli numbers
const double coeff[] = { 12.0,-720.0,30240.0,-1209600.0,47900160.0,-1.8924375803183791606e9,7.47242496e10,-2.950130727918164224e12, 1.1646782814350067249e14, -4.5979787224074726105e15, 1.8152105401943546773e17, -7.1661652561756670113e18};


//////////////////////////////////////////////////////////////////////////
// slow implementation
template <typename T>
T zetaSlow(const T x, const T q) {
	
	const T precision = (T)1e-7; 									// function stops the calculation of series when next item is <= precision
		
	// if (x <= (T)1.) 
	// 	throw("zeta function: x must be > 1 !");

	// if (q <= (T)0.) 
	// 	throw("zeta function: q must be > 0 !");

	T item;
	T result = (T)0.;
// #pragma omp declare reduction (add : double,float,float16 : omp_out += omp_in) initializer(omp_priv = (T)0.)
// #pragma omp simd private(item) reduction(add:result)
	for(int i = 0; i < maxIter; ++i) {		
		
		item = math::nd4j_pow((q + i),-x);
		result += item;
		
		if(item <= precision)
			break;
	}

	return result;
}


//////////////////////////////////////////////////////////////////////////
// fast implementation, it is based on Euler-Maclaurin summation formula
template <typename T>
T zeta(const T x, const T q) {
	
	// if (x <= (T)1.) 
	// 	throw("zeta function: x must be > 1 !");

	// if (q <= (T)0.) 
	// 	throw("zeta function: q must be > 0 !");

	T a, b(0.), k, s, t, w;
		
	s = math::nd4j_pow(q, -x);
	a = q;
	int i = 0;

	while(i < 9 || a <= (T)9.) {
		i += 1;
		a += (T)1.0;
		b = math::nd4j_pow(a, -x);
		s += b;
		if(math::nd4j_abs(b / s) < (T)machep)
			return s;
	}
	
	w = a;
	s += b * (w / (x - (T)1.) - (T)0.5);
	a = (T)1.;
	k = (T)0.;
	
	for(i = 0; i < 12; ++i) {
		a *= x + k;
		b /= w;
		t = a * b / coeff[i];
		s += t;
		t = math::nd4j_abs(t / s);
		
		if(t < (T)machep)
			return s;
		
		k += (T)1.;
		a *= x + k;
		b /= w;
		k += (T)1.;
	}
	
	return s;
}


//////////////////////////////////////////////////////////////////////////
// calculate the Hurwitz zeta function for arrays
template <typename T>
NDArray<T> zeta(const NDArray<T>& x, const NDArray<T>& q) {

	NDArray<T> result(&x, false, x.getWorkspace());

#pragma omp parallel for if(x.lengthOf() > Environment::getInstance()->elementwiseThreshold()) schedule(guided)	
	for(int i = 0; i < x.lengthOf(); ++i)
		result(i) = zeta<T>(x(i), q(i));

	return result;
}


template float   zeta<float>  (const float   x, const float   q);
template float16 zeta<float16>(const float16 x, const float16 q);
template double  zeta<double> (const double  x, const double  q);

template NDArray<float>   zeta<float>  (const NDArray<float>&   x, const NDArray<float>&   q);
template NDArray<float16> zeta<float16>(const NDArray<float16>& x, const NDArray<float16>& q);
template NDArray<double>  zeta<double> (const NDArray<double>&  x, const NDArray<double>&  q);


}
}
}

