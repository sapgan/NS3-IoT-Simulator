/****************************************************************************
*																			*
*						cryptlib Bignum Maths Routines						*
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

/****************************************************************************
*																			*
*								Add/Subtract Bignums						*
*																			*
****************************************************************************/

/* Add and subtract two bignums, r = a + b, r = a - b */

CHECK_RETVAL_BOOL STDC_NONNULL_ARG( ( 1, 2, 3 ) ) \
BOOLEAN BN_uadd( INOUT BIGNUM *r, const BIGNUM *a, const BIGNUM *b )
	{
	BN_ULONG carry;
	const int oldTop = r->top;
	int length = max( a->top, b->top );

	assert( isWritePtr( r, sizeof( BIGNUM ) ) );
	assert( isReadPtr( a, sizeof( BIGNUM ) ) );
	assert( isReadPtr( b, sizeof( BIGNUM ) ) );

	REQUIRES_B( sanityCheckBignum( a ) );
	REQUIRES_B( sanityCheckBignum( b ) );

	/* Add the two values, propagating the carry if required */
	carry = bn_add_words( r->d, a->d, b->d, length );
	if( carry )
		r->d[ length++ ] = 1;
	r->top = length;
	BN_set_negative( r, FALSE );
	BN_clear_top( r, oldTop );

	ENSURES_B( sanityCheckBignum( r ) );

	return( TRUE );
	}

/* Subtract two bignums, r = a - b */

CHECK_RETVAL_BOOL STDC_NONNULL_ARG( ( 1, 2, 3 ) ) \
BOOLEAN BN_usub( INOUT BIGNUM *r, const BIGNUM *a, const BIGNUM *b )
	{
	BN_ULONG carry;
	const int oldTop = r->top;
	int length = max( a->top, b->top );

	assert( isWritePtr( r, sizeof( BIGNUM ) ) );
	assert( isReadPtr( a, sizeof( BIGNUM ) ) );
	assert( isReadPtr( b, sizeof( BIGNUM ) ) );

	REQUIRES_B( sanityCheckBignum( a ) );
	REQUIRES_B( sanityCheckBignum( b ) );
	REQUIRES_B( BN_cmp( a, b ) >= 0 );

	/* Subtract the two values.  The carry should be zero since a >= b */
	carry = bn_sub_words( r->d, a->d, b->d, length );
	ENSURES_B( !carry );
	r->top = length;
	BN_set_negative( r, FALSE );
	BN_clear_top( r, oldTop );

	/* The subtraction may have reduced the size of the resulting value so 
	   we have to normalise it before we return */
	if( !BN_normalise( r ) )
		return( FALSE );

	ENSURES_B( sanityCheckBignum( r ) );

	return( TRUE );
	}

/* Signed versions of the above: r = a + b, r = a - b.  These are just 
   wrappers around BN_uadd()/BN_usub() that deal with sign bits as
   appropriate */

CHECK_RETVAL_BOOL STDC_NONNULL_ARG( ( 1, 2, 3 ) ) \
BOOLEAN BN_add( INOUT BIGNUM *r, const BIGNUM *a, const BIGNUM *b )
	{
	assert( isWritePtr( r, sizeof( BIGNUM ) ) );
	assert( isReadPtr( a, sizeof( BIGNUM ) ) );
	assert( isReadPtr( b, sizeof( BIGNUM ) ) );

	/* a can be negative via the BN_mod_inverse() used in Montgomery 
	   ops */
	REQUIRES_B( sanityCheckBignum( a ) );
	REQUIRES_B( sanityCheckBignum( b ) && !BN_is_negative( b ) );

	/* If a is negative then -a + b becomes b - a.  We can't pass this down 
	   to BN_sub() because that expects positive numbers so we have to 
	   handle the special case here, but in any case it's straightforward
	   because abs( a ) <= b so the result is always a positive number */
	if( BN_is_negative( a ) ) 
		{
		REQUIRES_B( BN_ucmp( a, b ) <= 0 ); 

		if( !BN_usub( r, b, a ) )
			return( FALSE );
		BN_set_negative( r, FALSE );

		return( TRUE );
		}

	/* It's a straight add */
	return( BN_uadd( r, a, b ) );
	}

CHECK_RETVAL_BOOL STDC_NONNULL_ARG( ( 1, 2, 3 ) ) \
BOOLEAN BN_sub( INOUT BIGNUM *r, const BIGNUM *a, const BIGNUM *b )
	{
	assert( isWritePtr( r, sizeof( BIGNUM ) ) );
	assert( isReadPtr( a, sizeof( BIGNUM ) ) );
	assert( isReadPtr( b, sizeof( BIGNUM ) ) );

	REQUIRES_B( sanityCheckBignum( a ) && !BN_is_negative( a ) );
	REQUIRES_B( sanityCheckBignum( b ) && !BN_is_negative( b ) );

	/* If a < b then the result is the difference as a negative number */
	if( BN_ucmp( a, b ) < 0 ) 
		{
		if( !BN_usub( r, b, a ) )
			return( FALSE );
		BN_set_negative( r, TRUE );

		return( TRUE );
		}

	/* It's a straight subtract */
	return( BN_usub( r, a, b ) );
	}

/****************************************************************************
*																			*
*								Shift Bignums								*
*																			*
****************************************************************************/

/* Shift a bignum left or right */

