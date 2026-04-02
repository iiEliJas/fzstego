#ifndef FUZZY_H
#define FUZZY_H

#include <stddef.h>
#include <stdint.h>


typedef struct {
    float a, b, c, d;
} trapez;


typedef struct {
    trapez low;
    trapez medium;
    trapez high;
} fuzzy_input;


typedef struct {
    trapez shallow;
    trapez moderate;
    trapez deep;
} fuzzy_output;


typedef struct {
    fuzzy_input entropy_sets;
    fuzzy_input edge_sets;
    fuzzy_input pressure_sets;
    fuzzy_output depth_sets;
} fuzzy_engine;


typedef struct {
    float *entropy;
    float *edge;
    float *pressure;
    float *depth;
    size_t width;
    size_t height;
} feature_map;


fuzzy_engine* fuzzy_create(void);

void fuzzy_destroy(fuzzy_engine *engine);

float fuzzy_trapezoid(float x, const trapez *trap);

float fuzzy_infer(fuzzy_engine *engine, float entropy, float edge, float pressure);

feature_map* feature_map_create(size_t width, size_t height);

void feature_map_destroy(feature_map *map);

#endif
