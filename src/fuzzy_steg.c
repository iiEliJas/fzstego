#include "fuzzy_steg.h"
#include "fuzzy.h"

#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdio.h>

#define WINDOW_SIZE 7
#define BITS_PER_BYTE 8


void strip_lower_bits(const unsigned char *pixel_data, unsigned char *stripped, size_t size) {
    for (size_t i = 0; i < size; i++){
        stripped[i] = pixel_data[i] & 0xF8;
    }
}



void compute_grayscale(const unsigned char *pixel_data, unsigned char *grayscale, int width, int height) {
    for (int y = 0; y < height; y++){
        for (int x = 0; x < width; x++){
            int idx = (y * width + x) * 3;
            
            int r = pixel_data[idx];
            int g = pixel_data[idx + 1];
            int b = pixel_data[idx + 2];
            
            float gray = 0.299f * r + 0.587f * g + 0.114f * b;
            grayscale[y * width + x] = (unsigned char)gray;
        }
    }
}



float extract_entropy(const unsigned char *image_strip, int width, int height, int x, int y, int window_size) {
    int half = window_size / 2;
    int x_start = (x - half < 0) ? 0 : x - half;
    int x_end = (x + half >= width) ? width - 1 : x + half;
    int y_start = (y - half < 0) ? 0 : y - half;
    int y_end = (y + half >= height) ? height - 1 : y + half;
    
    int histogram[256] = {0};
    int pixel_count = 0;
    
    for (int py = y_start; py <= y_end; py++){
        for (int px = x_start; px <= x_end; px++){
            histogram[image_strip[py * width + px]]++;
            pixel_count++;
        }
    }
    
    // Shannon entropy
    float entropy = 0.0f;
    for (int i = 0; i < 256; i++) {
        if (histogram[i] > 0) {
            float p = (float)histogram[i] / pixel_count;
            entropy -= p * logf(p) / logf(2.0f);
        }
    }
    
    return entropy;
}



// Sobel operator for edge detection
static void sobel_convolve(const unsigned char *image, int width, int height, int x, int y, float *gx, float *gy) {
    if (x < 1 || x >= width - 1 || y < 1 || y >= height - 1){
        *gx = 0.0f;
        *gy = 0.0f;
        return;
    }
    
    // Sobel X
    float sx = -image[(y-1)*width + (x-1)] + image[(y-1)*width + (x+1)]
             - 2*image[y*width + (x-1)] + 2*image[y*width + (x+1)]
             - image[(y+1)*width + (x-1)] + image[(y+1)*width + (x+1)];
    
    // Sobel Y
    float sy = -image[(y-1)*width + (x-1)] - 2*image[(y-1)*width + x] - image[(y-1)*width + (x+1)]
             + image[(y+1)*width + (x-1)] + 2*image[(y+1)*width + x] + image[(y+1)*width + (x+1)];
    
    *gx = sx;
    *gy = sy;
}



float extract_edge(const unsigned char *image_strip, int width, int height, int x, int y) {
    float gx, gy;
    sobel_convolve(image_strip, width, height, x, y, &gx, &gy);
    
    float magnitude = sqrtf(gx*gx + gy*gy);
    return magnitude;
}



// Normalize edge to [0, 1]
static void normalize_edges(float *edges, int width, int height) {
    float max_edge = 0.0f;
    for (int i = 0; i < width * height; i++){
        if (edges[i] > max_edge) max_edge = edges[i];
    }
    
    if (max_edge < 1e-6f) return;
    
    for (int i = 0; i < width * height; i++){
        edges[i] /= max_edge;
    }
}


/////////////////////////////////////////////////////////////////////////////////////
//              MAP FUNCTIONS

feature_map* build_entropy_map(const unsigned char *grayscale_strip, int width, int height, int window_size) {
    feature_map *map = feature_map_create(width, height);
    if (!map) return NULL;
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            map->entropy[y * width + x] = extract_entropy(grayscale_strip, width, height, x, y, window_size);
        }
    }
    
    return map;
}



feature_map* build_edge_map(const unsigned char *grayscale_strip, int width, int height) {
    feature_map *map = feature_map_create(width, height);
    if (!map) return NULL;
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            map->edge[y * width + x] = extract_edge(grayscale_strip, width, height, x, y);
        }
    }
    
    normalize_edges(map->edge, width, height);
    return map;
}



feature_map* build_pressure_map(const feature_map *depth_map, size_t data_len, int channels) {
    if (!depth_map) return NULL;
    
    feature_map *map = feature_map_create(depth_map->width, depth_map->height);
    if (!map) return NULL;
    
    // total capacity
    size_t total_capacity = 0;
    for (size_t i = 0; i < depth_map->width * depth_map->height; i++) {
        total_capacity += (int)depth_map->depth[i] * channels;
    }
    
    size_t required_bits = (sizeof(size_t) + data_len) * BITS_PER_BYTE;
    
    // global pressure
    float global_pressure = (total_capacity > 0) ? (float)required_bits / total_capacity    : 0.0f;
    
    // clamp to [0, 1]
    if (global_pressure < 0.0f) global_pressure = 0.0f;
    if (global_pressure > 1.0f) global_pressure = 1.0f;
    
    for (size_t i = 0; i < depth_map->width * depth_map->height; i++) {
        map->pressure[i] = global_pressure;
    }
    
    return map;
}



feature_map* build_depth_map(fuzzy_engine *engine, const feature_map *entropy_map, const feature_map *edge_map, const feature_map *pressure_map) {
    if (!engine || !entropy_map || !edge_map || !pressure_map) return NULL;
    
    size_t width = entropy_map->width;
    size_t height = entropy_map->height;
    
    feature_map *map = feature_map_create(width, height);
    if (!map) return NULL;
    
    for (size_t i = 0; i < width * height; i++) {
        float raw_depth = fuzzy_infer(engine, entropy_map->entropy[i], edge_map->edge[i], pressure_map->pressure[i]);
        
        // depth can only be 1,2 or 3
        map->depth[i] = (float)((int)(raw_depth + 0.5f));
        if (map->depth[i] < 1.0f) map->depth[i] = 1.0f;
        if (map->depth[i] > 3.0f) map->depth[i] = 3.0f;
    }
    
    return map;
}