CHECK_RETVAL_BOOL STDC_NONNULL_ARG( ( 1, 2 ) ) \
BOOLEAN BN_lshift( INOUT BIGNUM *r, const BIGNUM *a, 
				   IN_RANGE( 0, bytesToBits( CRYPT_MAX_PKCSIZE ) ) \
						const int shiftAmount )
	{
	const int wordShiftAmount = shiftAmount / BN_BITS2;
	const int bitShiftAmount = shiftAmount % BN_BITS2;
	const int bitShiftRemainder = BN_BITS2 - bitShiftAmount;
	const BN_ULONG *aData = a->d;
	const int oldTop = r->top;
	const int iterationBound = getBNMaxSize( a );
	BN_ULONG *rData = r->d, left, right;
	int iterationCount, i;

	assert( isWritePtr( r, sizeof( BIGNUM ) ) );
	assert( isReadPtr( a, sizeof( BIGNUM ) ) );

	REQUIRES_B( sanityCheckBignum( a ) && !BN_is_zero( a ) );
	REQUIRES_B( shiftAmount > 0 && \
				shiftAmount < bytesToBits( CRYPT_MAX_PKCSIZE ) );
	REQUIRES_B( a->top + wordShiftAmount < getBNMaxSize( r ) );

	/* Copy across the sign bit.  a can be negative via the BN_mod_inverse() 
	   used in Montgomery ops */
	BN_set_negative( r, BN_is_negative( a ) );

	/* If we're shifting a word at a time then it's just a straight copy 
	   operation */
	if( bitShiftAmount == 0 )
		{
		/* Move the words up, starting from the MSB and moving down to the 
		   LSB */
		for( i = a->top - 1, iterationCount = 0; 
			 i >= 0 && iterationCount < iterationBound; 
			 i--, iterationCount++ )
			{
			rData[ wordShiftAmount + i ] = aData[ i ];
			}
		ENSURES_B( iterationCount < iterationBound );

		/* Set the new top based on what we've shifted up */
		r->top = a->top + wordShiftAmount;
		}
	else
		{
		/* Shift everything up by taking two words and extracting out the 
		   single word of shifted data that we need */
		right = 0;
		for( i = a->top - 1, iterationCount = 0; 
			 i >= 0 && iterationCount < iterationBound; 
			 i--, iterationCount++ ) 
			{
			left = aData[ i ];
			rData[ wordShiftAmount + i + 1 ] = \
				( right << bitShiftAmount ) | ( left >> bitShiftRemainder );
			right = left;
			}
		ENSURES_B( iterationCount < iterationBound );
		rData[ wordShiftAmount ] = right << bitShiftAmount;

		/* Set the new top based on what we've shifted up */
		r->top = a->top + wordShiftAmount;
		if( rData[ r->top ] != 0 )
			{
			/* We shifted bits off the end of the last word, extend the 
			   length by one word */
			r->top++;
			}
		}
	BN_clear_top( r, oldTop );

	/* Clear the space that we've shifted up from.  This can become an issue 
	   when r == a */
	for( i = 0, iterationCount = 0; 
		 i < wordShiftAmount && iterationCount < iterationBound; 
		 i++, iterationCount++ )
		{ 
		rData[ i ] = 0;
		}
	ENSURES_B( iterationCount < iterationBound );

	ENSURES_B( sanityCheckBignum( r ) );

	return( TRUE );
	}

CHECK_RETVAL_BOOL STDC_NONNULL_ARG( ( 1, 2 ) ) \
BOOLEAN BN_rshift( INOUT BIGNUM *r, const BIGNUM *a, 
				   IN_RANGE( 0, bytesToBits( CRYPT_MAX_PKCSIZE ) ) \
						const int shiftAmount )
	{
	const int wordShiftAmount = shiftAmount / BN_BITS2;
	const int bitShiftAmount = shiftAmount % BN_BITS2;
	const int bitShiftRemainder = BN_BITS2 - bitShiftAmount;
	const int wordsToShift = a->top - wordShiftAmount;
	const BN_ULONG *aData = a->d;
	const int oldTop = r->top;
	const int iterationBound = getBNMaxSize( a );
	BN_ULONG *rData = r->d, left, right;
	int iterationCount, i;

	assert( isWritePtr( r, sizeof( BIGNUM ) ) );
	assert( isReadPtr( a, sizeof( BIGNUM ) ) );

	REQUIRES_B( sanityCheckBignum( a ) && !BN_is_negative( a ) );
	REQUIRES_B( shiftAmount > 0 && \
				shiftAmount < bytesToBits( CRYPT_MAX_PKCSIZE ) );
	REQUIRES_B( wordShiftAmount < a->top || BN_is_zero( a ) );
	REQUIRES_B( wordShiftAmount + wordsToShift < getBNMaxSize( r ) );

	/* a can be zero when we're being called from routines like 
	   BN_mod_inverse() that iterate until a certain value is reached */
	if( BN_is_zero( a ) )
		{
		int bnStatus = BN_STATUS;

		CK( BN_zero( r ) );
		ENSURES_B( bnStatusOK( bnStatus ) );

		return( TRUE );
		}

	/* Clear the sign bit.  We know that it's not set since a can't be 
	   negative, so we just clear it unconditionally */ 
	BN_set_negative( r, FALSE );

	/* If we're shifting a word at a time then it's just a straight copy 
	   operation */
	if( bitShiftAmount == 0 ) 
		{
		/* Move the words down, starting from the LSB and moving down to the 
		   MSB */
		for( i = 0, iterationCount = 0; 
			 i < wordsToShift && iterationCount < iterationBound; 
			 i++, iterationCount++ )
			{
			rData[ i ] = aData[ wordShiftAmount + i ];
			}
		ENSURES_B( iterationCount < iterationBound );

		/* Set the new top based on what we've shifted down */
		r->top = wordsToShift;
		} 
	else
		{
		/* Shift everything down by taking two words and extracting out the 
		   single word of shifted data that we need.  Since we're taking two 
		   words at a time (i.e. reading ahead by one word), we only iterate
		   to wordsToShift - 1 */
		left = aData[ wordShiftAmount ];
		for( i = 0, iterationCount = 0; 
			 i < wordsToShift - 1 && iterationCount < iterationBound; 
			 i++, iterationCount++ ) 
			{
			right = aData[ wordShiftAmount + i + 1 ];
			rData[ i ] = ( left >> bitShiftAmount ) | ( right << bitShiftRemainder );
			left = right;
			}
		ENSURES_B( iterationCount < iterationBound );

		/* Set the new top based on what we've shifted down */
		r->top = wordsToShift - 1;

		/* Add in any remaining bits if required */
		left >>= bitShiftAmount;
		if( left > 0 )
			rData[ r->top++ ] = left;
		}
	BN_clear_top( r, oldTop );

	ENSURES_B( sanityCheckBignum( r ) );

	return( TRUE );
	}

