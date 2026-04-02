# fzstego
Adaptive fuzzy logic-based steganographic framework in C

This project implements an **adaptive steganographic framework** in C, based on the research paper "Adaptive Fuzzy Logic-Based Steganographic Encryption Framework" by Joshi and Bhand (2026). It uses a **Mamdani-type fuzzy inference system** to dynamically determine the optimal embedding depth for every pixel based on local image characteristics.

## References
This framework implements the research from:

* **Joshi, A., & Bhand, K. (2026).** *Adaptive Fuzzy Logic-Based Steganographic Encryption Framework: A Comprehensive Experimental Evaluation*. arXiv:2603.18105v1 [cs.CR]. 
* **Link:** [https://arxiv.org/abs/2603.18105](https://arxiv.org/abs/2603.18105)

## Key Features
* **Content-Adaptive Embedding**: Calculates a pixel-wise embedding depth of 1–3 bits using local entropy, edge magnitude, and payload pressure as inputs.
* **Encoder-Decoder Synchronization**: Uses an **LSB-invariant feature extraction** strategy (lower-bit stripping) to ensure the receiver can recompute the exact same adaptive depth map from the stego-image.
* **High Visual Fidelity**: Designed to outperform fixed LSB methods, achieving high PSNR values by protecting smooth areas and utilizing textured regions.

# Getting Started

### Requirements
* GCC
* Make

### Installation & Build
```bash
# Clone the repository
git clone https://github.com/iiEliJas/fzstego.git
cd fuzzy-steganography

# Build the demo and tests
make
```
Note: The executables for the demo and tests all need bmp image files to work.
