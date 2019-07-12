//============================================================================
// Name        : bchchallenge.cpp
// Author      : 
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================


#include <cstdio>
#include <iostream>

#include "unity.h"


using namespace std;


void setUp(void) {
    // set stuff up here
}

void tearDown(void) {
    // clean stuff up here
}



/**
 * Calculate the BCH and even parity bits for a given codeword.
 *
 * @param cw	Codeword. Only the data bits need be filled in.
 * @return		Codeword with BCH parity and even parity applied.
 */
uint32_t calcBCHAndParity(const uint32_t cw)
{
	uint32_t bit = 0;
	uint32_t parity = 0;
	uint32_t local_cw = cw & 0xFFFFF800;	// mask off BCH parity and even parity
	uint32_t cw_e = local_cw;

	// Calculate BCH bits
	for (bit = 1; bit <= 21; bit++) {
		if (cw_e & 0x80000000) {
			cw_e ^= 0xED200000;
		}
		cw_e <<= 1;
	}
	local_cw |= (cw_e >> 21);

	// At this point local_cw contains a codeword with BCH but no parity

	// Calculate parity bit
	cw_e = local_cw;
	bit = 32;
	while ((cw_e != 0) && (bit > 0)) {
		if (cw_e & 1) {
			parity++;
		}
		cw_e >>= 1;
		bit--;
	}

	// apply parity bit
	return (parity%2) ? (local_cw | 1) : local_cw;
}


//#define BCH_REPAIR_DEBUG

int RepairBCH(const uint32_t cw, uint32_t *repaired_cw)
{
	// calculate syndrome
	// We do this by recalculating the BCH parity bits and XORing them against the received ones
	uint32_t syndrome = ((calcBCHAndParity(cw) ^ cw) >> 1) & 0x3FF;		// mask off data bits and parity, leaving the error syndrome in the LSB

	if (syndrome == 0) {
		// Syndrome of zero indicates no repair required
		*repaired_cw = cw;
		return 0;
	}

#ifdef BCH_REPAIR_DEBUG
	printf("cw:%08X  syndrome:%08X\n", cw, syndrome);
#endif

	// --- Meggitt decoder ---

	uint32_t result = 0;
	uint32_t damaged_cw = cw;

	// Calculate BCH bits
	for (uint32_t xbit = 0; xbit < 31; xbit++) {

#ifdef BCH_REPAIR_DEBUG_STEPBYSTEP
		printf("    xbit:%2d  synd:%08X  dcw:%08X  fixed:%08X", xbit, syndrome, damaged_cw, result);
#endif

		// produce the next corrected bit in the high bit of the result
		result <<= 1;
		if ((syndrome == 0x3B4) ||		// 0x3B4: Syndrome when a single error is detected in the MSB
			(syndrome == 0x26E)	||		// 0x26E: Two adjacent errors
			(syndrome == 0x359) ||		// 0x359: Two errors, one OK bit between
			(syndrome == 0x076) ||		// 0x076: Two errors, two OK bits between
			(syndrome == 0x255) ||		// 0x255: Two errors, three OK bits between
			(syndrome == 0x0F0) ||		// 0x0F0: Two errors, four OK bits between
			(syndrome == 0x216) ||
			(syndrome == 0x365) ||
			(syndrome == 0x068) ||
			(syndrome == 0x25A) ||
			(syndrome == 0x343) ||
			(syndrome == 0x07B) ||
			(syndrome == 0x1E7) ||
			(syndrome == 0x129) ||
			(syndrome == 0x14E) ||
			(syndrome == 0x2C9) ||
			(syndrome == 0x0BE) ||
			(syndrome == 0x231) ||
			(syndrome == 0x0C2) ||
			(syndrome == 0x20F) ||
			(syndrome == 0x0DD) ||
			(syndrome == 0x1B4) ||
			(syndrome == 0x2B4) ||
			(syndrome == 0x334) ||
			(syndrome == 0x3F4) ||
			(syndrome == 0x394) ||
			(syndrome == 0x3A4) ||
			(syndrome == 0x3BC) ||
			(syndrome == 0x3B0) ||
			(syndrome == 0x3B6) ||
			(syndrome == 0x3B5)
		   ) {
										// FIXME: Doesn't detect double bit errors
			// Syndrome matches an error in the MSB
			// Correct that error and adjust the syndrome to account for it
			syndrome ^= 0x3B4;

			result |= (~damaged_cw & 0x80000000) >> 30;

#ifdef BCH_REPAIR_DEBUG_STEPBYSTEP
			printf("  E\n");	// indicate that an error was corrected in this bit
#endif
		} else {
			// no error
			result |= (damaged_cw & 0x80000000) >> 30;

#ifdef BCH_REPAIR_DEBUG_STEPBYSTEP
			printf("   \n");
#endif
		}
		damaged_cw <<= 1;


		// Handle Syndrome shift register feedback
		if (syndrome & 0x200) {
			syndrome <<= 1;
			syndrome ^= 0x769;		// 0x769 = POCSAG generator polynomial -- x^10 + x^9 + x^8 + x^6 + x^5 + x^3 + 1
		} else {
			syndrome <<= 1;
		}
		// mask off bits which fall off the end of the syndrome shift register
		syndrome &= 0x3FF;

		// FIXME: Can we exit early if the syndrome is zero? (no more errors to correct)
	}

#ifdef BCH_REPAIR_DEBUG
	printf("  orig:%08X  fixed:%08X  %s\n",
			cw,					/* original codeword */
			result,				/* corrected codeword sans parity bit */
			syndrome == 0 ? "OK" : "ERR"		/* syndrome == 0 if error was corrected */
			);
#endif

	// Check if error correction was successful
	if (syndrome != 0) {
		// Syndrome nonzero at end indicates uncorrectable errors
		printf("nonzero syndrome at end\n");
		return -1;
	}

	// Syndrome is zero -- that means we must have succeeded!
	*repaired_cw = result;
	return 0;
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
		TEST_ASSERT_EQUAL_UINT(calcBCHAndParity(TEST_CWS[n]), TEST_CWS[n]);
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

			int result = RepairBCH(damaged_cw, &repaired_cw);
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
		TEST_ASSERT_EQUAL_UINT(calcBCHAndParity(TEST_CWS[n]), TEST_CWS[n]);
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

				int result = RepairBCH(damaged_cw, &repaired_cw);
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


int main()
{

	UNITY_BEGIN();

	// TODO: Test bitflips, erase to 0 and erase to 1

	RUN_TEST(test_BCH_single_bit_errors);
	RUN_TEST(test_BCH_double_bit_errors);

	return UNITY_END();
}
