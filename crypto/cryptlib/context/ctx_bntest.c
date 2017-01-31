/****************************************************************************
*																			*
*						  cryptlib Bignum Test Routines						*
*						Copyright Peter Gutmann 1995-2015					*
*																			*
****************************************************************************/

#define PKC_CONTEXT		/* Indicate that we're working with PKC contexts */
#if defined( INC_ALL )
  #include "crypt.h"
  #include "context.h"
#else
  #include "crypt.h"
  #include "context/context.h"
#endif /* Compiler-specific includes */

#ifdef USE_PKC

/* Test operation types */

typedef enum {
	BN_OP_NONE,				/* No operation type */
	BN_OP_CMP, BN_OP_CMPPART,
	BN_OP_ADD, BN_OP_SUB, 
	BN_OP_LSHIFT, BN_OP_RSHIFT,
	BN_OP_ADDWORD, BN_OP_SUBWORD,
	BN_OP_MULWORD, BN_OP_MODWORD,
	BN_OP_SQR, BN_OP_MONTMODMULT,
#if defined( USE_ECDH ) || defined( USE_ECDSA )
	BN_OP_MODADD, BN_OP_MODSUB,
	BN_OP_MODMUL, BN_OP_MODSHIFT,
#endif /* USE_ECDH || USE_ECDSA */
	BN_OP_LAST				/* Last possible operation type */
	} BN_OP_TYPE;

/* Some of the tests require negative values, in which case we encode a sign
   bit in the length field */

#define BN_VAL_NEGATIVE		0x800000
#define MK_NEGATIVE( val )	( ( val ) | 0x800000 )

/* The structure used to hold the self-test values */

typedef struct {
	const int aLen; const BYTE *a;
	const int bLen; const BYTE *b; unsigned int bWord;
	const int resultLen; const BYTE *result;
	const int modLen; const BYTE *mod;
	} SELFTEST_VALUE;

/* Macro to shorten the amount of text required to represent array sizes */

#define FS_SIZE( selfTestArray )	FAILSAFE_ARRAYSIZE( selfTestArray, SELFTEST_VALUE )

/****************************************************************************
*																			*
*								Self-test Data								*
*																			*
****************************************************************************/

#ifndef NDEBUG

/* Test values, a op b -> result.  Notes: We can't use values of 0 or 1 since
   these are never valid and are automatically rejected by importBignum(),
   and the code assumes a 32-bit architecture for checking of carry/borrow
   propagation functionality (it'll still work on 64-bit but won't hit as
   many corner cases) */

static const SELFTEST_VALUE cmpSelftestValues[] = {
	{ 1, "\x02", 1, "\x02", 0, 0, NULL },
	{ 1, "\x03", 1, "\x02", 0, 1, NULL },
	{ 1, "\x02", 1, "\x03", 0, -1, NULL },
	{ 5, "\x01\x00\x00\x00\x00", 4, "\xFF\xFF\xFF\xFF", 0, 1, NULL },
	{ 9, "\x01\x00\x00\x00\x00\x00\x00\x00\x00", 4, "\xFF\xFF\xFF\xFF", 0, 1, NULL },
	{ 4, "\xFF\xFF\xFF\xFF", 5, "\x01\x00\x00\x00\x00", 0, -1, NULL },
	{ 4, "\xFF\xFF\xFF\xFF", 9, "\x01\x00\x00\x00\x00\x00\x00\x00\x00", 0, -1, NULL },
		{ 0, NULL, 0, NULL, 0, 0, NULL }, { 0, NULL, 0, NULL, 0, 0, NULL }
	};

static const SELFTEST_VALUE addsubSelftestValues[] = {
	/* a, b, result */
	{ 1, "\x02", 1, "\x03", 0,
	  1, "\x05" },
	{ MK_NEGATIVE( 1 ), "\x03", 1, "\x05", 0,
	  1, "\x02" },
	{ 4, "\xFF\xFF\xFF\xFF", 1, "\x02", 0,
	  5, "\x01\x00\x00\x00\x01" },
	{ 4, "\xFF\xFF\xFF\xFF", 4, "\xFF\xFF\xFF\xFF", 0,
	  5, "\x01\xFF\xFF\xFF\xFE" },
	{ 8, "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF", 1, "\x02", 0,
	  9, "\x01\x00\x00\x00\x00\x00\x00\x00\x01" },
	{ 8, "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF", 8, "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF", 0,
	  9, "\x01\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFE" },
	{ 12, "\x55\x55\x55\x55\x55\x55\x55\x55\x55\x55\x55\x55", 1, "\x02", 0,
	  12, "\x55\x55\x55\x55\x55\x55\x55\x55\x55\x55\x55\x57" },
	{ 12, "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x00\x00\x00\x01", 4, "\xFF\xFF\xFF\xFF", 0,
	  13, "\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" },
	{ 12, "\xFF\xFF\xFF\xFF\x00\x00\x00\x00\x00\x00\x00\x01", 8, "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF", 0,
	  13, "\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" },
	{ 8, "\xFF\xFF\xFF\xFF\x00\x00\x00\x01", 12, "\xFF\xFF\xFF\xFF\x00\x00\x00\x00\xFF\xFF\xFF\xFF", 0,
	  13, "\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" },
	{ 16, "\x55\x55\x55\x55\x55\x55\x55\x55\x55\x55\x55\x55\x55\x55\x55\x55", 8, "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF", 0,
	  16, "\x55\x55\x55\x55\x55\x55\x55\x56\x55\x55\x55\x55\x55\x55\x55\x54" },
	{ 16, "\x55\x55\x55\x55\x55\x55\x55\x55\x00\x00\x00\x00\x55\x55\x55\x55", 8, "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF", 0,
	  16, "\x55\x55\x55\x55\x55\x55\x55\x56\x00\x00\x00\x00\x55\x55\x55\x54" },
	{ 16, "\x55\x55\x55\x55\x55\x55\x55\x55\x00\x00\x00\x00\x55\x55\x55\x55", 12, "\xFF\xFF\xFF\xFF\x00\x00\x00\x00\xFF\xFF\xFF\xFF", 0,
	  16, "\x55\x55\x55\x56\x55\x55\x55\x54\x00\x00\x00\x01\x55\x55\x55\x54" },
		{ 0, NULL, 0, NULL, 0, 0, NULL }, { 0, NULL, 0, NULL, 0, 0, NULL }
	};