/****************************************************************************
*																			*
*							Perform Word Ops on Bignums						*
*																			*
****************************************************************************/

/* Add and subtract words to/from a bignum */

CHECK_RETVAL_BOOL STDC_NONNULL_ARG( ( 1 ) ) \
BOOLEAN BN_add_word( INOUT BIGNUM *a, const BN_ULONG w )
	{
	BN_ULONG *aData = a->d, word = w;
	const int iterationBound = getBNMaxSize( a );
	int iterationCount, i;

	assert( isWritePtr( a, sizeof( BIGNUM ) ) );

	REQUIRES_B( sanityCheckBignum( a ) && !BN_is_zero( a ) && \
				!BN_is_negative( a ) );
	REQUIRES_B( word > 0 );

	/* Do an add with carry.  We can't use the bn_xxx_words() form because
	   it adds the words of two bignums, not a word to a bignum */
	for( i = 0, iterationCount = 0; 
		 i < a->top && iterationCount < iterationBound; 
		 i++, iterationCount++ )
		{
		aData[ i ] += word;

		/* If there wasn't an overflow, we're done, otherwise continue with
		   carry */
		if( word <= aData[ i ] )
			break;
		word = 1;
		}
	ENSURES_B( iterationCount < iterationBound );

	/* If we've overflowed onto a new word, increase the length of the 
	   bignum.  The logic here is as follows, if we got as far as a->top
	   without exiting the loop (so i >= a->top) then we're propagating a
	   carry (a->top is at least 1 since a is non-zero so we've passed at 
	   least once through the loop, setting word = 1), so we know that the 
	   top word has the value 1 */
	if( i >= a->top )
		aData[ a->top++ ] = 1;

	ENSURES_B( sanityCheckBignum( a ) );

	return( TRUE );
	}

CHECK_RETVAL_BOOL STDC_NONNULL_ARG( ( 1 ) ) \
BOOLEAN BN_sub_word( INOUT BIGNUM *a, const BN_ULONG w )
	{
	BN_ULONG *aData = a->d, word = w;
	const int iterationBound = getBNMaxSize( a );
	int iterationCount, i;

	assert( isWritePtr( a, sizeof( BIGNUM ) ) );

	REQUIRES_B( sanityCheckBignum( a ) && !BN_is_zero( a ) && \
				!BN_is_negative( a ) );
	REQUIRES_B( word > 0 );
	REQUIRES_B( a->top > 1 || a->d[ 0 ] >= w );
				/* Result shouldn't go negative */

	/* The bignum is larger than the value that we're subtracting, do a 
	   simple subtract with carry.  At this point we again run into the
	   bizarro non-orthogonality of the bn_xxx_words() routines where they
	   sometimes act on { bignum, word }, sometimes on { bignum, bignum },
	   and sometimes on { hiWord, loWord, word }.  In this case it's
	   { bignum, bignum } so we have to synthesise the { bignum, word }
	   form outselves */
	for( i = 0, iterationCount = 0; 
		 i < a->top && iterationCount < iterationBound; 
		 i++, iterationCount++ )
		{
		/* If we can satisfy the subtract from the current bignum word then
		   we're done */
		if( aData[ i ] >= word )
			{
			aData[ i ] -= word;
			break;
			}

		/* Subtract the word with carry */
		aData[ i ] -= word;
		word = 1;
		}
	ENSURES_B( iterationCount < iterationBound );

	/* If we've cleared the top word, decrease the overall bignum length */
	if( aData[ a->top - 1 ] == 0 )
		a->top--;

	ENSURES_B( sanityCheckBignum( a ) );

	return( TRUE );
	}

/* Multiply and divide (more accurately, perform a modulo operation) words 
   to/from a bignum */

