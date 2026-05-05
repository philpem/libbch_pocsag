#include <stdint.h>
#include "bch.h"

// BCH(31,21) parameters
//   g(x) = x^10 + x^9 + x^8 + x^6 + x^5 + x^3 + 1
#define BCH_N            31U                // codeword length (bits, sans parity)
#define BCH_K            21U                // data bits
#define BCH_R            (BCH_N - BCH_K)    // parity bits = deg(g) = 10
#define BCH_GEN_POLY     0x769U             // g(x), bit 10 = leading 1
#define BCH_GEN_HIGH     (1U << BCH_R)      // bit 10 (degree-10 term)
#define BCH_SYND_MASK    (BCH_GEN_HIGH - 1U) // 0x3FF, syndrome register width

// Same generator, shifted so its leading 1 lands in bit 31. Used by the
// systematic encoder to do long-division at the top of a uint32_t.
#define BCH_GEN_POLY_MSB (BCH_GEN_POLY << (32U - BCH_R - 1U))   // 0xED200000

uint32_t bch_encode(const uint32_t cw)
{
	uint32_t bit = 0;
	uint32_t parity = 0;
	uint32_t local_cw = cw & 0xFFFFF800;	// mask off BCH parity and even parity
	uint32_t cw_e = local_cw;

	// Calculate BCH bits
	for (bit = 1; bit <= BCH_K; bit++) {
		if (cw_e & 0x80000000) {
			cw_e ^= BCH_GEN_POLY_MSB;
		}
		cw_e <<= 1;
	}
	local_cw |= (cw_e >> BCH_K);

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

// Debug options for error correction
// -- Enable debug
//#define BCH_REPAIR_DEBUG
// -- Enable printing the output of the ECC process step-by-step
//#define BCH_REPAIR_DEBUG_STEPBYSTEP

// Bitmap of "MSB-error" syndromes for the Meggitt decoder. One bit per
// possible 10-bit syndrome value, so 1024 bits = 128 bytes.
//
// A syndrome is in this set iff the error pattern responsible for it
// includes an error in the MSB (codeword position 30), with at most one
// further error elsewhere (giving 1 + 30 = 31 entries total).
//
// All entries are derived from the generator polynomial: by linearity, the
// syndrome of error pattern e(x) = sum(x^i) is sum(x^i mod g(x)). Hence the
// table is { x^30 mod g } U { (x^30 ^ x^k) mod g : 0 <= k < 30 }.
static uint8_t meggitt_msb_error_bitmap[(1U << BCH_R) / 8U];
static int meggitt_table_initialised = 0;

// Syndrome of "single error in the MSB", i.e. x^30 mod g(x). Computed at
// init time and cached because the decoder needs it on every correction.
static uint16_t meggitt_msb_syndrome;

static void build_meggitt_table(void)
{
    // pow_x[k] = x^k mod g(x), built with a length-10 LFSR clocked by x.
    uint16_t pow_x[BCH_N];
    uint16_t v = 1U;
    for (uint32_t k = 0; k < BCH_N; k++) {
        pow_x[k] = v;
        v <<= 1;
        if (v & BCH_GEN_HIGH) {
            v ^= BCH_GEN_POLY;
        }
    }

    const uint16_t s_msb = pow_x[BCH_N - 1];   // x^30 mod g(x) == 0x3B4
    meggitt_msb_syndrome = s_msb;

    // Single error at the MSB.
    meggitt_msb_error_bitmap[s_msb >> 3] |= (uint8_t)(1U << (s_msb & 7U));

    // Double error: MSB plus position k for k = 0..29.
    for (uint32_t k = 0; k < BCH_N - 1; k++) {
        const uint16_t s = (uint16_t)(s_msb ^ pow_x[k]);
        meggitt_msb_error_bitmap[s >> 3] |= (uint8_t)(1U << (s & 7U));
    }

    meggitt_table_initialised = 1;
}

static inline int is_msb_error_syndrome(uint16_t syndrome)
{
    return (meggitt_msb_error_bitmap[syndrome >> 3] >> (syndrome & 7U)) & 1U;
}

int bch_repair(const uint32_t cw, uint32_t *repaired_cw)
{
    if (!meggitt_table_initialised) {
        build_meggitt_table();
    }


	// calculate syndrome
	// We do this by recalculating the BCH parity bits and XORing them against the received ones

	// mask off data bits and parity, leaving the error syndrome in the LSB
	uint32_t syndrome = ((bch_encode(cw) ^ cw) >> 1) & BCH_SYND_MASK;

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
	for (uint32_t xbit = 0; xbit < BCH_N; xbit++) {

#ifdef BCH_REPAIR_DEBUG_STEPBYSTEP
		printf("    xbit:%2d  synd:%08X  dcw:%08X  fixed:%08X", xbit, syndrome, damaged_cw, result);
#endif

		// produce the next corrected bit in the high bit of the result
		result <<= 1;
		if (is_msb_error_syndrome((uint16_t)syndrome)) {
			// Syndrome matches an error in the MSB.
			// Correct that error and remove it from the running syndrome.
			syndrome ^= meggitt_msb_syndrome;

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
		syndrome <<= 1;
		if (syndrome & BCH_GEN_HIGH) {
			syndrome ^= BCH_GEN_POLY;
		}
		// mask off bits which fall off the end of the syndrome shift register
		syndrome &= BCH_SYND_MASK;

		// XXX Possible optimisation: Can we exit early if the syndrome is zero? (no more errors to correct)
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
#ifdef BCH_REPAIR_DEBUG
		printf("nonzero syndrome at end\n");
#endif
		*repaired_cw = cw;
		return -1;
	}

	// Syndrome is zero -- that means we must have succeeded!
	*repaired_cw = result;
	return 0;
}


