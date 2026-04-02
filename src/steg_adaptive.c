#include "fuzzy_steg.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BMP_HEADER_SIZE 54
#define BITS_PER_BYTE 8
#define WINDOW_SIZE 7

#pragma pack(push, 1)

typedef struct {
    uint16_t signature;
    uint32_t file_size;
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t pixel_offset;
} BMP_FILE_HEADER;

typedef struct {
    uint32_t header_size;
    int32_t width;
    int32_t height;
    uint16_t planes;
    uint16_t bits_per_pixel;
    uint32_t compression;
    uint32_t image_size;
    int32_t x_pixels_per_meter;
    int32_t y_pixels_per_meter;
    uint32_t colors_used;
    uint32_t colors_important;
} BMP_INFO_HEADER;

#pragma pack(pop)


// Read BMP headers from file
static int read_bmp_headers(FILE *file, BMP_FILE_HEADER *fh, BMP_INFO_HEADER *ih) {
    if (fread(fh, sizeof(BMP_FILE_HEADER), 1, file) != 1){
        return -1;
    }
    if (fread(ih, sizeof(BMP_INFO_HEADER), 1, file) != 1){
        return -1;
    }
    return 0;
}

// Write BMP headers to file
static int write_bmp_headers(FILE *file, const BMP_FILE_HEADER *fh, const BMP_INFO_HEADER *ih) {
    if (fwrite(fh, sizeof(BMP_FILE_HEADER), 1, file) != 1){
        return -1;
    }
    if (fwrite(ih, sizeof(BMP_INFO_HEADER), 1, file) != 1){
        return -1;
    }
    return 0;
}

// Calculate bytes available
static size_t get_pixel_data_size(const BMP_INFO_HEADER *ih) {
    size_t width = (size_t)ih->width;
    size_t height = (size_t)ih->height;
    if (height < 0) height = -height;
    size_t total_bytes = width * height * 3;
    return total_bytes / BITS_PER_BYTE;
}

// Secure zero memory
static void secure_zero(unsigned char *data, size_t len) {
    if (data) {
        for (size_t i = 0; i < len; i++){
            data[i] = 0;
        }
    }
}


/////////////////////////////////////////////////////////////////////////////////////
//              EMBED DATA