CHECK_RETVAL_BOOL STDC_NONNULL_ARG( ( 1 ) ) \
BOOLEAN BN_mul_word( INOUT BIGNUM *a, const BN_ULONG w )
	{
	BN_ULONG *aData = a->d, word;

	assert( isWritePtr( a, sizeof( BIGNUM ) ) );

	REQUIRES_B( sanityCheckBignum( a ) && !BN_is_zero( a ) && \
				!BN_is_negative( a ) );
	REQUIRES_B( w > 0 );

	/* For once we've got a bn_xxx_words() form that's actually useful, so 
	   we just call down to that */
	word = bn_mul_words( aData, aData, a->top, w );
	if( word > 0 )
		aData[ a->top++ ] = word;

	ENSURES_B( sanityCheckBignum( a ) );

	return( TRUE );
	}

STDC_NONNULL_ARG( ( 1 ) ) \
BN_ULONG BN_mod_word( const BIGNUM *a, const BN_ULONG w )
	{
	const BN_ULONG *aData = a->d;
	BN_ULONG value = 0;
	const int iterationBound = getBNMaxSize( a );
	int iterationCount, i;

	assert( isReadPtr( a, sizeof( BIGNUM ) ) );

	REQUIRES_B( sanityCheckBignum( a ) && !BN_is_zero( a ) && \
				!BN_is_negative( a ) );
	REQUIRES_B( w > 0 );

	/* Now we run into yet another member of the non-orthogonal OpenSSL zoo,
	   this time a function that divides hi:lo by a word, so
	   result = bn_div_words( hi, lo, word ).  To work with this, we walk
	   down the bignum a double-word at a time, propagating the result as
	   we go */
	for( i = a->top - 1, iterationCount = 0; 
		 i >= 0 && iterationCount < iterationBound; 
		 i--, iterationCount++ )
		{
		BN_ULONG tmp;

		tmp = bn_div_words( value, aData[ i ], w );
		value = aData[ i ] - ( tmp * w );
		}
	ENSURES_B( iterationCount < iterationBound );

	return( value );
	}

/****************************************************************************
*																			*
*									Square Bignums							*
*																			*
****************************************************************************/

/* Square an array of BN_ULONGs */

CHECK_RETVAL_BOOL STDC_NONNULL_ARG( ( 1, 2, 4 ) ) \
static BOOLEAN bn_square( INOUT BN_ULONG *r, const BN_ULONG *a, 
						  IN_RANGE( 0, BIGNUM_ALLOC_WORDS ) const int length, 
						  INOUT BN_ULONG *tmp )
	{
	BN_ULONG carry;
	int max = length * 2, iterationCount, i;

	assert( isReadPtr( r, sizeof( BN_ULONG ) * length ) );
	assert( isReadPtr( a, sizeof( BN_ULONG ) * length ) );
	assert( isReadPtr( tmp, sizeof( BN_ULONG ) * ( length * 2 ) ) );

	REQUIRES_B( length > 0 && length <= BIGNUM_ALLOC_WORDS );

	/* Walk up the bignum multiplying out the words in it.  For the least-
	   significant word it's a straight multiply, for subsequent words it's
	   a multiply-accumulate.  To see what's going on here it's helpful to 
	   consider each round in turn:

		r[ length ]		=	 mul( r + 1, a + 1, length - 1, a[ 0 ] );
		r[ length + 1 ] = mulacc( r + 3, a + 2, length - 2, a[ 1 ] );
		r[ length + 2 ] = mulacc( r + 5, a + 3, length - 3, a[ 2 ] );
		r[ length + 3 ] = mulacc( r + 7, a + 4, length - 4, a[ 3 ] );

	   Since we're doubling the value, the low word becomes zero, and we also
	   set the high word to zero to handle possible carries in the 
	   bn_add_words() that follows */
	r[ 0 ] = r[ max - 1 ] = 0;
	if( length > 1 )
		{
		r[ length ] = bn_mul_words( &r[ 1 ], &a[ 1 ], length - 1, a[ 0 ] );
		for( i = 1, iterationCount = 0; 
			 i < length - 1 && iterationCount < BIGNUM_ALLOC_WORDS; 
			 i++, iterationCount++ )
			{
			r[ length + i ] = bn_mul_add_words( &r[ ( i * 2 ) + 1 ], 
												&a[ i + 1 ], length - ( i + 1 ), 
												a[ i ] );
			}
		ENSURES_B( iterationCount < BIGNUM_ALLOC_WORDS );
		}
	carry = bn_add_words( r, r, r, max );
	ENSURES_B( carry == 0 );
	bn_sqr_words( tmp, a, length );
	carry = bn_add_words( r, r, tmp, max );
	ENSURES_B( carry == 0 );

	return( TRUE );
	}

/* Square a bignum: r = a * a */