static const SELFTEST_VALUE shiftSelftestValues[] = {
	/* a, shiftAmt, result */
	{ 1, "\x02", 1, NULL, 0,
	  1, "\x04" },
	{ 4, "\x80\x00\x00\x01", 1, NULL, 0,
	  5, "\x01\x00\x00\x00\x02" },
	{ 4, "\xFF\xFF\xFF\xFF", 1, NULL, 0,
	  5, "\x01\xFF\xFF\xFF\xFE" },
	{ 8, "\x55\x55\x55\x55\x55\x55\x55\x55", 1, NULL, 0,
	  8, "\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA" },
	{ 10, "\x01\x23\x45\x67\x89\xAB\xCD\xEF\xE2\xD3", 1, NULL, 0,
	  10, "\x02\x46\x8A\xCF\x13\x57\x9B\xDF\xC5\xA6" },
	{ 12, "\xFF\xFF\xFF\xFF\x00\x00\x00\x00\xFF\xFF\xFF\xFF", 16, NULL, 0,
	  14, "\xFF\xFF\xFF\xFF\x00\x00\x00\x00\xFF\xFF\xFF\xFF\x00\x00" },
	{ 12, "\xFF\xFF\xFF\xFF\x00\x00\x00\x00\xFF\xFF\xFF\xFF", 32, NULL, 0,
	  16, "\xFF\xFF\xFF\xFF\x00\x00\x00\x00\xFF\xFF\xFF\xFF\x00\x00\x00\x00" },
	{ 8, "\x01\x23\x45\x67\x89\xAB\xCD\xEF", 93, NULL, 0,
	  19, "\x24\x68\xAC\xF1\x35\x79\xBD\xE0\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" },
		{ 0, NULL, 0, NULL, 0, 0, NULL }, { 0, NULL, 0, NULL, 0, 0, NULL }
	};

static const SELFTEST_VALUE addsubWordsSelftestValues[] = {
	/* a, bWord, result */
	{ 1, "\x02", 0, NULL, 3,
	  1, "\x05" },
	{ 4, "\xFF\xFF\xFF\xFF", 0, NULL, 2,
	  5, "\x01\x00\x00\x00\x01" },
	{ 4, "\xFF\xFF\xFF\xFD", 0, NULL, 2,
	  4, "\xFF\xFF\xFF\xFF" },
	{ 8, "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF", 0, NULL, 2,
	  9, "\x01\x00\x00\x00\x00\x00\x00\x00\x01" },
		{ 0, NULL, 0, NULL, 0, 0, NULL }, { 0, NULL, 0, NULL, 0, 0, NULL }
	};

static const SELFTEST_VALUE muldivWordsSelftestValues[] = {
	/* a, bWord, result */
	{ 1, "\x02", 0, NULL, 3,
	  1, "\x06" },
	{ 2, "\x45\x67", 0, NULL, 0x89AB,
	  4, "\x25\x52\x7A\xCD" },
	{ 4, "\x12\x34\x56\x78", 0, NULL, 0x9ABCDEF1,
	  8, "\x0B\x00\xEA\x4E\x36\x61\x76\xF8" },
	{ 8, "\x12\x34\x56\x78\x9A\xBC\xDE\xF1", 0, NULL, 0xAA55AA55,
	  12, "\x0C\x1C\xD8\xE9\x99\x99\x99\x8E\xDC\xC7\x10\x05" },
		{ 0, NULL, 0, NULL, 0, 0, NULL }, { 0, NULL, 0, NULL, 0, 0, NULL }
	};

