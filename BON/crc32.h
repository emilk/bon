//
//  crc32.h
//  BON
//
//  Created by emilk on 2013-04-07.
//  Copyright (c) 2013 Emil Ernerfeldt. All rights reserved.
//

#ifndef BON_crc32_h
#define BON_crc32_h

#include <stdint.h>

/*
 The normal CRC32 found in GZIP, PNG etc.
 http://www.faqs.org/rfcs/rfc1952.html
 http://www.w3.org/TR/PNG/#D-CRCAppendix
 */

/*
 Usage:
 uint8_t data[] = { .... };
 
 uint32_t crc_inv = 0xffffffff; // initial value
 crc_inv = crc_update(crc_inv, buff1, sizeof(buff1));
 crc_inv = crc_update(crc_inv, buff2, sizeof(buff2));
 uint32_t crc = crc_inv ^ 0xffffffff; // Final invert 
 */
uint32_t crc_update(uint32_t crc_inv, const uint8_t* data, uint64_t size);

// Calcualte the crc of the given bytes.
uint32_t crc_calc(const uint8_t* data, uint64_t size);

#endif