CHECK_RETVAL_BOOL STDC_NONNULL_ARG( ( 1, 2, 3 ) ) \
BOOLEAN BN_sqr( INOUT BIGNUM *r, const BIGNUM *a, INOUT BN_CTX *bnCTX )
	{
	BIGNUM *rTmp = r, *tmp;
	const int length = a->top;
	int oldTop, bnStatus = BN_STATUS;

	assert( isWritePtr( r, sizeof( BIGNUM ) ) );
	assert( isReadPtr( a, sizeof( BIGNUM ) ) );
	assert( isWritePtr( bnCTX, sizeof( BN_CTX ) ) );

	REQUIRES_B( sanityCheckBignum( a ) && !BN_is_zero( a ) && \
				!BN_is_negative( a ) );
	REQUIRES_B( length > 0 && length < BIGNUM_ALLOC_WORDS );
	REQUIRES_B( 2 * a->top <= getBNMaxSize( r ) );

	BN_CTX_start( bnCTX );
	if( a == r )
		{
		/* If the input is the same as the output, we need to use
		   a temporary value to compute the result into */
		rTmp = BN_CTX_get( bnCTX );
		if( rTmp == NULL )
			{
			BN_CTX_end( bnCTX );
			return( FALSE );
			}
		}
	oldTop = rTmp->top;

	/* Call the internal bn_square() function with a temporary as scratch 
	   space */
	tmp = BN_CTX_get_ext( bnCTX, BIGNUM_EXT_MUL1 );
	ENSURES_B( tmp != NULL );
	BN_set_flags( tmp, BN_FLG_SCRATCH );
	CK( bn_square( rTmp->d, a->d, length, tmp->d ) );
	if( bnStatusError( bnStatus ) )
		{
		BN_CTX_end_ext( bnCTX, BIGNUM_EXT_MUL1 );
		return( bnStatus );
		}

	/* Squaring a value typically doubles its size, however if the top word
	   of the original value has the high half clear then the result is one 
	   word shorter */
	rTmp->top = 2 * length;
	if( ( a->d[ length - 1 ] & BN_MASK2h ) == 0 )
		rTmp->top--;
	BN_clear_top( rTmp, oldTop );
	if( rTmp != r )
		{
		/* Since the input was the same as the output we need to copy the 
		   temporary back to the output */
		CKPTR( BN_copy( r, rTmp ) );
		if( bnStatusError( bnStatus ) )
			{
			BN_CTX_end_ext( bnCTX, BIGNUM_EXT_MUL1 );
			return( bnStatus );
			}
		}

	/* Clean up */
	BN_CTX_end_ext( bnCTX, BIGNUM_EXT_MUL1 );

	ENSURES_B( sanityCheckBignum( r ) );

	return( TRUE );
	}

/****************************************************************************
*																			*
*						Perform Modulus Ops on Bignums						*
*																			*
****************************************************************************/

/* BN_mod() variant that always returns a positive result (nn = non-
   negative) in the range { 0...abs( d ) - 1 }.  m can be negative via the 
   BN_mod_inverse() used in Montgomery ops */

CHECK_RETVAL_BOOL STDC_NONNULL_ARG( ( 1, 2, 3, 4 ) ) \
BOOLEAN BN_nnmod( INOUT BIGNUM *r, const BIGNUM *m, const BIGNUM *d, 
				  INOUT BN_CTX *ctx )
	{
	int bnStatus = BN_STATUS;

	assert( isWritePtr( r, sizeof( BIGNUM ) ) );
	assert( isReadPtr( m, sizeof( BIGNUM ) ) );
	assert( isReadPtr( d, sizeof( BIGNUM ) ) );
	assert( isWritePtr( ctx, sizeof( BN_CTX ) ) );

	REQUIRES_B( sanityCheckBignum( m ) && !BN_is_zero( m ) );
	REQUIRES_B( sanityCheckBignum( d ) && !BN_is_zero( d ) && \
				!BN_is_negative( d ) );
	REQUIRES_B( sanityCheckBNCTX( ctx ) );

	/* Perform the mod operation and, if the result is negative, add d to 
	   make it positive again */
	CK( BN_mod( r, m, d, ctx ) );
	if( bnStatusOK( bnStatus) && BN_is_negative( r ) )
		CK( BN_add( r, r, d ) );
	if( bnStatusError( bnStatus ) )
		return( bnStatus );

	ENSURES_B( sanityCheckBignum( r ) );

	return( TRUE );
	}

#if defined( USE_ECDH ) || defined( USE_ECDSA )

/* BN_mod_add/sub that assume that a and b are positive and less than m, 
   which avoids the need for an expensive mod operation */

CHECK_RETVAL_BOOL STDC_NONNULL_ARG( ( 1, 2, 3, 4 ) ) \
BOOLEAN BN_mod_add_quick( INOUT BIGNUM *r, const BIGNUM *a, const BIGNUM *b,
						  const BIGNUM *m )
	{
	int bnStatus = BN_STATUS;

	assert( isWritePtr( r, sizeof( BIGNUM ) ) );
	assert( isReadPtr( a, sizeof( BIGNUM ) ) );
	assert( isReadPtr( b, sizeof( BIGNUM ) ) );
	assert( isReadPtr( m, sizeof( BIGNUM ) ) );

	REQUIRES_B( sanityCheckBignum( a ) && !BN_is_zero( a ) && \
				!BN_is_negative( a ) );
	REQUIRES_B( sanityCheckBignum( b ) && !BN_is_zero( b ) && \
				!BN_is_negative( a ) );
	REQUIRES_B( sanityCheckBignum( m ) && !BN_is_zero( m ) && \
				!BN_is_negative( m ) );
	REQUIRES_B( BN_ucmp( a, m ) < 0 &&  BN_ucmp( b, m ) < 0 );

	/* Quick form of mod that subtracts m if the product ends up greater 
	   than m */
	CK( BN_uadd( r, a, b ) );
	if( bnStatusOK( bnStatus) && BN_ucmp( r, m ) >= 0 )
		{
		/* r is bigger than m, get it back within range */
		CK( BN_usub( r, r, m ) );
		}
	if( bnStatusError( bnStatus ) )
		return( bnStatus );

	ENSURES_B( sanityCheckBignum( r ) );

	return( TRUE );
	}

