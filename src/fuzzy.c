#include "fuzzy.h"

#include <stdlib.h>
#include <math.h>
#include <string.h>



// entropy normalized to [0, 6]
// edge magnitude normalized to [0, 1]
// pressure normalized to [0, 1]
//
// output depth normalized to [1, 3]
//
fuzzy_engine* fuzzy_create(void) {
    fuzzy_engine *engine = (fuzzy_engine *)malloc(sizeof(fuzzy_engine));
    if (!engine) return NULL;

    // Low entropy: [0, 0, 1.5, 3]
    engine->entropy_sets.low = (trapez){0.0f, 0.0f, 1.5f, 3.0f};
    // Medium entropy: [1.5, 2.5, 3.5, 4.5]
    engine->entropy_sets.medium = (trapez){1.5f, 2.5f, 3.5f, 4.5f};
    // High entropy: [3, 4.5, 6, 6]
    engine->entropy_sets.high = (trapez){3.0f, 4.5f, 6.0f, 6.0f};

    // Low edge: [0, 0, 0.2, 0.35]
    engine->edge_sets.low = (trapez){0.0f, 0.0f, 0.2f, 0.35f};
    // Medium edge: [0.2, 0.35, 0.65, 0.8]
    engine->edge_sets.medium = (trapez){0.2f, 0.35f, 0.65f, 0.8f};
    // High edge: [0.65, 0.8, 1.0, 1.0]
    engine->edge_sets.high = (trapez){0.65f, 0.8f, 1.0f, 1.0f};

    // Low pressure: [0, 0, 0.25, 0.4]
    engine->pressure_sets.low = (trapez){0.0f, 0.0f, 0.25f, 0.4f};
    // Medium pressure: [0.25, 0.4, 0.6, 0.75]
    engine->pressure_sets.medium = (trapez){0.25f, 0.4f, 0.6f, 0.75f};
    // High pressure: [0.6, 0.75, 1.0, 1.0]
    engine->pressure_sets.high = (trapez){0.6f, 0.75f, 1.0f, 1.0f};


    // Shallow: [1, 1, 1.4, 1.8]
    engine->depth_sets.shallow = (trapez){1.0f, 1.0f, 1.4f, 1.8f};
    // Moderate: [1.4, 1.8, 2.2, 2.6]
    engine->depth_sets.moderate = (trapez){1.4f, 1.8f, 2.2f, 2.6f};
    // Deep: [2.2, 2.6, 3.0, 3.0]
    engine->depth_sets.deep = (trapez){2.2f, 2.6f, 3.0f, 3.0f};

    return engine;
}



void fuzzy_destroy(fuzzy_engine *engine) {
    if (engine) free(engine);
}



float fuzzy_trapezoid(float x, const trapez *trap) {
    if (x <= trap->a || x >= trap->d) return 0.0f;
    if (x >= trap->b && x <= trap->c) return 1.0f;
    
    if (x < trap->b && trap->b > trap->a){
        return (x - trap->a) / (trap->b - trap->a);
    }
    if (x > trap->c && trap->d > trap->c){
        return (trap->d - x) / (trap->d - trap->c);
    }
    return 0.0f;
}