static const SELFTEST_VALUE modWordsSelftestValues[] = {
	/* a, bWord, result */
	{ 1, "\x06", 0, NULL, 3, 0, NULL },
	{ 1, "\x05", 0, NULL, 3, 2, NULL },
	{ 4, "\xFF\xFF\xFF\xFF", 0, NULL, 7, 3, NULL },
	{ 8, "\x55\x55\x55\x55\x55\x55\x55\x55", 0, NULL, 14, 5, NULL },
	{ 8, "\x55\x55\x55\x55\x55\x55\x55\x55", 0, NULL, 15, 5, NULL },
	{ 8, "\x55\x55\x55\x55\x55\x55\x55\x55", 0, NULL, 16, 5, NULL },
	{ 8, "\x55\x55\x55\x55\x55\x55\x55\x55", 0, NULL, 17, 0, NULL },
	{ 8, "\x55\x55\x55\x55\x55\x55\x55\x55", 0, NULL, 18, 17, NULL },
	{ 8, "\x55\x55\x55\x55\x55\x55\x55\x55", 0, NULL, 19, 18, NULL },
	{ 8, "\x55\x55\x55\x55\x55\x55\x55\x55", 0, NULL, 20, 5, NULL },
	{ 12, "\xFF\xFF\xFF\xFF\x00\x00\x00\x00\xFF\xFF\xFF\xFF", 0, NULL, 29, 27, NULL },
	{ 12, "\xFF\xFF\xFF\xFF\x00\x00\x00\x00\xFF\xFF\xFF\xFF", 0, NULL, 177545, 111310, NULL },
	{ 12, "\xFF\xFF\xFF\xFF\x00\x00\x00\x00\xFF\xFF\xFF\xFF", 0, NULL, 0xFFFE, 51, NULL },
	{ 12, "\xFF\xFF\xFF\xFF\x00\x00\x00\x00\xFF\xFF\xFF\xFF", 0, NULL, 0xFFFF, 0, NULL },
	{ 12, "\xFF\xFF\xFF\xFF\x00\x00\x00\x00\xFF\xFF\xFF\xFF", 0, NULL, 0xFFFFFFFE, 5, NULL },
	{ 12, "\xFF\xFF\xFF\xFF\x00\x00\x00\x00\xFF\xFF\xFF\xFF", 0, NULL, 0xFFFFFFFF, 0, NULL },
		{ 0, NULL, 0, NULL, 0, 0, NULL }, { 0, NULL, 0, NULL, 0, 0, NULL }
	};

static const SELFTEST_VALUE sqrSelftestValues[] = {
	/* a, result */
	{ 1, "\x02", 0, NULL, 0,
	  1, "\x04" },
	{ 4, "\xFF\xFF\xFF\xFF", 0, NULL, 0, 
	  8, "\xFF\xFF\xFF\xFE\x00\x00\x00\x01" },
	{ 8, "\x55\x55\x55\x55\x55\x55\x55\x55", 0, NULL, 0, 
	  16, "\x1C\x71\xC7\x1C\x71\xC7\x1C\x71\x8E\x38\xE3\x8E\x38\xE3\x8E\x39" },
	{ 12, "\x55\x55\x55\x55\x55\x55\x55\x55\x55\x55\x55\x55", 0, NULL, 0, 
	  24, "\x1C\x71\xC7\x1C\x71\xC7\x1C\x71\xC7\x1C\x71\xC6\xE3\x8E\x38\xE3"
		  "\x8E\x38\xE3\x8E\x38\xE3\x8E\x39" },
	{ 20, "\x55\x55\x55\x55\x55\x55\x55\x55\x55\x55\x55\x55\x55\x55\x55\x55"
		  "\x55\x55\x55\x55", 0, NULL, 0,
	  40, "\x1C\x71\xC7\x1C\x71\xC7\x1C\x71\xC7\x1C\x71\xC7\x1C\x71\xC7\x1C"
		  "\x71\xC7\x1C\x71\x8E\x38\xE3\x8E\x38\xE3\x8E\x38\xE3\x8E\x38\xE3"
		  "\x8E\x38\xE3\x8E\x38\xE3\x8E\x39" },
	{ 24, "\x55\x55\x55\x55\x55\x55\x55\x55\x55\x55\x55\x55\x55\x55\x55\x55"
		  "\x55\x55\x55\x55\x55\x55\x55\x55", 0, NULL, 0,
	  48, "\x1C\x71\xC7\x1C\x71\xC7\x1C\x71\xC7\x1C\x71\xC7\x1C\x71\xC7\x1C"
		  "\x71\xC7\x1C\x71\xC7\x1C\x71\xC6\xE3\x8E\x38\xE3\x8E\x38\xE3\x8E"
		  "\x38\xE3\x8E\x38\xE3\x8E\x38\xE3\x8E\x38\xE3\x8E\x38\xE3\x8E\x39" },
	{ 28, "\x55\x55\x55\x55\x55\x55\x55\x55\x55\x55\x55\x55\x55\x55\x55\x55"
		  "\x55\x55\x55\x55\x55\x55\x55\x55\x55\x55\x55\x55", 0, NULL, 0,
	  56, "\x1C\x71\xC7\x1C\x71\xC7\x1C\x71\xC7\x1C\x71\xC7\x1C\x71\xC7\x1C"
		  "\x71\xC7\x1C\x71\xC7\x1C\x71\xC7\x1C\x71\xC7\x1C\x38\xE3\x8E\x38"
		  "\xE3\x8E\x38\xE3\x8E\x38\xE3\x8E\x38\xE3\x8E\x38\xE3\x8E\x38\xE3"
		  "\x8E\x38\xE3\x8E\x38\xE3\x8E\x39" },
	{ 16, "\x55\x55\x55\x55\x55\x55\x55\x55\x55\x55\x55\x55\x55\x55\x55\x55", 0, NULL, 0,
	  32, "\x1C\x71\xC7\x1C\x71\xC7\x1C\x71\xC7\x1C\x71\xC7\x1C\x71\xC7\x1C"
		  "\x38\xE3\x8E\x38\xE3\x8E\x38\xE3\x8E\x38\xE3\x8E\x38\xE3\x8E\x39" },
	{ 32, "\x55\x55\x55\x55\x55\x55\x55\x55\x55\x55\x55\x55\x55\x55\x55\x55"
		  "\x55\x55\x55\x55\x55\x55\x55\x55\x55\x55\x55\x55\x55\x55\x55\x55", 0, NULL, 0,
	  64, "\x1C\x71\xC7\x1C\x71\xC7\x1C\x71\xC7\x1C\x71\xC7\x1C\x71\xC7\x1C"
		  "\x71\xC7\x1C\x71\xC7\x1C\x71\xC7\x1C\x71\xC7\x1C\x71\xC7\x1C\x71"
		  "\x8E\x38\xE3\x8E\x38\xE3\x8E\x38\xE3\x8E\x38\xE3\x8E\x38\xE3\x8E"
		  "\x38\xE3\x8E\x38\xE3\x8E\x38\xE3\x8E\x38\xE3\x8E\x38\xE3\x8E\x39" },
		{ 0, NULL, 0, NULL, 0, 0, NULL }, { 0, NULL, 0, NULL, 0, 0, NULL }
	};

