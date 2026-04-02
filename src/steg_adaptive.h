#ifndef STEG_ADAPTIVE_H
#define STEG_ADAPTIVE_H

#include <stddef.h>
#include <stdint.h>

// Embed data into BMP using adaptive fuzzy logic
// Returns 0 on success, -1 on failure
int embed_data_adaptive(const char *input_image_path, const char *output_image_path, const unsigned char *data, size_t data_len, unsigned int seed);

// Extract data from BMP using adaptive fuzzy logic
// Returns pointer to extracted data on success, NULL on failure
unsigned char* extract_data_adaptive(const char *image_path, size_t *data_len, unsigned int seed);

#endif
