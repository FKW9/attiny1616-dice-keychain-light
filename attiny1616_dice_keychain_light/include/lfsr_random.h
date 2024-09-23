/**
 * Copyright (c) 2017, Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 * ATtiny13/25/45/85 16-bit PRNG (Pseudo Random Number Generator) library based on Galois LFSR algorithm.
 */

#ifndef	_LFSR_RANDOM_H_
#define	_LFSR_RANDOM_H_

#define USE_RANDOM_EEPROM_SEED
#ifdef	USE_RANDOM_EEPROM_SEED
# define	RANDOM_EEPROM_SEED_ADDR	(0x00)
#endif	/* !USE_RANDOM_EEPROM_SEED */

void lfsr_seed(uint16_t seed);
uint16_t lfsr_random(void);

#endif	/* !_LFSR_RANDOM_H_ */