// Mamdani min max inference with centroid defuzzification
static float mamdani_infer(fuzzy_engine *engine,
                           float ent_low, float ent_med, float ent_high,
                           float edg_low, float edg_med, float edg_high,
                           float prs_low, float prs_med, float prs_high) {
    
    // output
    float out_shallow = 0.0f, out_moderate = 0.0f, out_deep = 0.0f;
    

    // Rule 1: Low, Low, Low -> Shallow
    float r1 = fminf(fminf(ent_low, edg_low), prs_low);
    out_shallow = fmaxf(out_shallow, r1);
    
    // Rule 2: Low, Low, Medium -> Shallow
    float r2 = fminf(fminf(ent_low, edg_low), prs_med);
    out_shallow = fmaxf(out_shallow, r2);
    
    // Rule 3: Low, Low, High -> Moderate
    float r3 = fminf(fminf(ent_low, edg_low), prs_high);
    out_moderate = fmaxf(out_moderate, r3);
    
    // Rule 4: Low, Medium, Low -> Shallow
    float r4 = fminf(fminf(ent_low, edg_med), prs_low);
    out_shallow = fmaxf(out_shallow, r4);
    
    // Rule 5: Low, Medium, Medium -> Moderate
    float r5 = fminf(fminf(ent_low, edg_med), prs_med);
    out_moderate = fmaxf(out_moderate, r5);
    
    // Rule 6: Low, Medium, High -> Moderate
    float r6 = fminf(fminf(ent_low, edg_med), prs_high);
    out_moderate = fmaxf(out_moderate, r6);
    
    // Rule 7: Low, High, Low -> Moderate
    float r7 = fminf(fminf(ent_low, edg_high), prs_low);
    out_moderate = fmaxf(out_moderate, r7);
    
    // Rule 8: Low, High, Medium -> Moderate
    float r8 = fminf(fminf(ent_low, edg_high), prs_med);
    out_moderate = fmaxf(out_moderate, r8);
    
    // Rule 9: Low, High, High -> Deep
    float r9 = fminf(fminf(ent_low, edg_high), prs_high);
    out_deep = fmaxf(out_deep, r9);
    
    // Rule 10: Medium, Low, Low -> Shallow
    float r10 = fminf(fminf(ent_med, edg_low), prs_low);
    out_shallow = fmaxf(out_shallow, r10);
    
    // Rule 11: Medium, Low, Medium -> Moderate
    float r11 = fminf(fminf(ent_med, edg_low), prs_med);
    out_moderate = fmaxf(out_moderate, r11);
    
    // Rule 12: Medium, Low, High -> Moderate
    float r12 = fminf(fminf(ent_med, edg_low), prs_high);
    out_moderate = fmaxf(out_moderate, r12);
    
    // Rule 13: Medium, Medium, Low -> Moderate
    float r13 = fminf(fminf(ent_med, edg_med), prs_low);
    out_moderate = fmaxf(out_moderate, r13);
    
    // Rule 14: Medium, Medium, Medium -> Moderate
    float r14 = fminf(fminf(ent_med, edg_med), prs_med);
    out_moderate = fmaxf(out_moderate, r14);
    
    // Rule 15: Medium, Medium, High -> Deep
    float r15 = fminf(fminf(ent_med, edg_med), prs_high);
    out_deep = fmaxf(out_deep, r15);
    
    // Rule 16: Medium, High, Low -> Moderate
    float r16 = fminf(fminf(ent_med, edg_high), prs_low);
    out_moderate = fmaxf(out_moderate, r16);
    
    // Rule 17: Medium, High, Medium -> Deep
    float r17 = fminf(fminf(ent_med, edg_high), prs_med);
    out_deep = fmaxf(out_deep, r17);
    
    // Rule 18: Medium, High, High -> Deep
    float r18 = fminf(fminf(ent_med, edg_high), prs_high);
    out_deep = fmaxf(out_deep, r18);
    
    // Rule 19: High, Low, Low -> Moderate
    float r19 = fminf(fminf(ent_high, edg_low), prs_low);
    out_moderate = fmaxf(out_moderate, r19);
    
    // Rule 20: High, Low, Medium -> Moderate
    float r20 = fminf(fminf(ent_high, edg_low), prs_med);
    out_moderate = fmaxf(out_moderate, r20);
    
    // Rule 21: High, Low, High -> Deep
    float r21 = fminf(fminf(ent_high, edg_low), prs_high);
    out_deep = fmaxf(out_deep, r21);
    
    // Rule 22: High, Medium, Low -> Moderate
    float r22 = fminf(fminf(ent_high, edg_med), prs_low);
    out_moderate = fmaxf(out_moderate, r22);
    
    // Rule 23: High, Medium, Medium -> Deep
    float r23 = fminf(fminf(ent_high, edg_med), prs_med);
    out_deep = fmaxf(out_deep, r23);
    
    // Rule 24: High, Medium, High -> Deep
    float r24 = fminf(fminf(ent_high, edg_med), prs_high);
    out_deep = fmaxf(out_deep, r24);
    
    // Rule 25: High, High, Low -> Deep
    float r25 = fminf(fminf(ent_high, edg_high), prs_low);
    out_deep = fmaxf(out_deep, r25);
    
    // Rule 26: High, High, Medium -> Deep
    float r26 = fminf(fminf(ent_high, edg_high), prs_med);
    out_deep = fmaxf(out_deep, r26);
    
    // Rule 27: High, High, High -> Deep
    float r27 = fminf(fminf(ent_high, edg_high), prs_high);
    out_deep = fmaxf(out_deep, r27);
    


    // centroid defuzzification
    float numerator = 0.0f, denominator = 0.0f;
    const int samples = 100;
    const float step = 2.0f/(samples - 1);
    
    for (int i = 0; i < samples; i++){
        float d = 1.0f + i * step;
        float mu = fmaxf(out_shallow, fmaxf(out_moderate, out_deep));
        
        float mu_shallow = fuzzy_trapezoid(d, &engine->depth_sets.shallow) * out_shallow;
        float mu_moderate = fuzzy_trapezoid(d, &engine->depth_sets.moderate) * out_moderate;
        float mu_deep = fuzzy_trapezoid(d, &engine->depth_sets.deep) * out_deep;
        
        mu = fmaxf(fmaxf(mu_shallow, mu_moderate), mu_deep);
        
        numerator += d*mu;
        denominator += mu;
    }
    
    if (denominator < 1e-6f) return 2.0f;
    return numerator / denominator;
}