CHECK_RETVAL_BOOL STDC_NONNULL_ARG( ( 1, 2, 3, 4 ) ) \
BOOLEAN BN_mod_sub_quick( INOUT BIGNUM *r, const BIGNUM *a, const BIGNUM *b,
						  const BIGNUM *m )
	{
	int bnStatus = BN_STATUS;

	assert( isWritePtr( r, sizeof( BIGNUM ) ) );
	assert( isReadPtr( a, sizeof( BIGNUM ) ) );
	assert( isReadPtr( b, sizeof( BIGNUM ) ) );
	assert( isReadPtr( m, sizeof( BIGNUM ) ) );

	REQUIRES_B( sanityCheckBignum( a ) && !BN_is_zero( a ) && \
				!BN_is_negative( a ) );
	REQUIRES_B( sanityCheckBignum( b ) && !BN_is_zero( b ) && \
				!BN_is_negative( a ) );
	REQUIRES_B( sanityCheckBignum( m ) && !BN_is_zero( m ) && \
				!BN_is_negative( m ) );
	REQUIRES_B( BN_ucmp( a, m ) < 0 &&  BN_ucmp( b, m ) < 0 );

	/* Quick form of mod that adds m if the product ends up negative.  We 
	   have to use BN_sub() rather than BN_usub() since b can be larger
	   than a so that r goes negative */
	CK( BN_sub( r, a, b ) );
	if( bnStatusOK( bnStatus) && BN_is_negative( r ) )
		{
		/* r is negative, get it back within range.  Note that we have to use 
		   BN_add() rather than BN_uadd() since r is negative */
		CK( BN_add( r, r, m ) );
		}
	if( bnStatusError( bnStatus ) )
		return( bnStatus );

	ENSURES_B( sanityCheckBignum( r ) );

	return( TRUE );
	}

/* BN_mod_lshift() that assumes that a is positive and less than m, avoiding
   an expensive modulus operation */

CHECK_RETVAL_BOOL STDC_NONNULL_ARG( ( 1, 2, 4 ) ) \
BOOLEAN BN_mod_lshift_quick( BIGNUM *r, const BIGNUM *a,
							 IN_RANGE( 0, bytesToBits( CRYPT_MAX_PKCSIZE ) ) \
								const int shiftAmount,
							 const BIGNUM *m )
	{
	int shiftCount, iterationCount, bnStatus = BN_STATUS;

	assert( isWritePtr( r, sizeof( BIGNUM ) ) );
	assert( isReadPtr( a, sizeof( BIGNUM ) ) );
	assert( isReadPtr( m, sizeof( BIGNUM ) ) );

	REQUIRES_B( sanityCheckBignum( a ) && !BN_is_zero( a ) && \
				!BN_is_negative( a ) );
	REQUIRES_B( shiftAmount > 0 && \
				shiftAmount < bytesToBits( CRYPT_MAX_PKCSIZE ) );
	REQUIRES_B( sanityCheckBignum( m ) && !BN_is_zero( m ) && \
				!BN_is_negative( m ) );
	REQUIRES_B( BN_cmp( a, m ) < 0 );

	/* Initialise the output value if it's not the same as the input */
	if( r != a )
		{
		if( BN_copy( r, a ) == NULL )
			return( FALSE );
		}

	/* Convert a shift-and-mod into a series of far less expensive shift-and-
	   subtracts.  We do this by ensuring that the shifts are capped so that
	   r can only grow to the same size as m, in which case the reduction 
	   step is just a subtract */
	for( shiftCount = shiftAmount, iterationCount = 0;
		 shiftCount > 0 && iterationCount < bytesToBits( CRYPT_MAX_PKCSIZE );
		iterationCount++ )
		{
		int shift;

		/* Set a shift amount that ensures that we can still do a reduction
		   with a simple subtract.  The first time through this shifts
		   sufficient bits that r grows to the same size as m, after which
		   it typically shifts one bit at a time, so the total number of
		   iterations is 1 + ( sizeof_bits( m ) - sizeof_bits( a ) ) */
		shift = BN_num_bits( m ) - BN_num_bits( r );
		ENSURES_B( shift >= 0 && shift < bytesToBits( CRYPT_MAX_PKCSIZE ) );
		if( shift > shiftCount )
			shift = shiftCount;
		else
			{
			/* If the bignums are the same size, make sure that we shift by
			   at least one bit */
			if( shift == 0 )
				shift = 1;
			}

		/* Perform the shift and reduction */
		CK( BN_lshift( r, r, shift ) );
		if( bnStatusOK( bnStatus) && BN_cmp( r, m ) >= 0 )
			CK( BN_sub( r, r, m ) );
		if( bnStatusError( bnStatus ) )
			return( bnStatus );
		shiftCount -= shift;
		}
	ENSURES_B( iterationCount < bytesToBits( CRYPT_MAX_PKCSIZE ) );

	ENSURES_B( sanityCheckBignum( r ) );

	return( TRUE );
	}
#endif /* USE_ECDH || USE_ECDSA */

/* Generic modmult without special tricks like Montgomery maths */

