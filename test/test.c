// Unit Testing for Adaptive Fuzzy Logic Steganography
//

#include "../src/steg_adaptive.h"
#include "../src/fuzzy.h"
#include "../src/fuzzy_steg.h"
#include "test.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

//////////////////////////////////////////////////////////////////////
// FUZZY LOGIC

void test_trapez() {
    fuzzy_engine *engine = fuzzy_create();
    trapez trap = {0.0f, 1.0f, 2.0f, 3.0f};
    
    // Test below
    float val = fuzzy_trapezoid(-1.0f, &trap);
    if (fabs(val - 0.0f) < 0.001f) {
        pass("trapez bottom");
    } else {
        fail("trapez bottom", "expected 0.0");
    }
    
    // Test rising edge
    val = fuzzy_trapezoid(0.5f, &trap);
    if (fabs(val - 0.5f) < 0.001f) {
        pass("trapez rising edge");
    } else {
        fail("trapez rising edge", "expected 0.5");
    }
    
    // Test top
    val = fuzzy_trapezoid(1.5f, &trap);
    if (fabs(val - 1.0f) < 0.001f) {
        pass("trapez top");
    } else {
        fail("trapez top", "expected 1.0");
    }
    
    // Test falling edge
    val = fuzzy_trapezoid(2.5f, &trap);
    if (fabs(val - 0.5f) < 0.001f) {
        pass("trapez falling edge");
    } else {
        fail("trapez falling edge", "expected 0.5");
    }
    
    // Test above
    val = fuzzy_trapezoid(4.0f, &trap);
    if (fabs(val - 0.0f) < 0.001f) {
        pass("trapez above");
    } else {
        fail("trapez above", "expected 0.0");
    }
    
    fuzzy_destroy(engine);
}



void test_fuzzy_inference_complex_region() {
    fuzzy_engine *engine = fuzzy_create();
    
    // high entropy, high edge, high pressure
    float depth = fuzzy_infer(engine, 5.5f, 0.9f, 0.8f);
    
    if (depth >= 2.0f && depth <= 3.0f) {
        pass(__func__);
    } else {
        fail(__func__, "depth out of expected range");
    }
    
    fuzzy_destroy(engine);
}



void test_fuzzy_input_clamping() {
    fuzzy_engine *engine = fuzzy_create();
    
    // entropy should be clamped to 6
    float depth1 = fuzzy_infer(engine, 100.0f, 0.5f, 0.5f);
    
    // edge should be clamped to 1
    float depth2 = fuzzy_infer(engine, 3.0f, 100.0f, 0.5f);
    
    // pressure should be clamped to 1
    float depth3 = fuzzy_infer(engine, 3.0f, 0.5f, 100.0f);
    
    // All should produce valid outputs
    if (depth1 >= 1.0f && depth1 <= 3.0f &&
        depth2 >= 1.0f && depth2 <= 3.0f &&
        depth3 >= 1.0f && depth3 <= 3.0f) {
        pass(__func__);
    } else {
        fail(__func__, "clamping failed");
    }
    
    fuzzy_destroy(engine);
}


//////////////////////////////////////////////////////////////////////
// FEATURE EXTRACTION

void test_bit_stripping() {
    unsigned char original[] = {0xFF, 0x4D, 0xAB, 0x12};
    unsigned char stripped[4];
    
    strip_lower_bits(original, stripped, 4);
    
    // lower 3 bits are cleared
    if (stripped[0] == 0xF8 &&
        stripped[1] == 0x48 &&
        stripped[2] == 0xA8 &&
        stripped[3] == 0x10) {
        pass(__func__);
    } else {
        fail(__func__, "stripping failed");
    }
}



void test_feature_map_creation() {
    feature_map *map = feature_map_create(256, 256);
    
    if (map == NULL) {
        fail(__func__, "map is null");
        return;
    }
    
    if (map->width == 256 && map->height == 256) {
        pass(__func__);
    } else {
        fail(__func__, "incorrect dimensions");
    }
    
    if (map->entropy != NULL && map->edge != NULL &&
        map->pressure != NULL && map->depth != NULL) {
        pass(__func__);
    } else {
        fail(__func__, "null pointer");
    }
    
    feature_map_destroy(map);
}