static const SELFTEST_VALUE montModMulSelftestValues[] = {
	/* a, b, result, modulus */
	{ 1, "\x03", 1, "\x04", 0,
	  1, "\x02", 1, "\x05" },
	{ 1, "\x07", 1, "\x07", 0,
	  1, "\x04", 1, "\x09" },
	{ 4, "\x12\x34\x56\x78", 4, "\x55\x55\x55\x55", 0,
	  4, "\x6E\x6E\x6E\x6E", 4, "\x9A\xBC\xDE\xF1" },
	{ 8, "\x12\x34\x56\x78\x9A\xBC\xDE\xF1", 8, "\x0C\x1C\xD8\xE9\x99\x99\x99\x8F", 0,
	  8, "\x67\x63\xD1\x34\xCA\x6D\x53\x9F", 8, "\xAA\x55\xAA\x55\xAA\x55\xAA\x55" },
	{ 8, "\x12\x34\x56\x78\x9A\xBC\xDE\xF1", 4, "\x56\x78\x9A\xBC", 0,
	  8, "\x02\x34\x6B\x2A\x61\x50\x18\x42", 8, "\x12\x34\x56\x79\x00\x00\x00\x01" },
	{ 12, "\xFF\xFF\xFF\xFF\x00\x00\x00\x00\xFF\xFF\xFF\xFF", 8, "\xAA\x55\xAA\x55\xAA\x55\xAA\x55", 0, 
	  13, "\x04\x12\x96\xEF\xDF\x20\x8F\x5F\xFC\x0B\xFE\x08\x53", 13, "\x12\x34\x56\x78\x9A\xBC\xDE\xF1\x23\x45\x67\x89\xAB" },

		{ 0, NULL, 0, NULL, 0, 0, NULL }, { 0, NULL, 0, NULL, 0, 0, NULL }
	};

#if defined( USE_ECDH ) || defined( USE_ECDSA )

static const SELFTEST_VALUE modAddSelftestValues[] = {
	/* a, b, result, modulus */
	{ 1, "\x03", 1, "\x04", 0,
	  1, "\x02", 1, "\x05" },
	{ 4, "\xFF\xFF\xFF\xFE", 1, "\x03", 0,
	  1, "\x02", 4, "\xFF\xFF\xFF\xFF" },
	{ 8, "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFE", 8, "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFE", 0,
	  8, "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFD", 8, "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF" },
	{ 12, "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x00\x00\x00\x03", 4, "\xFF\xFF\xFF\xFF", 0,
	  1, "\x02", 13, "\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" },
	{ 12, "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x00\x00\x00\x04", 4, "\xFF\xFF\xFF\xFF", 0,
	  1, "\x03", 13, "\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" },
	{ 12, "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x00\x00\x00\x05", 4, "\xFF\xFF\xFF\xFF", 0,
	  1, "\x04", 13, "\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" },
		{ 0, NULL, 0, NULL, 0, 0, NULL }, { 0, NULL, 0, NULL, 0, 0, NULL }
	};

static const SELFTEST_VALUE modSubSelftestValues[] = {
	/* a, b, result, modulus */
	{ 1, "\x03", 1, "\x04", 0,
	  1, "\x04", 1, "\x05" },
	{ 4, "\xFF\xFF\xFF\xFE", 1, "\x03", 0,
	  4, "\xFF\xFF\xFF\xFB", 4, "\xFF\xFF\xFF\xFF" },
	{ 1, "\x02", 8, "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFE", 0,
	  1, "\x03", 8, "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF" },
	{ 12, "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x00\x00\x00\x03", 4, "\xFF\xFF\xFF\xFF", 0,
	  12, "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFE\x00\x00\x00\x04", 13, "\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" },
	{ 12, "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x00\x00\x00\x04", 4, "\xFF\xFF\xFF\xFF", 0,
	  12, "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFE\x00\x00\x00\x05", 13, "\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" },
	{ 12, "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x00\x00\x00\x05", 4, "\xFF\xFF\xFF\xFF", 0,
	  12, "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFE\x00\x00\x00\x06", 13, "\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" },
		{ 0, NULL, 0, NULL, 0, 0, NULL }, { 0, NULL, 0, NULL, 0, 0, NULL }
	};