CHECK_RETVAL_BOOL STDC_NONNULL_ARG( ( 1, 2, 3, 4, 5 ) ) \
BOOLEAN BN_mod_mul( INOUT BIGNUM *r, const BIGNUM *a, const BIGNUM *b,
					const BIGNUM *m, INOUT BN_CTX *ctx )
	{
	BIGNUM *tmp;
	int bnStatus = BN_STATUS;

	assert( isWritePtr( r, sizeof( BIGNUM ) ) );
	assert( isReadPtr( a, sizeof( BIGNUM ) ) );
	assert( isReadPtr( b, sizeof( BIGNUM ) ) );
	assert( isReadPtr( m, sizeof( BIGNUM ) ) );
	assert( isWritePtr( ctx, sizeof( BN_CTX ) ) );

	REQUIRES_B( sanityCheckBignum( a ) && !BN_is_zero( a ) && \
				!BN_is_negative( a ) );
	REQUIRES_B( sanityCheckBignum( b ) && !BN_is_zero( b ) && \
				!BN_is_negative( b ) );
	REQUIRES_B( sanityCheckBignum( m ) && !BN_is_zero( m ) && \
				!BN_is_negative( m ) );
	REQUIRES_B( sanityCheckBNCTX( ctx ) );

	/* If a and b are the same then we can use a more efficient squaring op 
	   rather than a multiply */
	if( !BN_cmp( a, b ) )
		return( BN_mod_sqr( r, a, m, ctx ) );

    BN_CTX_start( ctx );
    tmp = BN_CTX_get( ctx );
	if( tmp == NULL )
		{
		BN_CTX_end( ctx );
		return( FALSE );
		}
	CK( BN_mul( tmp, a, b, ctx ) );
	CK( BN_mod( r, tmp, m, ctx ) );
	BN_CTX_end( ctx );
	if( bnStatusError( bnStatus ) )
		return( bnStatus );

	ENSURES_B( sanityCheckBignum( r ) );

	return( TRUE );
	}

/* Mod square */

CHECK_RETVAL_BOOL STDC_NONNULL_ARG( ( 1, 2, 3, 4 ) ) \
BOOLEAN BN_mod_sqr( INOUT BIGNUM *r, const BIGNUM *a, const BIGNUM *m, 
					INOUT BN_CTX *ctx )
	{
	int bnStatus = BN_STATUS;

	assert( isWritePtr( r, sizeof( BIGNUM ) ) );
	assert( isReadPtr( a, sizeof( BIGNUM ) ) );
	assert( isReadPtr( m, sizeof( BIGNUM ) ) );
	assert( isWritePtr( ctx, sizeof( BN_CTX ) ) );

	REQUIRES_B( sanityCheckBignum( a ) && !BN_is_zero( a ) && \
				!BN_is_negative( a ) );
	REQUIRES_B( sanityCheckBignum( m ) && !BN_is_zero( m ) && \
				!BN_is_negative( m ) );
	REQUIRES_B( sanityCheckBNCTX( ctx ) );

	/* Since we know that r can't be negative after the squaring (which it 
	   couldn't be in any case since a is positive), we can just call 
	   BN_mod() directly */
    CK( BN_sqr( r, a, ctx ) );
	CK( BN_mod( r, r, m, ctx ) );
	if( bnStatusError( bnStatus ) )
		return( bnStatus );

	ENSURES_B( sanityCheckBignum( r ) );

	return( TRUE );
	}

/****************************************************************************
*																			*
*						Perform Montgomery Ops on Bignums					*
*																			*
****************************************************************************/

/* Montgomery modmult */

CHECK_RETVAL_BOOL STDC_NONNULL_ARG( ( 1, 2, 3, 4, 5 ) ) \
BOOLEAN BN_mod_mul_montgomery( INOUT BIGNUM *r, const BIGNUM *a, 
							   const BIGNUM *b, const BN_MONT_CTX *bnMontCTX, 
							   INOUT BN_CTX *bnCTX )
	{
	BIGNUM *tmp;
	int bnStatus = BN_STATUS;

	assert( isReadPtr( r, sizeof( BIGNUM ) ) );
	assert( isReadPtr( a, sizeof( BIGNUM ) ) );
	assert( isReadPtr( b, sizeof( BIGNUM ) ) );
	assert( isReadPtr( bnMontCTX, sizeof( BN_MONT_CTX ) ) );
	assert( isWritePtr( bnCTX, sizeof( BN_CTX ) ) );

	REQUIRES_B( sanityCheckBignum( a ) && !BN_is_zero( a ) && \
				!BN_is_negative( a ) );
	REQUIRES_B( sanityCheckBignum( b ) && !BN_is_zero( b ) && \
				!BN_is_negative( b ) );
	REQUIRES_B( sanityCheckBNMontCTX( bnMontCTX ) );
	REQUIRES_B( sanityCheckBNCTX( bnCTX ) );
	REQUIRES_B( BN_cmp( a, &bnMontCTX->N ) <= 0 );
	REQUIRES_B( BN_cmp( b, &bnMontCTX->N ) <= 0 );

	BN_CTX_start( bnCTX );

	/* Since we're dealing with oversize values that temporarily get very 
	   large, we have to use an extended bignum */
	tmp = BN_CTX_get_ext( bnCTX, BIGNUM_EXT_MONT );
	if( tmp == NULL )
		{
		BN_CTX_end( bnCTX );
		return( FALSE );
		}
	BN_set_flags( tmp, BN_FLG_SCRATCH );

	/* Perform the multiply and convert the result back from the Montgomery 
	   form */
	CK( BN_mul( tmp, a, b, bnCTX ) );
	CK( BN_from_montgomery( r, tmp, bnMontCTX, bnCTX ) )

	BN_CTX_end_ext( bnCTX, BIGNUM_EXT_MONT );
	if( bnStatusError( bnStatus ) )
		return( FALSE );

	ENSURES_B( sanityCheckBignum( r ) );

	return( TRUE );
	}

