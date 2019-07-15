//============================================================================
// Name        : bchchallenge.cpp
// Author      : 
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================


#include <cstdio>
#include <iostream>

#include "bch.h"

#include "unity.h"


using namespace std;


void setUp(void) {
    // set stuff up here
}

void tearDown(void) {
    // clean stuff up here
}


void test_BCH_single_bit_errors(void)
{
	// message buffer for unity
	char message[100];

	// codewords to test with -- these are the POCSAG Idle Codeword and Sync Codeword
	const uint32_t TEST_CWS[] = {0x7A89C197UL, 0x7CD215D8UL};

	// Idle Codeword and Sync Codeword have valid BCH and parity and can be error-corrected
	// Start by making sure this assumption is true
	for (size_t n=0; n<sizeof(TEST_CWS)/sizeof(TEST_CWS[0]); n++) {
		sprintf(message, "testCW index %zu: 0x%08X", n, TEST_CWS[n]);
		TEST_ASSERT_EQUAL_UINT(bch_encode(TEST_CWS[n]), TEST_CWS[n]);
	}

	for (size_t n=0; n<sizeof(TEST_CWS)/sizeof(TEST_CWS[0]); n++) {
		uint32_t OriginalCW = TEST_CWS[n];

		// try all possible single bit errors
		uint32_t mask = 0x80000000;

		while (mask > 0)
		{
			// add damage to the CW
			uint32_t damaged_cw = OriginalCW ^ mask;
			uint32_t repaired_cw;

			sprintf(message, "origCW:%08X, errormask:%08X", OriginalCW, mask);

			int result = bch_repair(damaged_cw, &repaired_cw);
			TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, message);

			// BCH doesn't repair the parity bit. We're only concerned about repairing errors in the message or BCH parity bits.
			// (So mask off the parity in the LSB)
			TEST_ASSERT_EQUAL_UINT32_MESSAGE(OriginalCW & 0xFFFFFFFE, repaired_cw & 0xFFFFFFFE, message);

			// test the next bit
			mask >>= 1;
		}
	}
}


void test_BCH_double_bit_errors(void)
{
	// message buffer for unity
	char message[100];
	// subtest count
	size_t numTests = 0;

	// codewords to test with -- these are the POCSAG Idle Codeword and Sync Codeword
	const uint32_t TEST_CWS[] = {0x7A89C197UL, 0x7CD215D8UL};

	// Idle Codeword and Sync Codeword have valid BCH and parity and can be error-corrected
	// Start by making sure this assumption is true
	for (size_t n=0; n<sizeof(TEST_CWS)/sizeof(TEST_CWS[0]); n++) {
		sprintf(message, "testCW index %zu: 0x%08X", n, TEST_CWS[n]);
		TEST_ASSERT_EQUAL_UINT(bch_encode(TEST_CWS[n]), TEST_CWS[n]);
	}

	for (size_t n=0; n<sizeof(TEST_CWS)/sizeof(TEST_CWS[0]); n++) {
		uint32_t OriginalCW = TEST_CWS[n];

		// try all possible single bit errors
		uint32_t mask1 = 0x80000000;

		while (mask1 > 1)	// LSB is even parity, don't waste time on it
		{
			uint32_t mask2 = mask1 >> 1;
			while (mask2 > 1)	// LSB is even parity, don't waste time on it
			{
				// add damage to the CW
				uint32_t damaged_cw = OriginalCW ^ mask1 ^ mask2;
				uint32_t repaired_cw;

				sprintf(message, "origCW:%08X, errormask:%08X", OriginalCW, mask1 ^ mask2);

				int result = bch_repair(damaged_cw, &repaired_cw);
				TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, message);

				// BCH doesn't repair the parity bit. We're only concerned about repairing errors in the message or BCH parity bits.
				// (So mask1 off the parity in the LSB)
				TEST_ASSERT_EQUAL_UINT32_MESSAGE(OriginalCW & 0xFFFFFFFE, repaired_cw & 0xFFFFFFFE, message);

				// increment test counter
				numTests++;

				// test the next bit
				mask2 >>= 1;
			}

			mask1 >>= 1;
		}
	}

	printf("\t%s: %zu tests finished\n", __func__, numTests);
}

