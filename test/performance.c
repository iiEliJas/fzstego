// Performance Testing for Adaptive Fuzzy Logic Steganography
//

#include "../src/fzstego.h"
#include "../src/fuzzy.h"
#include "../src/fuzzy_steg.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>


//////////////////////////////////////////////////////////////////////
// Timing

typedef struct {
    clock_t start;
    clock_t end;
    double elapsed_ms;
} timer_t;

static timer_t timer_create() {
    timer_t t;
    t.start = 0;
    t.end = 0;
    t.elapsed_ms = 0.0;
    return t;
}

static void timer_start(timer_t *t) {
    t->start = clock();
}

static void timer_stop(timer_t *t) {
    t->end = clock();
    t->elapsed_ms = ((double)(t->end - t->start) / CLOCKS_PER_SEC) * 1000.0;
}

static double timer_elapsed_ms(timer_t *t) {
    return t->elapsed_ms;
}


//////////////////////////////////////////////////////////////////////
// PERFORMANCE TESTS

void benchmark_fuzzy_inference() {
    printf("\n\n-------------------------------------\n");
    printf("\nBenchmark fuzzy inference\n");
    printf("-------------------------------------\n");
    
    fuzzy_engine *engine = fuzzy_create();
    timer_t timer = timer_create();
    
    const int iterations = 10000;
    
    timer_start(&timer);

    for (int i = 0; i < iterations; i++) {
        float entropy = 2.5f + (float)(rand() % 30) / 10.0f;
        float edge = (float)(rand() % 100) / 100.0f;
        float pressure = (float)(rand() % 100) / 100.0f;
        fuzzy_infer(engine, entropy, edge, pressure);
    }
    timer_stop(&timer);
    
    double time_per_inference = timer_elapsed_ms(&timer) / iterations;
    
    printf("Total time: %.2f ms\n", timer_elapsed_ms(&timer));
    printf("Iterations: %d\n", iterations);
    printf("Time per inference: %.4f ms\n", time_per_inference);
    
    fuzzy_destroy(engine);
}



void benchmark_entropy_extraction() {
    printf("\n\n-------------------------------------\n");
    printf("\nBenchmark entropy extraction\n");
    printf("-------------------------------------\n");
    
    unsigned char image[256 * 256];
    for (int i = 0; i < 256 * 256; i++) {
        image[i] = (unsigned char)(rand() % 256);
    }
    
    timer_t timer = timer_create();
    
    // entropy for every pixel
    timer_start(&timer);
    for (int y = 1; y < 255; y++) {
        for (int x = 1; x < 255; x++) {
            extract_entropy(image, 256, 256, x, y, 7);
        }
    }
    timer_stop(&timer);
    
    int pixels_processed = (256 - 2) * (256 - 2);
    double time_per_pixel = timer_elapsed_ms(&timer) / pixels_processed;
    
    printf("Total time: %.2f ms\n", timer_elapsed_ms(&timer));
    printf("Pixels processed: %d\n", pixels_processed);
    printf("Time per pixel: %.4f ms\n", time_per_pixel);
}



void benchmark_edge_detection() {
    printf("\n\n-------------------------------------\n");
    printf("\nBenchmark edge detection\n");
    printf("-------------------------------------\n");
    
    unsigned char image[256 * 256];
    for (int i = 0; i < 256 * 256; i++) {
        image[i] = (unsigned char)(rand() % 256);
    }
    
    timer_t timer = timer_create();
    
    // edges for every pixel
    timer_start(&timer);
    for (int y = 1; y < 255; y++) {
        for (int x = 1; x < 255; x++) {
            extract_edge(image, 256, 256, x, y);
        }
    }
    timer_stop(&timer);
    
    int pixels_processed = (256 - 2) * (256 - 2);
    double time_per_pixel = timer_elapsed_ms(&timer) / pixels_processed;
    
    printf("Total time: %.2f ms\n", timer_elapsed_ms(&timer));
    printf("Pixels processed: %d\n", pixels_processed);
    printf("Time per pixel: %.4f ms\n", time_per_pixel);
}



void benchmark_bit_stripping() {
    printf("\n\n-------------------------------------\n");
    printf("\n Benchmark bit stripping\n");
    printf("-------------------------------------\n");
    
    unsigned char original[256 * 256 * 3];
    unsigned char stripped[256 * 256 * 3];
    
    for (int i = 0; i < 256 * 256 * 3; i++) {
        original[i] = (unsigned char)(rand() % 256);
    }
    
    timer_t timer = timer_create();
    
    timer_start(&timer);
    strip_lower_bits(original, stripped, 256 * 256 * 3);
    timer_stop(&timer);
    
    double time_per_byte = timer_elapsed_ms(&timer) / (256 * 256 * 3);
    
    printf("Total time: %.4f ms\n", timer_elapsed_ms(&timer));
    printf("Bytes processed: %d\n", 256 * 256 * 3);
    printf("Time per byte: %.6f ms\n", time_per_byte);
}