int embed_data_adaptive(const char *input_image_path, const char *output_image_path, const unsigned char *data, size_t data_len, unsigned int seed) {
    FILE *input_file = fopen(input_image_path, "rb");
    if (!input_file) {
        fprintf(stderr, "Error: Cannot open input image\n");
        return -1;
    }

    BMP_FILE_HEADER fh;
    BMP_INFO_HEADER ih;

    if (read_bmp_headers(input_file, &fh, &ih) != 0) {
        fprintf(stderr, "Error: Cannot read BMP headers\n");
        fclose(input_file);
        return -1;
    }

    if (fh.signature != 0x4D42) {
        fprintf(stderr, "Error: Not a valid BMP file\n");
        fclose(input_file);
        return -1;
    }

    if (ih.bits_per_pixel != 24) {
        fprintf(stderr, "Error: Only 24-bit BMP files are supported\n");
        fclose(input_file);
        return -1;
    }

    // Read pixel data
    size_t pixel_data_size = fh.file_size - fh.pixel_offset;
    unsigned char *pixel_data = (unsigned char *)malloc(pixel_data_size);
    if (!pixel_data) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        fclose(input_file);
        return -1;
    }

    if (fseek(input_file, fh.pixel_offset, SEEK_SET) != 0){
        fprintf(stderr, "Error: Seek failed\n");
        free(pixel_data);
        fclose(input_file);
        return -1;
    }

    if (fread(pixel_data, pixel_data_size, 1, input_file) != 1){
        fprintf(stderr, "Error: Cannot read pixel data\n");
        free(pixel_data);
        fclose(input_file);
        return -1;
    }

    fclose(input_file);

    // strip lower 3 bits
    unsigned char *pixel_stripped = (unsigned char *)malloc(pixel_data_size);
    if (!pixel_stripped) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        free(pixel_data);
        return -1;
    }

    strip_lower_bits(pixel_data, pixel_stripped, pixel_data_size);

    // compute grayscale
    int width = ih.width;
    int height = (ih.height < 0) ? -ih.height : ih.height;
    unsigned char *grayscale = (unsigned char *)malloc(width * height);
    if (!grayscale) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        free(pixel_data);
        free(pixel_stripped);
        return -1;
    }

    compute_grayscale(pixel_stripped, grayscale, width, height);

    // fuzzy engine
    fuzzy_engine *engine = fuzzy_create();
    if (!engine) {
        fprintf(stderr, "Error: Fuzzy engine creation failed\n");
        free(pixel_data);
        free(pixel_stripped);
        free(grayscale);
        return -1;
    }

    // feature maps
    feature_map *entropy_map = build_entropy_map(grayscale, width, height, WINDOW_SIZE);
    if (!entropy_map) {
        fprintf(stderr, "Error: Entropy map creation failed\n");
        fuzzy_destroy(engine);
        free(pixel_data);
        free(pixel_stripped);
        free(grayscale);
        return -1;
    }

    feature_map *edge_map = build_edge_map(grayscale, width, height);
    if (!edge_map) {
        fprintf(stderr, "Error: Edge map creation failed\n");
        feature_map_destroy(entropy_map);
        fuzzy_destroy(engine);
        free(pixel_data);
        free(pixel_stripped);
        free(grayscale);
        return -1;
    }

    // temporary depth for pressure calculation
    feature_map *temp_depth = feature_map_create(width, height);
    if (!temp_depth) {
        fprintf(stderr, "Error: Temporary depth allocation failed\n");
        feature_map_destroy(edge_map);
        feature_map_destroy(entropy_map);
        fuzzy_destroy(engine);
        free(pixel_data);
        free(pixel_stripped);
        free(grayscale);
        return -1;
    }

    for (int i = 0; i < width * height; i++) {
        temp_depth->depth[i] = 2.0f;
    }

    feature_map *pressure_map = build_pressure_map(temp_depth, data_len, 3);
    if (!pressure_map) {
        fprintf(stderr, "Error: Pressure map creation failed\n");
        feature_map_destroy(temp_depth);
        feature_map_destroy(edge_map);
        feature_map_destroy(entropy_map);
        fuzzy_destroy(engine);
        free(pixel_data);
        free(pixel_stripped);
        free(grayscale);
        return -1;
    }

    // build depth map
    feature_map *depth_map = build_depth_map(engine, entropy_map, edge_map, pressure_map);
    if (!depth_map) {
        fprintf(stderr, "Error: Depth map creation failed\n");
        feature_map_destroy(pressure_map);
        feature_map_destroy(temp_depth);
        feature_map_destroy(edge_map);
        feature_map_destroy(entropy_map);
        fuzzy_destroy(engine);
        free(pixel_data);
        free(pixel_stripped);
        free(grayscale);
        return -1;
    }

    // generate permutation
    size_t total_pixels = width * height;
    size_t *perm = (size_t *)malloc(total_pixels * sizeof(size_t));
    if (!perm) {
        fprintf(stderr, "Error: Permutation allocation failed\n");
        feature_map_destroy(depth_map);
        feature_map_destroy(pressure_map);
        feature_map_destroy(temp_depth);
        feature_map_destroy(edge_map);
        feature_map_destroy(entropy_map);
        fuzzy_destroy(engine);
        free(pixel_data);
        free(pixel_stripped);
        free(grayscale);
        return -1;
    }

    for (size_t i = 0; i < total_pixels; i++) {
        perm[i] = i;
    }

    srand(seed);
    for (size_t i = total_pixels - 1; i > 0; i--) {
        size_t j = rand() % (i + 1);
        size_t temp = perm[i];
        perm[i] = perm[j];
        perm[j] = temp;
    }

    // embed buffer
    unsigned char *embed_buffer = (unsigned char *)malloc(sizeof(size_t) + data_len);
    if (!embed_buffer) {
        fprintf(stderr, "Error: Embed buffer allocation failed\n");
        free(perm);
        feature_map_destroy(depth_map);
        feature_map_destroy(pressure_map);
        feature_map_destroy(temp_depth);
        feature_map_destroy(edge_map);
        feature_map_destroy(entropy_map);
        fuzzy_destroy(engine);
        free(pixel_data);
        free(pixel_stripped);
        free(grayscale);
        return -1;
    }

    memcpy(embed_buffer, &data_len, sizeof(size_t));
    memcpy(embed_buffer + sizeof(size_t), data, data_len);
    size_t total_embed_len = sizeof(size_t) + data_len;

    // adaptive embedding
    size_t byte_position = 0;
    int bit_in_byte = 0;

    for (size_t p = 0; p < total_pixels; p++) {
        size_t pixel_idx = perm[p];
        int y = pixel_idx / width;
        int x = pixel_idx % width;

        int depth = (int)depth_map->depth[pixel_idx];
        if (depth < 1) depth = 1;
        if (depth > 3) depth = 3;

        for (int bit_idx = 0; bit_idx < depth; bit_idx++) {
            if (byte_position >= total_embed_len) {
                break;
            }

            unsigned char bit_val = (embed_buffer[byte_position] >> bit_in_byte) & 1;
            size_t pixel_offset = (y * width + x) * 3 + bit_idx;
            pixel_data[pixel_offset] = (pixel_data[pixel_offset] & ~1) | bit_val;

            bit_in_byte++;
            if (bit_in_byte >= 8) {
                bit_in_byte = 0;
                byte_position++;
            }
        }

        if (byte_position >= total_embed_len) {
            break;
        }
    }

    // output
    FILE *output_file = fopen(output_image_path, "wb");
    if (!output_file) {
        fprintf(stderr, "Error: Cannot create output file\n");
        free(embed_buffer);
        free(perm);
        feature_map_destroy(depth_map);
        feature_map_destroy(pressure_map);
        feature_map_destroy(temp_depth);
        feature_map_destroy(edge_map);
        feature_map_destroy(entropy_map);
        fuzzy_destroy(engine);
        free(pixel_data);
        free(pixel_stripped);
        free(grayscale);
        return -1;
    }

    if (write_bmp_headers(output_file, &fh, &ih) != 0) {
        fprintf(stderr, "Error: Cannot write BMP headers\n");
        fclose(output_file);
        free(embed_buffer);
        free(perm);
        feature_map_destroy(depth_map);
        feature_map_destroy(pressure_map);
        feature_map_destroy(temp_depth);
        feature_map_destroy(edge_map);
        feature_map_destroy(entropy_map);
        fuzzy_destroy(engine);
        free(pixel_data);
        free(pixel_stripped);
        free(grayscale);
        return -1;
    }

    if (fseek(output_file, fh.pixel_offset, SEEK_SET) != 0) {
        fprintf(stderr, "Error: Seek failed\n");
        fclose(output_file);
        free(embed_buffer);
        free(perm);
        feature_map_destroy(depth_map);
        feature_map_destroy(pressure_map);
        feature_map_destroy(temp_depth);
        feature_map_destroy(edge_map);
        feature_map_destroy(entropy_map);
        fuzzy_destroy(engine);
        free(pixel_data);
        free(pixel_stripped);
        free(grayscale);
        return -1;
    }

    if (fwrite(pixel_data, pixel_data_size, 1, output_file) != 1) {
        fprintf(stderr, "Error: Cannot write pixel data\n");
        fclose(output_file);
        free(embed_buffer);
        free(perm);
        feature_map_destroy(depth_map);
        feature_map_destroy(pressure_map);
        feature_map_destroy(temp_depth);
        feature_map_destroy(edge_map);
        feature_map_destroy(entropy_map);
        fuzzy_destroy(engine);
        free(pixel_data);
        free(pixel_stripped);
        free(grayscale);
        return -1;
    }

    fclose(output_file);

    // cleanup
    secure_zero(embed_buffer, sizeof(size_t) + data_len);
    free(embed_buffer);
    free(perm);
    feature_map_destroy(depth_map);
    feature_map_destroy(pressure_map);
    feature_map_destroy(temp_depth);
    feature_map_destroy(edge_map);
    feature_map_destroy(entropy_map);
    fuzzy_destroy(engine);
    free(pixel_data);
    free(pixel_stripped);
    free(grayscale);

    return 0;
}



