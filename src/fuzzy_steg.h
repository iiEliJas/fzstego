#ifndef FUZZY_STEG_H
#define FUZZY_STEG_H

#include "fuzzy.h"

#include <stddef.h>
#include <stdint.h>



float extract_entropy(const unsigned char *image_strip, int width, int height, int x, int y, int window_size);

                    
float extract_edge(const unsigned char *image_strip, int width, int height, int x, int y);


void strip_lower_bits(const unsigned char *pixel_data, unsigned char *stripped, size_t size);


void compute_grayscale(const unsigned char *pixel_data, unsigned char *grayscale, int width, int height);


feature_map* build_entropy_map(const unsigned char *grayscale_strip, int width, int height, int window_size);


feature_map* build_edge_map(const unsigned char *grayscale_strip, int width, int height);


feature_map* build_pressure_map(const feature_map *depth_map, size_t data_len, int channels);


feature_map* build_depth_map(fuzzy_engine *engine, const feature_map *entropy_map, const feature_map *edge_map, const feature_map *pressure_map);

#endif
