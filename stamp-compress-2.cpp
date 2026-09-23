#include <cstdio>
#include <cmath>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define MSF_GIF_IMPL
#include "msf_gif.h"

static const int N = 32;
static const float PI = 3.14159265358979f;

// Layout conventions used everywhere:
//   pixels        : [y*N + x]
//   coefficients  : [v*N + u]   (u = x-frequency, v = y-frequency)
//   DCT_LK        : [N*N*N*v + N*N*y + N*u + x]
//                   For N=8 a 64x64 image of 8x8 tiles; tile (u,v) is the basis
//                   function for coefficient (u,v), already scaled by a[u]*a[v]

static unsigned char toByte(float v) {
    v = (v + 1.0f)/2.0f * 255.0f;
    return (unsigned char)(v < 0.0f ? 0.0f : (v > 255.0f ? 255.0f : v + 0.5f));
}

static float maxAbsDiff(const float* p, const float* q, int n) {
    float m = 0.0f;
    for (int i = 0; i < n; i++) {
        float d = fabsf(p[i] - q[i]);
        if (d > m) m = d;
    }
    return m;
}

static void printMat(const char* title, const float* m) {
    printf("%s\n", title);
    for (int y = 0; y < N; y++) {
        for (int x = 0; x < N; x++) printf("%9.5f ", m[y*N + x]);
        printf("\n");
    }
    printf("\n");
}