static const SELFTEST_VALUE modMulSelftestValues[] = {
	/* a, b, result, modulus */
	{ 1, "\x03", 1, "\x04", 0,
	  1, "\x02", 1, "\x05" },
	{ 1, "\x07", 1, "\x07", 0,
	  1, "\x04", 1, "\x05" },
	{ 4, "\x12\x34\x56\x78", 4, "\x9A\xBC\xDE\xF1", 0,
	  4, "\x41\x62\x61\x46", 4, "\x55\x55\x55\x55" },
	{ 8, "\x12\x34\x56\x78\x9A\xBC\xDE\xF1", 8, "\xAA\x55\xAA\x55\xAA\x55\xAA\x55", 0,
	  8, "\x02\xBF\xCF\x99\x0F\xFA\x44\x09", 8, "\x0C\x1C\xD8\xE9\x99\x99\x99\x8E" },
	{ 8, "\x12\x34\x56\x78\x9A\xBC\xDE\xF1", 8, "\xAA\x55\xAA\x55\xAA\x55\xAA\x55", 0,
	  8, "\x01\x1D\x9A\xC1\xA9\x93\xDC\xB2", 8, "\x0C\x1C\xD8\xE9\x99\x99\x99\x8F" },
	{ 8, "\x12\x34\x56\x78\x9A\xBC\xDE\xF1", 8, "\xAA\x55\xAA\x55\xAA\x55\xAA\x55", 0,
	  8, "\x0B\x98\x3E\xD3\xDC\xC7\x0F\x15", 8, "\x0C\x1C\xD8\xE9\x99\x99\x99\x90" },
		{ 0, NULL, 0, NULL, 0, 0, NULL }, { 0, NULL, 0, NULL, 0, 0, NULL }
	};

static const SELFTEST_VALUE modShiftSelftestValues[] = {
	/* a, b, result, modulus */
	{ 1, "\x02", 2, NULL, 0,
	  1, "\x02", 1, "\x03" },
	{ 4, "\x12\x34\x56\x78", 27, NULL, 0,
	  4, "\x19\x38\xFB\x5B", 4, "\xAA\x55\xAA\x55" },
	{ 2, "\xAA\x55", 27, NULL, 0,
	  4, "\x06\x6F\xAD\xD0", 4, "\x12\x34\x56\x78" },
	{ 2, "\xAA\x55", 27, NULL, 0,
	  4, "\x06\x6F\x62\xF6", 4, "\x12\x34\x56\x79" },
	{ 2, "\xAA\x55", 27, NULL, 0,
	  4, "\x06\x6F\x18\x1C", 4, "\x12\x34\x56\x7A" },
		{ 0, NULL, 0, NULL, 0, 0, NULL }, { 0, NULL, 0, NULL, 0, 0, NULL }
	};
#endif /* USE_ECDH || USE_ECDSA */

/****************************************************************************
*																			*
*								Self-test Routines							*
*																			*
****************************************************************************/

/* Test general bignum ops.  Apart from acting as a standard self-test these 
   also test the underlying building blocks used in the higher-level 
   routines whose self-tests follow */

CHECK_RETVAL_BOOL \
static BOOLEAN selfTestGeneralOps1( void )
	{
	BIGNUM a;

	/* Simple tests that don't need the support of higher-level routines 
	   like importBignum() */
	BN_init( &a );
	if( !BN_zero( &a ) )
		return( FALSE );
	if( !BN_is_zero( &a ) || BN_is_one( &a ) )
		return( FALSE );
	if( !BN_is_word( &a, 0 ) || BN_is_word( &a, 1 ) )
		return( FALSE );
	if( BN_is_odd( &a ) )
		return( FALSE );
	if( BN_get_word( &a ) != 0 )
		return( FALSE );
	if( !BN_one( &a ) )
		return( FALSE );
	if( BN_is_zero( &a ) || !BN_is_one( &a ) )
		return( FALSE );
	if( BN_is_word( &a, 0 ) || !BN_is_word( &a, 1 ) )
		return( FALSE );
	if( !BN_is_odd( &a ) )
		return( FALSE );
	if( BN_num_bytes( &a ) != 1 )
		return( FALSE );
	if( BN_get_word( &a ) != 1 )
		return( FALSE );
	BN_clear( &a );

	return( TRUE );
	}

