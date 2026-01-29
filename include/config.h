/**
 * @file config.h
 * @brief Configuration and precision settings for iirdsp library
 */

#ifndef IIRDSP_CONFIG_H
#define IIRDSP_CONFIG_H

#include <stdint.h>
#include <string.h>

/**
 * Precision configuration
 * Define IIRDSP_USE_FLOAT for embedded systems (float)
 * Undefine for desktop systems (double)
 */
/* #define IIRDSP_USE_FLOAT */

#ifdef IIRDSP_USE_FLOAT
typedef float iirdsp_real;
#else
typedef double iirdsp_real;
#endif

/**
 * Maximum number of second-order sections (biquads) in a filter cascade
 * Constraint: order <= IIRDSP_MAX_SECTIONS * 2 for band-pass filters
 */
#define IIRDSP_MAX_SECTIONS 8

#endif /* IIRDSP_CONFIG_H */