void benchmark_embedding() {
    printf("\n\n-------------------------------------\n");
    printf("\nBenchmark data embedding\n");
    printf("-------------------------------------\n");
    
    FILE *f = fopen("test_cover.bmp", "rb");
    if (!f) {
        printf("test_cover.bmp not found\n");
        return;
    }
    fclose(f);
    
    unsigned char secret[1024];
    for (int i = 0; i < 1024; i++) {
        secret[i] = (unsigned char)(rand() % 256);
    }
    
    timer_t timer = timer_create();
    
    timer_start(&timer);
    int result = embed_data_adaptive("test_cover.bmp", "benchmark_stego.bmp", secret, 1024, 42);
    timer_stop(&timer);
    
    if (result == 0) {
        printf("Embedding success\n");
        printf("Data embedded: 1024 bytes\n");
        printf("Total time: %.2f ms\n", timer_elapsed_ms(&timer));
    } else {
        printf("Embedding failed\n");
    }
}



void benchmark_extraction() {
    printf("\n\n-------------------------------------\n");
    printf("\nBenchmark data extraction\n");
    printf("-------------------------------------\n");
    
    FILE *f = fopen("benchmark_stego.bmp", "rb");
    if (!f) {
        printf("benchmark_stego.bmp not found\n");
        return;
    }
    fclose(f);
    
    timer_t timer = timer_create();
    
    size_t recovered_len;
    timer_start(&timer);
    unsigned char *recovered = extract_data_adaptive("benchmark_stego.bmp", &recovered_len, 42);
    timer_stop(&timer);
    
    if (recovered != NULL) {
        printf("Extraction success\n");
        printf("Data recovered: %zu bytes\n", recovered_len);
        printf("Total time: %.2f ms\n", timer_elapsed_ms(&timer));
        free(recovered);
    } else {
        printf("Extraction failed\n");
    }
}



void benchmark_feature_map_creation() {
    printf("\n\n-------------------------------------\n");
    printf("\nBenchmark feature map creation\n");
    printf("-------------------------------------\n");
    
    FILE *f = fopen("test_cover.bmp", "rb");
    if (!f) {
        printf("test_cover.bmp not found\n");
        return;
    }
    fclose(f);
    
    unsigned char image[256 * 256];
    for (int i = 0; i < 256 * 256; i++) {
        image[i] = (unsigned char)(rand() % 256);
    }
    
    timer_t timer = timer_create();
    fuzzy_engine *engine = fuzzy_create();
    
    timer_start(&timer);
    
    // entropy
    feature_map *entropy_map = feature_map_create(256, 256);
    for (int y = 0; y < 256; y++) {
        for (int x = 0; x < 256; x++) {
            entropy_map->entropy[y * 256 + x] = extract_entropy(image, 256, 256, x, y, 7);
        }
    }
    
    // edges
    feature_map *edge_map = feature_map_create(256, 256);
    for (int y = 0; y < 256; y++) {
        for (int x = 0; x < 256; x++) {
            edge_map->edge[y * 256 + x] = extract_edge(image, 256, 256, x, y);
        }
    }
    
    // depth
    feature_map *depth_map = feature_map_create(256, 256);
    for (int i = 0; i < 256 * 256; i++) {
        depth_map->depth[i] = fuzzy_infer(engine, entropy_map->entropy[i], edge_map->edge[i], 0.5f);
    }
    
    timer_stop(&timer);
    
    printf("Feature extraction:\n");
    printf("Total time: %.2f ms\n", timer_elapsed_ms(&timer));
    
    feature_map_destroy(entropy_map);
    feature_map_destroy(edge_map);
    feature_map_destroy(depth_map);
    fuzzy_destroy(engine);
}


//////////////////////////////////////////////////////////////////////
// MAIN

int main() {
    printf("\n");
    printf("--- Performance Tests ---\n");
    printf("\n");

    srand((unsigned int)time(NULL));
    
    benchmark_fuzzy_inference();
    benchmark_entropy_extraction();
    benchmark_edge_detection();
    benchmark_bit_stripping();
    
    benchmark_embedding();
    benchmark_extraction();
    benchmark_feature_map_creation();
    
    return 0;
}