CHECK_RETVAL_BOOL \
static BOOLEAN selfTestGeneralOps2( void )
	{
	BIGNUM a;
	int status;

	/* More complex tests that need higher-level routines like importBignum(),
	   run after the tests of components of importBignum() have concluded */
	BN_init( &a );
#if BN_BITS2 == 64
	status = importBignum( &a, "\x01\x00\x00\x00\x00\x00\x00\x00\x00", 9, 
						   1, 128, NULL, KEYSIZE_CHECK_NONE );
#else
	status = importBignum( &a, "\x01\x00\x00\x00\x00", 5, 1, 128, NULL, 
						   KEYSIZE_CHECK_NONE );
#endif /* 64- vs 32-bit */
	if( cryptStatusError( status ) )
		return( FALSE );
	if( BN_is_zero( &a ) || BN_is_one( &a ) )
		return( FALSE );
	if( BN_is_word( &a, 0 ) || BN_is_word( &a, 1 ) )
		return( FALSE );
	if( BN_is_odd( &a ) )
		return( FALSE );
	if( BN_get_word( &a ) != BN_NAN )
		return( FALSE );
	if( BN_num_bytes( &a ) != ( BN_BITS2 / 8 ) + 1 )
		return( FALSE );
	if( BN_num_bits( &a ) != BN_BITS2 + 1 )
		return( FALSE );
	if( !BN_is_bit_set( &a, BN_BITS2 ) )
		return( FALSE );
	if( BN_is_bit_set( &a, 17 ) || !BN_set_bit( &a, 17 ) || \
		!BN_is_bit_set( &a, 17 ) )
		return( FALSE );
#if BN_BITS2 == 64
	status = importBignum( &a, "\x01\x00\x00\x00\x00\x00\x00\x00\x01", 9, 
						   1, 128, NULL, KEYSIZE_CHECK_NONE );
#else
	status = importBignum( &a,	"\x01\x00\x00\x00\x01", 5, 1, 128, NULL,
						   KEYSIZE_CHECK_NONE );
#endif /* 64- vs 32-bit */
	if( cryptStatusError( status ) )
		return( FALSE );
	if( BN_is_zero( &a ) || BN_is_one( &a ) )
		return( FALSE );
	if( BN_is_word( &a, 0 ) || BN_is_word( &a, 1 ) )
		return( FALSE );
	if( !BN_is_odd( &a ) )
		return( FALSE );
	if( BN_num_bytes( &a ) != ( BN_BITS2 / 8 ) + 1 )
		return( FALSE );
	if( BN_get_word( &a ) != BN_NAN )
		return( FALSE );
	if( BN_num_bits( &a ) != BN_BITS2 + 1 )
		return( FALSE );
	if( !BN_is_bit_set( &a, BN_BITS2 ) )
		return( FALSE );
	if( BN_is_bit_set( &a, BN_BITS2 + 27 ) || \
		!BN_set_bit( &a, BN_BITS2 + 27 ) || \
		!BN_is_bit_set( &a, BN_BITS2 + 27 ) )
		return( FALSE );
	/* Setting a bit off the end of a bignum extends its size, which is why
	   the following value doesn't match the one from a few lines earlier */
	if( BN_num_bytes( &a ) != ( BN_BITS2 / 8 ) + 4 )
		return( FALSE );
	/* The bit index for indexing bits is zero-based (since 1 == 1 << 0) but
	   for counting bits is one-based, which is why the following comparison
	   looks wrong.  Yet another one of OpenSSL's many booby-traps */
	if( BN_num_bits( &a ) != BN_BITS2 + 28 )
		return( FALSE );
	BN_clear( &a );

	return( TRUE );
	}

/* Make sure that the selected operation gives the required result */

