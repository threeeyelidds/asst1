#include <stdio.h>
#include <algorithm>
#include <math.h>
#include "CMU418intrin.h"
#include "logger.h"
using namespace std;


void absSerial(float* values, float* output, int N) {
    for (int i=0; i<N; i++) {
	float x = values[i];
	if (x < 0) {
	    output[i] = -x;
	} else {
	    output[i] = x;
	}
    }
}

// implementation of absolute value using 15418 instrinsics
void absVector(float* values, float* output, int N) {
    __cmu418_vec_float x;
    __cmu418_vec_float result;
    __cmu418_vec_float zero = _cmu418_vset_float(0.f);
    __cmu418_mask maskAll, maskIsNegative, maskIsNotNegative;

    //  Note: Take a careful look at this loop indexing.  This example
    //  code is not guaranteed to work when (N % VECTOR_WIDTH) != 0.
    //  Why is that the case?
    for (int i=0; i<N; i+=VECTOR_WIDTH) {

	// All ones
	maskAll = _cmu418_init_ones();

	// All zeros
	maskIsNegative = _cmu418_init_ones(0);

	// Load vector of values from contiguous memory addresses
	_cmu418_vload_float(x, values+i, maskAll);               // x = values[i];

	// Set mask according to predicate
	_cmu418_vlt_float(maskIsNegative, x, zero, maskAll);     // if (x < 0) {

	// Execute instruction using mask ("if" clause)
	_cmu418_vsub_float(result, zero, x, maskIsNegative);      //   output[i] = -x;

	// Inverse maskIsNegative to generate "else" mask
	maskIsNotNegative = _cmu418_mask_not(maskIsNegative);     // } else {

	// Execute instruction ("else" clause)
	_cmu418_vload_float(result, values+i, maskIsNotNegative); //   output[i] = x; }

	// Write results back to memory
	_cmu418_vstore_float(output+i, result, maskAll);
    }
}

// Accepts an array of values and an array of exponents
// For each element, compute values[i]^exponents[i] and clamp value to
// 4.18.  Store result in outputs.
// Uses iterative squaring, so that total iterations is proportional
// to the log_2 of the exponent
void clampedExpSerial(float* values, int* exponents, float* output, int N) {
    for (int i=0; i<N; i++) {
	float x = values[i];
	float result = 1.f;
	int y = exponents[i];
	float xpower = x;
	while (y > 0) {
	    if (y & 0x1) {
			result *= xpower;
		}
	    xpower = xpower * xpower;
	    y >>= 1;
	}
	if (result > 4.18f) {
	    result = 4.18f;
	}
	output[i] = result;
    }
}

void clampedExpVector(float* values, int* exponents, float* output, int N) {
    __cmu418_vec_float x, xpower, result, clampVal;
    __cmu418_vec_int y, oneInt, zeroInt, maskOneInt;
    __cmu418_mask maskAll, maskBits, maskOne, maskClamp;

    clampVal = _cmu418_vset_float(4.18f);
    oneInt = _cmu418_vset_int(1);
    zeroInt = _cmu418_vset_int(0);

    for (int i = 0; i < N; i += VECTOR_WIDTH) {
        maskAll = _cmu418_init_ones();

        _cmu418_vload_float(x, values + i, maskAll);
        _cmu418_vload_int(y, exponents + i, maskAll);
        
        result = _cmu418_vset_float(1.0f);
        xpower = x;
        // get the mask for y>0
        maskBits = _cmu418_init_ones(0);
        _cmu418_vgt_int(maskBits, y, zeroInt, maskAll);
        while (_cmu418_cntbits(maskBits) > 0) {
            // get the mask for y==1
            maskOne = _cmu418_init_ones(0);
            _cmu418_vbitand_int(maskOneInt, y, oneInt, maskBits);
            _cmu418_vgt_int(maskOne, maskOneInt, zeroInt, maskBits);
            // if y==1, compute result *= xpower;
            _cmu418_vmult_float(result, result, xpower, maskOne);
            // xpower = xpower * xpower;
            _cmu418_vmult_float(xpower, xpower, xpower, maskBits);
            // y >>= 1;
            _cmu418_vshiftright_int(y, y, oneInt, maskBits);
            _cmu418_vgt_int(maskBits, y, zeroInt, maskAll);
        }

        maskBits = _cmu418_init_ones(0);
        _cmu418_vgt_int(maskBits, y, zeroInt, maskAll);
        maskClamp = _cmu418_init_ones();
        _cmu418_vgt_float(maskClamp, result, clampVal, maskBits);
        _cmu418_vmove_float(result, clampVal, maskClamp);
        _cmu418_vstore_float(output + i, result, maskBits);
    }
}




float arraySumSerial(float* values, int N) {
    float sum = 0;
    for (int i=0; i<N; i++) {
	sum += values[i];
    }

    return sum;
}

// Assume N % VECTOR_WIDTH == 0
// Assume VECTOR_WIDTH is a power of 2
float arraySumVector(float* values, int N) {
    __cmu418_vec_float x, sumVec;
    __cmu418_mask maskAll;
    float sumArray[VECTOR_WIDTH];

    // Initialize the vector sum to zero
    sumVec = _cmu418_vset_float(0.f);

    // Process in chunks of VECTOR_WIDTH
    for (int i = 0; i < N; i += VECTOR_WIDTH) {
        maskAll = _cmu418_init_ones();

        // Load VECTOR_WIDTH elements into the vector register
        _cmu418_vload_float(x, values + i, maskAll);

        // Add the vector elements to the running sum
        _cmu418_vadd_float(sumVec, sumVec, x, maskAll);
    }

    // Store the vector sums into an array
    _cmu418_vstore_float(sumArray, sumVec, maskAll);

    // Sum up the elements of the sum array
    float totalSum = 0.f;
    for (int i = 0; i < VECTOR_WIDTH; i++) {
        totalSum += sumArray[i];
    }

    return totalSum;
}
