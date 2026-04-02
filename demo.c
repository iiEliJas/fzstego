#include "src/fzstego.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


int main(void) {
    printf("\n\n-----------------------------------------------------\n");
    printf("EXAMPLE 1: Embedding data\n");
    printf("-----------------------------------------------------\n");
    
    unsigned char secret_message[] = "secret messageeeee!";
    size_t message_len = strlen((char*)secret_message);
    unsigned int seed = 123; 
    
    printf("message: %s\n", secret_message);
    printf("length: %zu bytes\n", message_len);
    printf("seed: %u\n", seed);
    
    int embed_result = embed_data_adaptive(
        "lilly.bmp",           // Input image
        "stego.bmp",           // Output image
        secret_message,  
        message_len,           
        seed   
    );
    
    if (embed_result == 0) {
        printf("Embedding successful\n");
    } else {
        printf("Embedding failed\n");
    }
    




    printf("\n\n-----------------------------------------------------\n");
    printf("EXAMPLE 2: Extracting data\n");
    printf("-----------------------------------------------------\n");
    
    size_t recovered_len;
    unsigned char *recovered_message = extract_data_adaptive(
        "stego.bmp",           
        &recovered_len,    
        seed 
    );
    
    if (recovered_message) {
        printf("Extraction successful!\n");
        printf("message (%zu bytes):\n", recovered_len);
        printf("  \"%.*s\"\n", (int)recovered_len, recovered_message);
        
        if (recovered_len == message_len &&
            memcmp(recovered_message, secret_message, message_len) == 0) {
            printf("\nRecovered message matches original!\n");
        } else {
            printf("\nMismatch!\n");
        }
        
        free(recovered_message);
    } else {
        printf("Extraction failed\n");
    }
    
    printf("\n");
    
    return 0;
}