CHECK_RETVAL_BOOL STDC_NONNULL_ARG( ( 1 ) ) \
static BOOLEAN selfTestOp( const SELFTEST_VALUE *selftestValue,
						   IN_ENUM( BN_OP ) const BN_OP_TYPE op )
	{
	BN_CTX bnCTX;
	BN_MONT_CTX bnMontCTX;
	BIGNUM a, b, mod, result, expectedResult;
	BN_ULONG bWord DUMMY_INIT;
	const BOOLEAN isCompareOp = ( op == BN_OP_CMP || op == BN_OP_CMPPART ) ? \
								TRUE : FALSE;
#if defined( USE_ECDH ) || defined( USE_ECDSA )
	const BOOLEAN isModOp = ( ( op >= BN_OP_MODADD && op <= BN_OP_MODSHIFT ) || \
							  ( op == BN_OP_MONTMODMULT ) ) ? TRUE : FALSE;
#else
	const BOOLEAN isModOp = ( op == BN_OP_MONTMODMULT ) ? TRUE : FALSE;
#endif /* USE_ECDH || USE_ECDSA */
	BOOLEAN aNeg, expectedResultNeg DUMMY_INIT;
	int aLen, expectedResultLen, bValue DUMMY_INIT;
	int bnStatus = BN_STATUS, status;

	assert( isReadPtr( selftestValue, sizeof( SELFTEST_VALUE ) ) );

	REQUIRES_B( op > BN_OP_NONE && op < BN_OP_LAST );

	/* The BN_OP_MODWORD is sufficently specialised that we have to handle 
	   it specially */
	if( op == BN_OP_MODWORD )
		{
		BN_ULONG word;

		BN_init( &a );

		status = importBignum( &a, selftestValue->a, selftestValue->aLen, 
							   1, 128, NULL, KEYSIZE_CHECK_NONE );
		if( cryptStatusError( status ) )
			return( FALSE );
		word = BN_mod_word( &a, selftestValue->bWord );
		if( word != selftestValue->resultLen )
			return( FALSE );

		BN_clear( &a );

		return( TRUE );
		}

	BN_CTX_init( &bnCTX );
	BN_MONT_CTX_init( &bnMontCTX );
	BN_init( &a );
	BN_init( &b );
	BN_init( &mod );
	BN_init( &result );
#if BN_BITS2 == 64
	status = importBignum( &result,		/* Pollute result value */
						   "\x55\x55\x55\x55\x55\x55\x55\x55" \
						   "\x44\x44\x44\x44\x44\x44\x44\x44\x33\x33\x33\x33" \
						   "\x33\x33\x33\x33\x22\x22\x22\x22\x22\x22\x22\x22" \
						   "\x11\x11\x11\x11\x11\x11\x11\x11", 
						   40, 1, 128, NULL, KEYSIZE_CHECK_NONE );
#else
	status = importBignum( &result,		/* Pollute result value */
						   "\xAA\xAA\xAA\xAA\x99\x99\x99\x99" \
						   "\x88\x88\x88\x88\x77\x77\x77\x77\x66\x66\x66\x66" \
						   "\x55\x55\x55\x55\x44\x44\x44\x44\x33\x33\x33\x33" \
						   "\x22\x22\x22\x22\x11\x11\x11\x11", 40, 1, 128,
						   NULL, KEYSIZE_CHECK_NONE );
#endif /* 64- vs 32-bit */
	if( cryptStatusError( status ) )
		return( FALSE );
	BN_init( &expectedResult );

	/* Some of the quantities may have flags indicating that they're meant 
	   to be treated as negative values, so we have to extract the actual 
	   value and the negative flag from the overall value */
	aLen = selftestValue->aLen & ~BN_VAL_NEGATIVE;
	aNeg = ( selftestValue->aLen & BN_VAL_NEGATIVE ) ? TRUE : FALSE;
	expectedResultLen = selftestValue->resultLen & ~BN_VAL_NEGATIVE;
	expectedResultNeg = ( selftestValue->resultLen & BN_VAL_NEGATIVE ) ? \
						TRUE : FALSE;

	/* Set up the test data */
	status = importBignum( &a, selftestValue->a, aLen, 1, 128, NULL, 
						   KEYSIZE_CHECK_NONE );
	if( cryptStatusOK( status ) )
		{
		if( selftestValue->b != NULL )
			{
			status = importBignum( &b, selftestValue->b, selftestValue->bLen, 
								   1, 128, NULL, KEYSIZE_CHECK_NONE );
			}
		else
			{
			if( selftestValue->bLen != 0 )
				bValue = selftestValue->bLen;
			else
				bWord = selftestValue->bWord;
			}
		}
	if( cryptStatusOK( status ) && !isCompareOp )
		{
		status = importBignum( &expectedResult, selftestValue->result, 
							   expectedResultLen, 1, 128, NULL, 
							   KEYSIZE_CHECK_NONE );
		}
	if( cryptStatusOK( status ) && isModOp )
		{
		status = importBignum( &mod, selftestValue->mod, 
							   selftestValue->modLen, 1, 128, NULL, 
							   KEYSIZE_CHECK_NONE );
		}
	if( cryptStatusError( status ) )
		return( FALSE );
	if( aNeg )
		BN_set_negative( &a, TRUE );

	/* Perform the requested operation on the values */
	switch( op )
		{
		case BN_OP_CMP:
			/* This is a compare op so bnStatus is the compare result, not 
			   an error code */
			bnStatus = BN_cmp( &a, &b );
			break;

		case BN_OP_CMPPART:
			/* As before */
			bnStatus = bn_cmp_part_words( a.d, b.d, min( a.top, b.top ),
										  a.top - b.top );
			break;

		case BN_OP_ADD:
			CK( BN_add( &result, &a, &b ) );
			break;

		case BN_OP_SUB:
			BN_swap( &expectedResult, &a );
			CK( BN_sub( &result, &a, &b ) );
			break;

		case BN_OP_LSHIFT:
			CK( BN_lshift( &result, &a, bValue ) );
			break;

		case BN_OP_RSHIFT:
			BN_swap( &expectedResult, &a );
			CK( BN_rshift( &result, &a, bValue ) );
			break;

		case BN_OP_ADDWORD:
			CKPTR( BN_copy( &result, &a ) );
			CK( BN_add_word( &result, bWord ) );
			break;

		case BN_OP_SUBWORD:
			CKPTR( BN_copy( &result, &expectedResult ) );
			CKPTR( BN_copy( &expectedResult, &a ) );
			CK( BN_sub_word( &result, bWord ) );
			break;

		case BN_OP_MULWORD:
			CKPTR( BN_copy( &result, &a ) );
			CK( BN_mul_word( &result, bWord ) );
			break;

		case BN_OP_SQR:
			CK( BN_sqr( &result, &a, &bnCTX ) );
			break;

		case BN_OP_MONTMODMULT:
			CK( BN_MONT_CTX_set( &bnMontCTX, &mod, &bnCTX ) );
			CK( BN_to_montgomery( &a, &a, &bnMontCTX, &bnCTX ) );
			CK( BN_mod_mul_montgomery( &result, &a, &b, &bnMontCTX, &bnCTX ) );
			break;

#if defined( USE_ECDH ) || defined( USE_ECDSA )
		case BN_OP_MODADD:
			CK( BN_mod_add_quick( &result, &a, &b, &mod ) );
			break;

		case BN_OP_MODSUB:
			CK( BN_mod_sub_quick( &result, &a, &b, &mod ) );
			break;

		case BN_OP_MODMUL:
			CK( BN_mod_mul( &result, &a, &b, &mod, &bnCTX ) );
			break;
		
		case BN_OP_MODSHIFT:
			CK( BN_mod_lshift_quick( &result, &a, bValue, &mod ) );
			break;
#endif /* USE_ECDH || USE_ECDSA */

		default:
			return( FALSE );
		}
	if( isCompareOp )
		{
		/* The compare operations return their result as the return status 
		   so there's no result value to check.  In addition they return
		   { -1, 0, 1 } so we can't use the processed expectedResultLen 
		   value */
		if( bnStatus != selftestValue->resultLen )
			return( FALSE );

		return( TRUE );
		}
	if( bnStatusError( bnStatus ) )
		return( FALSE );

	/* Make sure that we got what we were expecting */
	if( BN_cmp( &result, &expectedResult ) )
		return( FALSE );
	if( expectedResultNeg && !BN_is_negative( &result ) )
		return( FALSE );

	BN_clear( &expectedResult );
	BN_clear( &result );
	BN_clear( &mod );
	BN_clear( &b );
	BN_clear( &a );
	BN_MONT_CTX_free( &bnMontCTX );
	BN_CTX_final( &bnCTX );

	return( TRUE );
	}