float fuzzy_infer(fuzzy_engine *engine, float entropy, float edge, float pressure) {
    // Clamp inputs
    if (entropy < 0.0f) entropy = 0.0f;
    if (entropy > 6.0f) entropy = 6.0f;
    if (edge < 0.0f) edge = 0.0f;
    if (edge > 1.0f) edge = 1.0f;
    if (pressure < 0.0f) pressure = 0.0f;
    if (pressure > 1.0f) pressure = 1.0f;
    
    // Fuzzify
    float ent_low = fuzzy_trapezoid(entropy, &engine->entropy_sets.low);
    float ent_med = fuzzy_trapezoid(entropy, &engine->entropy_sets.medium);
    float ent_high = fuzzy_trapezoid(entropy, &engine->entropy_sets.high);
    
    float edg_low = fuzzy_trapezoid(edge, &engine->edge_sets.low);
    float edg_med = fuzzy_trapezoid(edge, &engine->edge_sets.medium);
    float edg_high = fuzzy_trapezoid(edge, &engine->edge_sets.high);
    
    float prs_low = fuzzy_trapezoid(pressure, &engine->pressure_sets.low);
    float prs_med = fuzzy_trapezoid(pressure, &engine->pressure_sets.medium);
    float prs_high = fuzzy_trapezoid(pressure, &engine->pressure_sets.high);
    
    // Inference and defuzzification
    return mamdani_infer(engine, ent_low, ent_med, ent_high,
                         edg_low, edg_med, edg_high, prs_low, prs_med, prs_high);
}



feature_map* feature_map_create(size_t width, size_t height){
    feature_map *map = (feature_map *)malloc(sizeof(feature_map));
    if (!map) return NULL;
    
    size_t total = width * height;
    
    map->entropy = (float *)malloc(total * sizeof(float));
    map->edge = (float *)malloc(total * sizeof(float));
    map->pressure = (float *)malloc(total * sizeof(float));
    map->depth = (float *)malloc(total * sizeof(float));
    
    if (!map->entropy || !map->edge || !map->pressure || !map->depth){
        free(map->entropy);
        free(map->edge);
        free(map->pressure);
        free(map->depth);
        free(map);
        return NULL;
    }
    
    map->width = width;
    map->height = height;
    return map;
}



void feature_map_destroy(feature_map *map){
    if (map) {
        free(map->entropy);
        free(map->edge);
        free(map->pressure);
        free(map->depth);
        free(map);
    }
}