unsigned char* extract_data_adaptive(const char *image_path, size_t *data_len,unsigned int seed) {
    FILE *file = fopen(image_path, "rb");
    if (!file) {
        fprintf(stderr, "Error: Cannot open image file\n");
        return NULL;
    }

    BMP_FILE_HEADER fh;
    BMP_INFO_HEADER ih;

    if (read_bmp_headers(file, &fh, &ih) != 0) {
        fprintf(stderr, "Error: Cannot read BMP headers\n");
        fclose(file);
        return NULL;
    }

    if (fh.signature != 0x4D42 || ih.bits_per_pixel != 24) {
        fprintf(stderr, "Error: Only 24-bit BMP supported\n");
        fclose(file);
        return NULL;
    }

    size_t pixel_data_size = fh.file_size - fh.pixel_offset;
    unsigned char *pixel_data = (unsigned char *)malloc(pixel_data_size);
    if (!pixel_data) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        fclose(file);
        return NULL;
    }

    if (fseek(file, fh.pixel_offset, SEEK_SET) != 0 ||
        fread(pixel_data, pixel_data_size, 1, file) != 1) {
        fprintf(stderr, "Error: Cannot read pixel data\n");
        secure_zero(pixel_data, pixel_data_size);
        free(pixel_data);
        fclose(file);
        return NULL;
    }

    fclose(file);

    // compute depth map
    unsigned char *pixel_stripped = (unsigned char *)malloc(pixel_data_size);
    if (!pixel_stripped) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        secure_zero(pixel_data, pixel_data_size);
        free(pixel_data);
        return NULL;
    }

    strip_lower_bits(pixel_data, pixel_stripped, pixel_data_size);

    int width = ih.width;
    int height = (ih.height < 0) ? -ih.height : ih.height;
    unsigned char *grayscale = (unsigned char *)malloc(width * height);
    if (!grayscale) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        secure_zero(pixel_stripped, pixel_data_size);
        free(pixel_stripped);
        secure_zero(pixel_data, pixel_data_size);
        free(pixel_data);
        return NULL;
    }

    compute_grayscale(pixel_stripped, grayscale, width, height);

    fuzzy_engine *engine = fuzzy_create();
    if (!engine) {
        fprintf(stderr, "Error: Fuzzy engine creation failed\n");
        free(grayscale);
        secure_zero(pixel_stripped, pixel_data_size);
        free(pixel_stripped);
        secure_zero(pixel_data, pixel_data_size);
        free(pixel_data);
        return NULL;
    }

    feature_map *entropy_map = build_entropy_map(grayscale, width, height, WINDOW_SIZE);
    feature_map *edge_map = build_edge_map(grayscale, width, height);
    feature_map *temp_depth = feature_map_create(width, height);

    for (int i = 0; i < width * height; i++) {
        temp_depth->depth[i] = 2.0f;
    }

    feature_map *pressure_map = build_pressure_map(temp_depth, 1024, 3);
    feature_map *depth_map = build_depth_map(engine, entropy_map, edge_map, pressure_map);

    // permutation
    size_t total_pixels = width * height;
    size_t *perm = (size_t *)malloc(total_pixels * sizeof(size_t));
    if (!perm) {
        fprintf(stderr, "Error: Permutation allocation failed\n");
        feature_map_destroy(depth_map);
        feature_map_destroy(pressure_map);
        feature_map_destroy(temp_depth);
        feature_map_destroy(edge_map);
        feature_map_destroy(entropy_map);
        fuzzy_destroy(engine);
        free(grayscale);
        secure_zero(pixel_stripped, pixel_data_size);
        free(pixel_stripped);
        secure_zero(pixel_data, pixel_data_size);
        free(pixel_data);
        return NULL;
    }

    for (size_t i = 0; i < total_pixels; i++) {
        perm[i] = i;
    }

    srand(seed);
    for (size_t i = total_pixels - 1; i > 0; i--) {
        size_t j = rand() % (i + 1);
        size_t temp = perm[i];
        perm[i] = perm[j];
        perm[j] = temp;
    }

    // extract bits
    unsigned char *extracted_buffer = (unsigned char *)malloc(pixel_data_size * 3);
    if (!extracted_buffer) {
        fprintf(stderr, "Error: Extraction buffer allocation failed\n");
        free(perm);
        feature_map_destroy(depth_map);
        feature_map_destroy(pressure_map);
        feature_map_destroy(temp_depth);
        feature_map_destroy(edge_map);
        feature_map_destroy(entropy_map);
        fuzzy_destroy(engine);
        free(grayscale);
        secure_zero(pixel_stripped, pixel_data_size);
        free(pixel_stripped);
        secure_zero(pixel_data, pixel_data_size);
        free(pixel_data);
        return NULL;
    }

    size_t byte_position = 0;
    int bit_in_byte = 0;
    memset(extracted_buffer, 0, pixel_data_size * 3);

    for (size_t p = 0; p < total_pixels; p++) {
        size_t pixel_idx = perm[p];
        int y = pixel_idx / width;
        int x = pixel_idx % width;

        int depth = (int)depth_map->depth[pixel_idx];
        if (depth < 1) depth = 1;
        if (depth > 3) depth = 3;

        for (int bit_idx = 0; bit_idx < depth; bit_idx++) {
            size_t pixel_offset = (y * width + x) * 3 + bit_idx;
            unsigned char bit_val = pixel_data[pixel_offset] & 1;

            extracted_buffer[byte_position] |= (bit_val << bit_in_byte);

            bit_in_byte++;
            if (bit_in_byte >= 8) {
                bit_in_byte = 0;
                byte_position++;
            }
        }
    }

    // length
    size_t length = *(size_t *)extracted_buffer;

    if (length > pixel_data_size || length == 0) {
        fprintf(stderr, "Error: Invalid embedded data\n");
        free(extracted_buffer);
        free(perm);
        feature_map_destroy(depth_map);
        feature_map_destroy(pressure_map);
        feature_map_destroy(temp_depth);
        feature_map_destroy(edge_map);
        feature_map_destroy(entropy_map);
        fuzzy_destroy(engine);
        free(grayscale);
        secure_zero(pixel_stripped, pixel_data_size);
        free(pixel_stripped);
        secure_zero(pixel_data, pixel_data_size);
        free(pixel_data);
        return NULL;
    }

    // result
    unsigned char *result = (unsigned char *)malloc(length);
    if (!result) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        free(extracted_buffer);
        free(perm);
        feature_map_destroy(depth_map);
        feature_map_destroy(pressure_map);
        feature_map_destroy(temp_depth);
        feature_map_destroy(edge_map);
        feature_map_destroy(entropy_map);
        fuzzy_destroy(engine);
        free(grayscale);
        secure_zero(pixel_stripped, pixel_data_size);
        free(pixel_stripped);
        secure_zero(pixel_data, pixel_data_size);
        free(pixel_data);
        return NULL;
    }

    memcpy(result, extracted_buffer + sizeof(size_t), length);
    *data_len = length;

    // cleanup
    free(extracted_buffer);
    free(perm);
    feature_map_destroy(depth_map);
    feature_map_destroy(pressure_map);
    feature_map_destroy(temp_depth);
    feature_map_destroy(edge_map);
    feature_map_destroy(entropy_map);
    fuzzy_destroy(engine);
    free(grayscale);
    secure_zero(pixel_stripped, pixel_data_size);
    free(pixel_stripped);
    secure_zero(pixel_data, pixel_data_size);
    free(pixel_data);

    return result;
}