/****************************************************************************
*																			*
*								Compare Bignums								*
*																			*
****************************************************************************/

/* Compare two bignums */

STDC_NONNULL_ARG( ( 1 ) ) \
int BN_cmp_word( const BIGNUM *bignum, const BN_ULONG word )
	{
	assert( isReadPtr( bignum, sizeof( BIGNUM ) ) );

	/* If the bignum is negative then the (unsigned) word is always
	   larger.  This can occur via the usual mechanism of the 
	   BN_mod_inverse() used in Montgomery ops */
	if( BN_is_negative( bignum ) )
		return( -1 );

	/* If the bignum is longer than one word then the word being compared is 
	   smaller */
	if( bignum->top > 1 )
		return( 1 );

	/* If the bignum is zero then the word is either equal or larger */
	if( bignum->top <= 0 )
		return( ( word == 0 ) ? 0 : -1 );

	if( bignum->d[ 0 ] != word )
		return( ( bignum->d[ 0 ] > word ) ? 1 : -1 );
	
	/* They're identical */
	return( 0 );
	}

STDC_NONNULL_ARG( ( 1, 2 ) ) \
int BN_ucmp( const BIGNUM *bignum1, const BIGNUM *bignum2 )
	{
	const int bignum1top = bignum1->top;

	assert( isReadPtr( bignum1, sizeof( BIGNUM ) ) );
	assert( isReadPtr( bignum2, sizeof( BIGNUM ) ) );

	REQUIRES_B( bignum1top >= 0 && bignum1top < getBNMaxSize( bignum1 ) );
				/* There's not really any error return value that we can use 
				   here, the best that we can do is return zero on error */

	/* If we're comparing a bignum to itself (which can happen in functions
	   that take bignums as parameters in multiple locations) then we don't 
	   need to explicitly compare the contents */
	if( bignum1 == bignum2 )
		return( 0 );

	/* If the magnitude differs then we don't need to look at the values */
	if( bignum1top != bignum2->top )
		return( ( bignum1top > bignum2->top ) ? 1 : -1 );

	return( bn_cmp_words( bignum1->d, bignum2->d, bignum1top ) );
	}

/* Internal function that compares the words of two bignums */

STDC_NONNULL_ARG( ( 1, 2 ) ) \
int bn_cmp_words( const BN_ULONG *bignumData1, const BN_ULONG *bignumData2, 
				  IN_RANGE( 0, BIGNUM_ALLOC_WORDS ) const int length )
	{
	int iterationCount, i;

	assert( isReadPtr( bignumData1, length ) );
	assert( isReadPtr( bignumData2, length ) );

	REQUIRES_B( length >= 0 && length <= BIGNUM_ALLOC_WORDS );
				/* There's not really any error return value that we can use 
				   here, the best that we can do is return zero on error */

	/* Walk down the bignum until we find a difference */
	for( i = length - 1, iterationCount = 0; 
		 i >= 0 && iterationCount < BIGNUM_ALLOC_WORDS; 
		 i--, iterationCount++ )
		{
		if( bignumData1[ i ] != bignumData2[ i ] )
			return( ( bignumData1[ i ] > bignumData2[ i ] ) ? 1 : -1 );
		}
	ENSURES_B( iterationCount < BIGNUM_ALLOC_WORDS );

	/* They're identical */
	return( 0 );
	}

/* An oddball compare function used in BN_mul() via the bn_mul_recursive() 
   routine.  This compares two lots of bignum data of different lengths.
   Instead of doing the sensible thing and passing in { a, aLen } and
   { b, bLen }, we get passed { a, b, min( aLen, bLen ), aLen - bLen },
   where the last value can be negative if a < b, and have to figure things
   out from there */

STDC_NONNULL_ARG( ( 1, 2 ) ) \
int bn_cmp_part_words( const BN_ULONG *a, const BN_ULONG *b, 
					   IN_RANGE( 0, BIGNUM_ALLOC_WORDS_EXT ) const int cl, 
					   IN_RANGE( -BIGNUM_ALLOC_WORDS_EXT, \
								 BIGNUM_ALLOC_WORDS_EXT ) const int dl )
	{
	const BN_ULONG *data = ( dl < 0 ) ? b : a;
	const int max = ( dl < 0 ) ? -dl + cl : dl + cl;
	int iterationCount, i;

	REQUIRES_B( cl >= 0 && cl < BIGNUM_ALLOC_WORDS_EXT );
	REQUIRES_B( dl > -BIGNUM_ALLOC_WORDS_EXT && \
				dl < BIGNUM_ALLOC_WORDS_EXT );
	REQUIRES_B( max >= 0 && max < BIGNUM_ALLOC_WORDS_EXT );
				/* There's not really any error return value that we can use 
				   here, the best that we can do is return zero on error */

	/* Compare the overflow portions of length dl.  If any of the overflow
	   portion is nonzero then a or b is larger, depending on whether dl is
	   positive or negative */
	for( i = cl, iterationCount = 0; 
		 i < max && iterationCount < BIGNUM_ALLOC_WORDS_EXT; 
		 i++, iterationCount++ )
		{
		if( data[ i ] != 0 )
			return( ( dl < 0 ) ? -1 : 1 );
		}
	ENSURES_B( iterationCount < BIGNUM_ALLOC_WORDS_EXT );

	/* Compare the common-length portions */
	return( bn_cmp_words( a, b, cl ) );
	}
#endif /* USE_PKC */