int main() {
    float a[N];
    float data[N*N];
    unsigned char dataMask[N*N];

    // Used to store the input image when reading
    unsigned char* dataTemp;

    // DCT Computed directly
    float dctDirect[N*N];
    // DCT Computed via Look-Up-Table
    float dctLUT[N*N];
    // Inverse DCT Computed directly
    float invDirect[N*N];
    // Inverse DCT Computed via Look-Up-Table
    float invLUT[N*N];
    // Inverse DCT Computed via LUT, skips small terms
    float sparseReconLUT[N*N];

    float* DCT_LK = new float[N*N*N*N];
    unsigned char* imgOut = new unsigned char[N*N*N*N];

    unsigned char imageBuffer[N*N*4];

    unsigned char dataChar[N*N];
    unsigned char dctLUTChar[N*N];
    unsigned char dctLUTAbsChar[N*N];
    unsigned char invDirectChar[N*N];
    unsigned char invLUTChar[N*N];
    unsigned char sparseChar[N*N];
    unsigned char invLUTMaskedChar[N*N];

    // Test input
    /*
    for (int y = 0; y < N; y++)
        for (int x = 0; x < N; x++)
            data[y*N + x] = (float)(x*y) / (N*N);
    */
    int width, height, bpp;
    dataTemp= stbi_load("renderOut.png", &width, &height, &bpp, 0);

    for (int y = 0; y < N; y++) {
        for (int x = 0; x < N; x++) {
            data[y*N + x] = (float)(dataTemp[(y*N + x)*4 + 0] / (255.0))*2.0f - 1.0f;
            dataMask[y*N + x] = dataTemp[(y*N + x)*4 + 3];
        }
    }

    // GIF setup
    int gifWidth = N, gifHeight = N, centisecondsPerFrame = 50, quality = 16;
    MsfGifState gifState = {};
    // msf_gif_alpha_threshold = 128; //optionally, enable transparency (see function documentation below for details)
    msf_gif_begin(&gifState, gifWidth, gifHeight);

    //printMat("Input:", data);

    // Orthonormal DCT-II scale factors
    a[0] = sqrtf(1.0f / N);
    for (int k = 1; k < N; k++) a[k] = sqrtf(2.0f / N);

    // ---- Forward DCT, direct ----
    for (int v = 0; v < N; v++) {
        for (int u = 0; u < N; u++) {
            float sum = 0.0f;
            for (int y = 0; y < N; y++) {
                for (int x = 0; x < N; x++) {
                    sum += data[y*N + x]
                         * cosf((2*x + 1) * u * PI / (2*N))
                         * cosf((2*y + 1) * v * PI / (2*N));
                }
            }
            dctDirect[v*N + u] = a[u] * a[v] * sum;
        }
    }
    //printMat("DCT (direct):", dctDirect);

    // ---- Lookup table ----
    for (int v = 0; v < N; v++) {
        for (int u = 0; u < N; u++) {
            for (int y = 0; y < N; y++) {
                for (int x = 0; x < N; x++) {
                    int idx = N*N*N*v + N*N*y + N*u + x;
                    DCT_LK[idx] = a[u] * a[v]
                                * cosf((2*x + 1) * u * PI / (2*N))
                                * cosf((2*y + 1) * v * PI / (2*N));
                    // Basis values lie in [-2/N, 2/N]; map to 0..255
                    imgOut[idx] = toByte((DCT_LK[idx] + 2.0f/N) / (4.0f/N));
                }
            }
        }
    }
    stbi_write_png("dctMat.png", N*N, N*N, 1, imgOut, N*N);

    int opaquePixelCount = 0;
    // Count Opaque Pixels
    for (int y = 0; y < N; y++) {
        for (int x = 0; x < N; x++) {
            if (dataMask[N*y + x] !=0) {
                opaquePixelCount++;
            }
        }
    }

    // ---- Forward DCT, via table (one coefficient at a time) ----
    for (int v = 0; v < N; v++) {
        for (int u = 0; u < N; u++) {
            float sum = 0.0f;
            float opaqueIdealSum = 0.0f;
            float transIdealSum = 0.0f;
            float fullIdealSum = 0.0f;
            float individualNormalizedSum = 0.0f;
            for (int y = 0; y < N; y++) {
                for (int x = 0; x < N; x++) {
                    // Skip transparent pixels
                    float lutVal = DCT_LK[N*N*N*v + N*N*y + N*u + x];
                    float curVal = lutVal * data[y*N + x];
                    if (dataMask[N*y + x] != 0) {
                        sum += curVal;
                        opaqueIdealSum += abs(lutVal);
                        individualNormalizedSum += curVal / abs(lutVal);
                    } else {
                        transIdealSum += abs(lutVal);
                    }
                    fullIdealSum += abs(lutVal);
                }
            }

            float transSum = 0.0f;

            for (int y = 0; y < N; y++) {
                for (int x = 0; x < N; x++) {
                    // Skip transparent pixels
                    float lutVal = DCT_LK[N*N*N*v + N*N*y + N*u + x];
                    float curVal = lutVal * data[y*N + x];
                    if (dataMask[N*y + x] != 0) {

                    } else {
                        transSum += abs(lutVal) * individualNormalizedSum/(N*N);
                    }
                }
            }
            // Normalize sum to value as if opaque pixels only mattered

            // 
            float opaqueFactor = sum/opaqueIdealSum;

            float transparentPixelCount = N*N - opaquePixelCount;

            //float result = transparentPixelCount/(2*N*N) * (opaqueFactor * transIdealSum);
            float result = transparentPixelCount * (individualNormalizedSum/(N*N) * transIdealSum/(N*N));

            dctLUT[v*N + u] = sum + transSum;
            //dctLUT[v*N + u] = sum;
        }
    }
    //printMat("DCT (table):", dctLUT);

    // ---- Inverse DCT, direct: coefficient-outer, pixel-inner ----
    for (int k = 0; k < N*N; k++) invDirect[k] = 0.0f;

    for (int v = 0; v < N; v++) {
        for (int u = 0; u < N; u++) {
            float c = dctDirect[v*N + u];
            if (c == 0.0f) continue;          // skip empty coefficient
            float w = a[u] * a[v] * c;

            for (int y = 0; y < N; y++) {
                for (int x = 0; x < N; x++) {
                    invDirect[y*N + x] += w
                        * cosf((2*x + 1) * u * PI / (2*N))
                        * cosf((2*y + 1) * v * PI / (2*N));
                }
            }
        }
    }
    //printMat("Inverse (direct):", invDirect);

    // ---- Inverse DCT, via table: coefficient-outer, pixel-inner ----
    for (int k = 0; k < N*N; k++) invLUT[k] = 0.0f;

    for (int curNv = 1; curNv <= N; curNv++) {
    //for (int curNu = 1; curNu <= N; curNu++) {

    // Clear
    for (int y = 0; y < N; y++) {
        for (int x = 0; x < N; x++) {
            invLUT[N*y + x] = 0;
        }
    }

    // Doing subset for testing
    for (int v = 0; v < curNv; v++) {
        for (int u = 0; u < curNv; u++) {
            float c = dctLUT[v*N + u];
            //if (c == 0.0f) continue;          // skip empty coefficient

            for (int y = 0; y < N; y++) {
                for (int x = 0; x < N; x++) {
                    invLUT[y*N + x] += c * DCT_LK[N*N*N*v + N*N*y + N*u + x];
                }
            }
        }
    }

    for (int y = 0; y < N; y++) {
        for (int x = 0; x < N; x++) {
            int ibIdx = (y*N + x)*4;
            if (true) {
                imageBuffer[ibIdx+0] = toByte(invLUT[N*y + x]);
                imageBuffer[ibIdx+1] = toByte(invLUT[N*y + x]);
                imageBuffer[ibIdx+2] = toByte(invLUT[N*y + x]);
                imageBuffer[ibIdx+3] = 0xFF;
            } else {
                imageBuffer[ibIdx+0] = 0x40;
                imageBuffer[ibIdx+1] = 0x40;
                imageBuffer[ibIdx+2] = 0x40;
                imageBuffer[ibIdx+3] = 0xFF;
            }

        }
    }

    msf_gif_frame(&gifState, imageBuffer, centisecondsPerFrame, quality, width * 4);

    //}
    }

    // Have final result displayed for longer
    msf_gif_frame(&gifState, imageBuffer, centisecondsPerFrame * 2, quality, width * 4);

    // Write GIF
    MsfGifResult result = msf_gif_end(&gifState);
    if (result.data) {
        FILE * fp = fopen("sequence.gif", "wb");
        fwrite(result.data, result.dataSize, 1, fp);
        fclose(fp);
    }
    msf_gif_free(result);

    //printMat("Inverse (table):", invLUT);

    // ---- Sparse reconstruction: drop small coefficients ----
    const float threshold = 0.05f;
    int kept = 0;
    for (int k = 0; k < N*N; k++) sparseReconLUT[k] = 0.0f;

    for (int v = 0; v < N; v++) {
        for (int u = 0; u < N; u++) {
            float c = dctLUT[v*N + u];
            if (fabsf(c) < threshold) continue;
            kept++;
            for (int y = 0; y < N; y++)
                for (int x = 0; x < N; x++)
                    sparseReconLUT[y*N + x] += c * DCT_LK[N*N*N*v + N*N*y + N*u + x];
        }
    }
    printf("Sparse reconstruction kept %d of %d coefficients (threshold %.3f)\n",
           kept, N*N, threshold);
    //printMat("Sparse recon:", sparseReconLUT);

    // ---- Checks ----
    float sumData = 0.0f;
    for (int k = 0; k < N*N; k++) sumData += data[k];

    printf("Checks:\n");
    printf("  max |direct DCT  - table DCT|      = %g\n", maxAbsDiff(dctDirect, dctLUT, N*N));
    printf("  max |data - direct inverse|        = %g\n", maxAbsDiff(data, invDirect, N*N));
    printf("  max |data - table inverse|         = %g\n", maxAbsDiff(data, invLUT, N*N));
    printf("  max |data - sparse recon|          = %g\n", maxAbsDiff(data, sparseReconLUT, N*N));
    printf("  DC coefficient = %f, sum(data)/N = %f\n", dctLUT[0], sumData / N);

    // ---- Images ----
    for (int k = 0; k < N*N; k++) {
        dataChar[k]       = toByte(data[k]);
        // Signed DCT coefficients (centered on gray), with arbitary scaling factor
        dctLUTChar[k]     = toByte(0.5f + dctLUT[k] * 0.15f);
        // Unsigned magnitude of coefficient
        dctLUTAbsChar[k]     = toByte(fabsf(dctLUT[k]) * 0.15f);
        invDirectChar[k]  = toByte(invDirect[k]);
        invLUTChar[k]     = toByte(invLUT[k]);
        sparseChar[k]     = toByte(sparseReconLUT[k]);

        if (dataMask[k] !=0) {
            invLUTMaskedChar[k] = invLUTChar[k];
        } else {
            invLUTMaskedChar[k] = 0x40;
        }

    }
    stbi_write_png("data.png",                N, N, 1, dataChar, N);
    stbi_write_png("dct_LUT.png",             N, N, 1, dctLUTChar, N);
    stbi_write_png("dct_LUT_abs.png",         N, N, 1, dctLUTAbsChar, N);
    stbi_write_png("directRecon.png",         N, N, 1, invDirectChar, N);
    stbi_write_png("dataReconLUT.png",        N, N, 1, invLUTChar, N);
    stbi_write_png("dataReconLUT_sparse.png", N, N, 1, sparseChar, N);
    stbi_write_png("dataReconLUT_Masked.png", N, N, 1, invLUTMaskedChar,N);

    delete[] DCT_LK;
    delete[] imgOut;
    return 0;
}