void test_entropy_range() {
    // low entropy
    unsigned char uniform[49];
    memset(uniform, 100, 49);
    
    float entropy_low = extract_entropy(uniform, 7, 7, 3, 3, 7);
    
    if (entropy_low >= 0.0f && entropy_low <= 0.5f) {
        pass(__func__);
    } else {
        fail(__func__, "entropy out of range");
    }

    // high entropy
    unsigned char varied[49];
    for (int i = 0; i < 49; i++) {
        varied[i] = (unsigned char)((i * 17) % 256);
    }
    
    float entropy_high = extract_entropy(varied, 7, 7, 3, 3, 7);
    
    if (entropy_high > entropy_low) {
        pass(__func__);
    } else {
        fail(__func__, "varied entropy not higher");
    }
}



void test_edge_detection() {
    // uniform gradient
    unsigned char edges[49];
    for (int i = 0; i < 49; i++) {
        edges[i] = (unsigned char)((i / 7) * 30);
    }
    
    float edge_val = extract_edge(edges, 7, 7, 3, 3);
    
    if (edge_val >= 0.0f) {
        pass(__func__);
    } else {
        fail(__func__, "edge value negative");
    }
}


//////////////////////////////////////////////////////////////////////
// EMBEDDING & EXTRACTION

void test_embedding() {
    // Check if test image exists
    FILE *f = fopen("test_cover.bmp", "rb");
    if (!f) {
        fail(__func__, "test_cover.bmp not found");
        return;
    }
    fclose(f);
    
    unsigned char secret[] = "Test";
    unsigned int seed = 42;
    
    int result = embed_data_adaptive("test_cover.bmp", "test_stego.bmp", secret, 4, seed);
    
    if (result == 0) {
        pass(__func__);
    } else {
        fail(__func__, "embedding failed");
    }
}



void test_extraction() {
    FILE *f = fopen("test_stego.bmp", "rb");
    if (!f) {
        fail(__func__, "test_stego.bmp not found");
        return;
    }
    fclose(f);
    
    size_t recovered_len;
    unsigned char *recovered = extract_data_adaptive("test_stego.bmp", &recovered_len, 42);
    
    if (recovered != NULL) {
        pass(__func__);
        free(recovered);
    } else {
        fail(__func__, "extraction failed");
    }
}



void test_embedding_extraction_roundtrip() {
    FILE *f = fopen("test_cover.bmp", "rb");
    if (!f) {
        fail(__func__, "test_cover.bmp not found");
        return;
    }
    fclose(f);
    
    unsigned char secret[] = "Hello there";
    unsigned int seed = 123;
    
    // Embed
    int embed_result = embed_data_adaptive("test_cover.bmp", "test_stego.bmp", secret, 5, seed);
    
    if (embed_result != 0) {
        fail(__func__, "embedding failed");
        return;
    }
    
    // Extract
    size_t recovered_len;
    unsigned char *recovered = extract_data_adaptive("test_stego.bmp", &recovered_len, seed);
    
    if (recovered == NULL) {
        fail(__func__, "extraction failed");
        return;
    }
    
    if (recovered_len == 5 && memcmp(recovered, secret, 5) == 0) {
        pass(__func__);
    } else {
        fail(__func__, "extracted data mismatch");
    }
    
    free(recovered);
}



void test_seed_dependency() {
    FILE *f = fopen("test_cover.bmp", "rb");
    if (!f) {
        fail(__func__, "test_cover.bmp not found");
        return;
    }
    fclose(f);
    
    unsigned char secret[] = "Secrettt";
    
    // Embed with seed 67
    int embed_result = embed_data_adaptive("test_cover.bmp", "test_stego_seed1.bmp", secret, 6, 67);
    
    if (embed_result != 0) {
        fail(__func__, "embedding failed");
        return;
    }
    
    // Extract with wrong seed
    size_t len_wrong;
    printf("  (Note: Should produce Error)\n");
    unsigned char *recovered = extract_data_adaptive("test_stego_seed1.bmp", &len_wrong, 99);
    
    if (recovered == NULL) {
        pass(__func__);
    } else if (len_wrong != 6 || memcmp(recovered, secret, 6) != 0) {
        pass(__func__);
        free(recovered);
    } else {
        fail(__func__, "wrong seed produced correct data");
        free(recovered);
    }
}


//////////////////////////////////////////////////////////////////////
// MAIN

int main() {
    printf("\n");
    printf("--- Unit Tests ---\n");
    printf("\n");
    
    test_trapez();
    test_fuzzy_inference_complex_region();
    test_fuzzy_input_clamping();
    printf("\n");
    
    test_bit_stripping();
    test_feature_map_creation();
    test_entropy_range();
    test_edge_detection();
    printf("\n");
    
    test_embedding();
    test_extraction();
    test_embedding_extraction_roundtrip();
    test_seed_dependency();
    printf("\n");
    
    printf("\nResult: %d / %d passed\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}