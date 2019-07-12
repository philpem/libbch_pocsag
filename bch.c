#include <stdio.h>
#include <stdint.h>

/// POCSAG synchronisation codeword.
const uint32_t POCSAG_SYNC = 0x7CD215D8UL;

/// POCSAG idle codeword.
const uint32_t POCSAG_IDLE = 0x7A89C197UL;



/**
 * POCSAG codeword:
 *
 * Address
 * -=-=-=-
 * msb  18 bits address   func  BCH check  even parity
 * [0][aaaaaaaaaaaaaaaaaa][ff][bbbbbbbbbb][p]
 *  1       18 bits         2    10 bits   1
 */



/**
 * Calculate BCH and Parity bits for a POCSAG codeword.
 *
 * From http://stackoverflow.com/questions/27577404/bch-crc-help-in-c.
 * The same algorithm is also used in the Perl POCSAG::Encode library
 * (http://search.cpan.org/~hessu/POCSAG-Encode-1.00/Encode.pm) so it's
 * probably fair to say this is accurate ;)
 *
 * @param	cw	Codeword to calculate BCH and parity bits for.
 * @returns		Codeword with BCH and parity bits present.
 */
static uint32_t pocsag_bch_and_parity(const uint32_t cw)
{
	uint32_t bit=0;
	uint32_t parity = 0;
	uint32_t local_cw = cw;
	uint32_t cw_e = cw;

	// Calculate BCH bit
	for(bit=1; bit<=21; bit++, cw_e <<= 1)
		if (cw_e & 0x80000000)
			cw_e ^= 0xED200000;
	local_cw = cw | (cw_e >> 21);

	// At this point, local_cw contains a codeword with BCH but no parity.

	// Calculate parity bit
#if 0
	cw_e = local_cw;
	for(bit=1; bit<=32; bit++, cw_e <<= 1)
		if (cw_e & 0x80000000)
			parity++;
#else
	cw_e = local_cw;
	bit = 32;
	// Only loop while cw is nonzero and we have more bits to process
	// This counts the number of bits set in cw_e
	while ((cw_e != 0) && (bit > 0))
	{
		if (cw_e & 1)
		{
			parity++;
		}
		cw_e >>= 1;
		bit--;
	}
#endif

	// Set last bit high or low depending on parity
	return (parity%2) ? (local_cw | 1) : local_cw;
}



static uint32_t meggitt_correct(const uint32_t cw)
{
	uint32_t cw_e = cw & 0xFFFFF800;	// mask off the BCH and parity

	// Calculate BCH bits for error polynomial
	for(size_t bit=1; bit<=21; bit++) {
		if (cw_e & 0x80000000)
			cw_e ^= 0xED200000;
		cw_e <<= 1;
	}
	// cw_e now contains the BCH code
	uint32_t cw_bch = cw | (cw_e >> 21);


	// Calculate the error syndrome
	uint32_t syndrome = (cw ^ cw_bch) >> 1;		// shr 1 to knock off the parity bit
	printf("syndrome: %08X\n", syndrome);


	return cw_bch;

	// At this point, local_cw contains a codeword with BCH but no parity.

	// Calculate parity bit
#if 0
	uint32_t parity = 0;
	cw_e = local_cw;
	bit = 32;
	// Only loop while cw is nonzero and we have more bits to process
	// This counts the number of bits set in cw_e
	while ((cw_e != 0) && (bit > 0))
	{
		if (cw_e & 1)
		{
			parity++;
		}
		cw_e >>= 1;
		bit--;
	}

	// Set last bit high or low depending on parity
	return (parity%2) ? (local_cw | 1) : local_cw;
#endif
}



int main(void)
{
	uint32_t cw = POCSAG_IDLE & (0xFFFFFFFF << 1);	// mask off parity
	uint32_t errormask = 0x80000000;

	printf("Orig:    %08X\n", POCSAG_IDLE);
	printf("Emask:   %08X\n", errormask);

	cw ^= errormask;

	printf("Errored: %08X\n", cw);
	printf("Corr'd:  %08X\n", meggitt_correct(cw));

	return 0;
}