CHECK_RETVAL_BOOL STDC_NONNULL_ARG( ( 1 ) ) \
static BOOLEAN selfTestOps( const SELFTEST_VALUE *selftestValueArray,
							IN_RANGE( 1, 50 ) const int selftestValueArraySize,
							IN_ENUM( BN_OP ) const BN_OP_TYPE op,
							const char *opDescription )
	{
	int i;

	assert( isReadPtr( selftestValueArray, 
					   selftestValueArraySize * sizeof( SELFTEST_VALUE ) ) );

	REQUIRES_B( selftestValueArraySize >= 1 && selftestValueArraySize < 50 );
	REQUIRES_B( op > BN_OP_NONE && op < BN_OP_LAST );

	for( i = 0; selftestValueArray[ i ].a != NULL && \
				i < selftestValueArraySize; i++ )
		{
		if( !selfTestOp( &selftestValueArray[ i ], op ) )
			{
			DEBUG_PRINT(( "Failed %s test #%d.\n", opDescription, i ));
			return( FALSE );
			}
		}
	ENSURES_B( i < selftestValueArraySize );

	return( TRUE );
	}

CHECK_RETVAL_BOOL \
BOOLEAN bnmathSelfTest( void )
	{
	if( !selfTestGeneralOps1() )
		{
		DEBUG_PRINT(( "Failed general operations test phase 1.\n" ));
		return( FALSE );
		}
	if( !selfTestOps( cmpSelftestValues, FS_SIZE( cmpSelftestValues ),
					  BN_OP_CMP, "BN_OP_CMP" ) )
		return( FALSE );
	if( !selfTestOps( cmpSelftestValues, FS_SIZE( cmpSelftestValues ),
					  BN_OP_CMPPART, "BN_OP_CMPPART" ) )
		return( FALSE );
	if( !selfTestGeneralOps2() )
		{
		DEBUG_PRINT(( "Failed general operations test phase 1.\n" ));
		return( FALSE );
		}
	if( !selfTestOps( addsubSelftestValues, FS_SIZE( addsubSelftestValues ),
					  BN_OP_ADD, "BN_OP_ADD" ) )
		return( FALSE );
	if( !selfTestOps( addsubSelftestValues, FS_SIZE( addsubSelftestValues ),
					  BN_OP_SUB, "BN_OP_SUB" ) )
		return( FALSE );
	if( !selfTestOps( shiftSelftestValues, FS_SIZE( shiftSelftestValues ),
					  BN_OP_LSHIFT, "BN_OP_LSHIFT" ) )
		return( FALSE );
	if( !selfTestOps( shiftSelftestValues, FS_SIZE( shiftSelftestValues ),
					  BN_OP_RSHIFT, "BN_OP_RSHIFT" ) )
		return( FALSE );
	if( !selfTestOps( addsubWordsSelftestValues, FS_SIZE( addsubWordsSelftestValues ),
					  BN_OP_ADDWORD, "BN_OP_ADDWORD" ) )
		return( FALSE );
	if( !selfTestOps( addsubWordsSelftestValues, FS_SIZE( addsubWordsSelftestValues ),
					  BN_OP_SUBWORD, "BN_OP_SUBWORD" ) )
		return( FALSE );
	if( !selfTestOps( muldivWordsSelftestValues, FS_SIZE( muldivWordsSelftestValues ),
					  BN_OP_MULWORD, "BN_OP_MULWORD" ) )
		return( FALSE );
	if( !selfTestOps( modWordsSelftestValues, FS_SIZE( modWordsSelftestValues ),
					  BN_OP_MODWORD, "BN_OP_MODWORD" ) )
		return( FALSE );
	if( !selfTestOps( sqrSelftestValues, FS_SIZE( sqrSelftestValues ),
					  BN_OP_SQR, "BN_OP_SQR" ) )
		return( FALSE );
	if( !selfTestOps( montModMulSelftestValues, FS_SIZE( montModMulSelftestValues ),
					  BN_OP_MONTMODMULT, "BN_OP_MONTMODMULT" ) )
		return( FALSE );
#if defined( USE_ECDH ) || defined( USE_ECDSA )
	if( !selfTestOps( modAddSelftestValues, FS_SIZE( modAddSelftestValues ),
					  BN_OP_MODADD, "BN_OP_MODADD" ) )
		return( FALSE );
	if( !selfTestOps( modSubSelftestValues, FS_SIZE( modSubSelftestValues ),
					  BN_OP_MODSUB, "BN_OP_MODSUB" ) )
		return( FALSE );
	if( !selfTestOps( modMulSelftestValues, FS_SIZE( modMulSelftestValues ),
					  BN_OP_MODMUL, "BN_OP_MODMUL" ) )
		return( FALSE );
	if( !selfTestOps( modShiftSelftestValues, FS_SIZE( modShiftSelftestValues ),
					  BN_OP_MODSHIFT, "BN_OP_MODSHIFT" ) )
		return( FALSE );
#endif /* USE_ECDH || USE_ECDSA */

	return( TRUE );
	}
#endif /* NDEBUG */

#endif /* USE_PKC */
