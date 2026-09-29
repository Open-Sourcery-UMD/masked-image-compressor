#include <cstdio>
#include <cmath>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define MSF_GIF_IMPL
#include "msf_gif.h"

// N is the block size of the DCT
// There is only one block, so the block size and the image size is the same
static const int N = 32;

static const float PI = 3.14159265358979f;

// Size of each GIF pixel, padded for nearest approx
int PAD = 8;

// Set to value for transparency, 256 disables for image output
int MASK_CHECK = 256;

// Layout conventions used everywhere:
//   pixels        : [y*N + x]
//   coefficients  : [v*N + u]   (u = x-frequency, v = y-frequency)
//   DCT_LUT        : [N*N*N*v + N*N*y + N*u + x]
//                   For N=8 a 64x64 image of 8x8 tiles; tile (u,v) is the basis
//                   function for coefficient (u,v), already scaled by a[u]*a[v]

static unsigned char toByte(float v) {
    v = (v + 1.0f)/2.0f * 255.0f;
    return (unsigned char)(v < 0.0f ? 0.0f : (v > 255.0f ? 255.0f : v + 0.5f));
}

static float maxAbsDiff(const float* p, const float* q, const unsigned char* mask, int n) {
    float m = 0.0f;
    for (int i = 0; i < n; i++) {
        if (mask[i] != 0) {
            float d = fabsf(p[i] - q[i]);
            if (d > m) m = d;
        }
    }
    return m;
}

int main() {
    float a[N];
    float data[N*N];
    float dataCopy[N*N];
    unsigned char dataMask[N*N];

    // Used to store the input image when reading
    unsigned char* dataTemp;

    // DCT Computed directly
    float dctCoeffDirect[N*N];
    // DCT Computed via Look-Up-Table
    float dctCoeffLUT[N*N];
    // Inverse DCT Computed directly
    float invDirect[N*N];
    // Inverse DCT Computed via Look-Up-Table
    float invLUT[N*N];

    // DCT Look Up Table, computed once
    // Stores N*N coefficient blocks each of size N*N
    float* DCT_LUT = new float[N*N*N*N];

    // Buffer storing DCT LUT image, with additional 1 pixel gap between blocks
    unsigned char* dctLUTImage = new unsigned char[(N+1)*(N+1)*N*N];

    // Buffers the gif images, stores RGBA
    unsigned char imageBuffer[N*N*PAD*PAD*4];

    // Individual image buffers for data converted to unsigned char
    unsigned char dataChar[N*PAD*N*PAD];
    unsigned char dctCoeffLUTAbsChar[N*PAD*N*PAD];
    unsigned char dctCoeffDirectAbsChar[N*PAD*N*PAD];
    unsigned char invDirectChar[N*PAD*N*PAD];
    unsigned char invLUTChar[N*PAD*N*PAD];
    unsigned char invLUTMaskedChar[N*PAD*N*PAD];

    // Loads the input image
    int width, height, bpp;
    dataTemp = stbi_load("input_image.png", &width, &height, &bpp, 0);

    // Fails if loaded image is not N x N
    if (width != N || height != N) {
        printf("Loaded image is not NxN, exiting\n");
        return EXIT_FAILURE;
    }

    // Converts the image from char [0, 255] values, to float [-1.0, 1.0].
    for (int y = 0; y < N; y++) {
        for (int x = 0; x < N; x++) {
            int dataIndex = N*y + x;
            int inputIndex = dataIndex*4;

            // Copies from channel 0 of RGBA image
            data[dataIndex] = (float)(dataTemp[inputIndex + 0] / (255.0))*2.0f - 1.0f;
            dataCopy[dataIndex] = (float)(dataTemp[inputIndex + 0] / (255.0))*2.0f - 1.0f;

            // Channels 2 and 3 are currently unused, would be used for
            // full color RGB images

            // Copies from channel 3 of RGBA image, the alpha value
            // Keeps unsigned char value
            dataMask[dataIndex] = dataTemp[inputIndex + 3];
        }
    }

    // GIF setup
    int gifWidth = N*PAD, gifHeight = N*PAD, centisecondsPerFrame = 50, quality = 16;
    MsfGifState gifState = {};
    // Uncomment to enable gif transparency
    //msf_gif_alpha_threshold = 128;
    msf_gif_begin(&gifState, gifWidth, gifHeight);

    // Orthonormal DCT-II scale factors
    a[0] = sqrtf(1.0f / N);

    for (int k = 1; k < N; k++) {
        a[k] = sqrtf(2.0f / N);
    }

    // Forward DCT, directly calculated
    // Computes each DCT coefficient before moving on to the next
    // Outer loop iterates over all the DCT coefficients
    for (int v = 0; v < N; v++) {
        for (int u = 0; u < N; u++) {

            // Inner loop iterates over the pixels of the image and the
            // corresponding entires of the DCT coefficient at (u, v)
            float sum = 0.0f;

            for (int y = 0; y < N; y++) {
                for (int x = 0; x < N; x++) {

                    sum += data[y*N + x]
                         * cosf((2*x + 1) * u * PI / (2*N))
                         * cosf((2*y + 1) * v * PI / (2*N));

                }
            }

            // Sets the DCT coefficient (u, v) to the result
            dctCoeffDirect[v*N + u] = a[u] * a[v] * sum;
        }
    }

    // Generates DCT Lookup table
    // The LUT is symmetric, so can be used for both DCT and IDCT
    // (AKA, can be used for both converting from pixels to coefficients (DCT),
    // as well as converting back from coefficients to pixels. (IDCT))
    for (int v = 0; v < N; v++) {
        for (int u = 0; u < N; u++) {
            for (int y = 0; y < N; y++) {
                for (int x = 0; x < N; x++) {

                    int idx = N*N*N*v + N*N*y + N*u + x;
                    DCT_LUT[idx] = a[u] * a[v]
                                * cosf((2*x + 1) * u * PI / (2*N))
                                * cosf((2*y + 1) * v * PI / (2*N));

                }
            }
        }
    }

    // Outputs DCT Lookup table as image, for visualization
    for (int v = 0; v < N; v++) {
        for (int u = 0; u < N; u++) {
            for (int y = 0; y < N; y++) {
                for (int x = 0; x < N; x++) {
                    int idx = N*N*N*v + N*N*y + N*u + x;

                    // Output adds a 1 pixel border between coefficients
                    int imgIdx = (N+1)*N*(N+1)*v + N*(N+1)*y + (N+1)*u + x;

                    // Scales the LUT image to be visible
                    // Not used as part of the calculation, just for visualization
                    dctLUTImage[imgIdx] = (DCT_LUT[idx] + a[u]*a[v]) / (a[u]*a[v])/2.0*255.0;
                }
            }
        }
    }

    // The DCT LUT is a NxN array of entries, with each entry corresponding
    // to a specific DCT coefficient. Each entry is a grid of NxN values.
    stbi_write_png("DCT_LUT.png", (N+1)*N, (N+1)*N, 1, dctLUTImage, (N+1)*N);

    // Calculates the DCT using the LUT
    // Computes each DCT coefficient before moving on to the next
    // Outer loop iterates over all the DCT coefficients
    for (int v = 0; v < N; v++) {
        for (int u = 0; u < N; u++) {

            float sum = 0.0f;

            // Inner loop iterates over the pixels of the image and the
            // corresponding entires of the DCT coefficient at (u, v)
            for (int y = 0; y < N; y++) {
                for (int x = 0; x < N; x++) {

                    float lutVal = DCT_LUT[N*N*N*v + N*N*y + N*u + x];
                    sum += lutVal * dataCopy[y*N + x];

                }
            }

            // Sets the DCT coefficient (u, v) to the result
            dctCoeffLUT[v*N + u] = sum;

        }
    }

    for (int k = 0; k < N*N; k++) {
        invDirect[k] = 0.0f;
    }

    // Inverse DCT, directly calculated
    for (int v = 0; v < N; v++) {
        for (int u = 0; u < N; u++) {
            float coeff = dctCoeffDirect[v*N + u];
            float weightedCoeff = a[u] * a[v] * coeff;

            for (int y = 0; y < N; y++) {
                for (int x = 0; x < N; x++) {
                    invDirect[y*N + x] += weightedCoeff
                        * cosf((2*x + 1) * u * PI / (2*N))
                        * cosf((2*y + 1) * v * PI / (2*N));
                }
            }
        }
    }

    // Inverse DCT LUT is calculated at differing subsections of the DCT
    // coefficients, run in a loop

    // Generates GIF with each frame showing the curN by curN block of
    // coefficients transformed into the image
    // The first frame shows just 1 coefficient, N=0, which is the DC term
    // The next frame shows 2x2 coefficients, etc.

    // Algorithm: Performs a loop, where after each subset of the DCT is
    // calculated, the next iteration of the loop will perform an IDCT
    // on the previous result, and set that result to the transparent
    // pixel data.
    // This results in the low frequency data being evaluated onto the
    // transparent pixels.
    // Aka, for iteration 1, finds the average DC value of image
    // Iteration 2 sets the background to the average value, then
    // runs the algorithm again to find the lowest frequency values.
    for (int curN = 1; curN <= N; curN++) {

        // Forward DCT
        for (int v = 0; v < curN; v++) {
            for (int u = 0; u < curN; u++) {

                float sum = 0.0f;

                // Inner loop iterates over the pixels of the image and the
                // corresponding entires of the DCT coefficient at (u, v)
                for (int y = 0; y < N; y++) {
                    for (int x = 0; x < N; x++) {

                        float lutVal = DCT_LUT[N*N*N*v + N*N*y + N*u + x];
                        sum += lutVal * dataCopy[y*N + x];

                    }
                }

                // Sets the DCT coefficient (u, v) to the result
                dctCoeffLUT[v*N + u] = sum;

            }
        }

        // Reset output
        for (int y = 0; y < N; y++) {
            for (int x = 0; x < N; x++) {
                invLUT[N*y + x] = 0;
            }
        }

        // Inverse DCT
        for (int v = 0; v < curN; v++) {
            for (int u = 0; u < curN; u++) {
                float c = dctCoeffLUT[v*N + u];

                for (int y = 0; y < N; y++) {
                    for (int x = 0; x < N; x++) {
                        invLUT[y*N + x] += c * DCT_LUT[N*N*N*v + N*N*y + N*u + x];
                    }
                }
            }
        }

        // Copy background for next iteration
        for (int y = 0; y < N; y++) {
            for (int x = 0; x < N; x++) {
                int dataIdx = N*y + x;
                // Sets transparent background pixels to value of previous iteration
                if (dataMask[dataIdx] == 0) {
                    dataCopy[dataIdx] = invLUT[dataIdx];
                }
            }
        }

        // Copies each frame to the imageBuffer, converting it from
        // grayscale to RGBA (With RGB channels all the same)
        for (int y = 0; y < N; y++) {
            for (int x = 0; x < N; x++) {
                int dataIdx = N*y + x;
                //int ibIdx = dataIdx*4;

                for (int padX = 0; padX < PAD; padX++) {
                    for (int padY = 0; padY < PAD; padY++) {

                        int ibIdx = (PAD*PAD*N*(y) + PAD*N*padY + (PAD*x + padX))*4;

                        if (dataMask[dataIdx] != MASK_CHECK) {
                            imageBuffer[ibIdx+0] = toByte(invLUT[dataIdx]);
                            imageBuffer[ibIdx+1] = toByte(invLUT[dataIdx]);
                            imageBuffer[ibIdx+2] = toByte(invLUT[dataIdx]);
                            imageBuffer[ibIdx+3] = 0xFF;
                        } else {
                            imageBuffer[ibIdx+0] = 0x40;
                            imageBuffer[ibIdx+1] = 0x40;
                            imageBuffer[ibIdx+2] = 0x40;
                            imageBuffer[ibIdx+3] = 0x00;
                        }
                    }
                }

            }
        }

        msf_gif_frame(&gifState, imageBuffer, centisecondsPerFrame, quality, gifWidth * 4);

    }

    // Have final result displayed for longer
    msf_gif_frame(&gifState, imageBuffer, centisecondsPerFrame * 2, quality, gifWidth * 4);

    // Write GIF
    MsfGifResult result = msf_gif_end(&gifState);
    if (result.data) {
        FILE * fp = fopen("sequence.gif", "wb");
        fwrite(result.data, result.dataSize, 1, fp);
        fclose(fp);
    }
    msf_gif_free(result);

    // Analysis of results

    // Computes the average, to check against DC coefficient
    float sumData = 0.0f;
    for (int k = 0; k < N*N; k++) {
        sumData += data[k];
    }
    float averageData = sumData/N;

    printf("Checks:\n");
    printf("  max abs(direct DCT  - table DCT)      = %g\n", maxAbsDiff(dctCoeffDirect, dctCoeffLUT, dataMask, N*N));
    printf("  max abs(data - direct inverse)        = %g\n", maxAbsDiff(data, invDirect, dataMask, N*N));
    printf("  max abs(data - table inverse)         = %g\n", maxAbsDiff(data, invLUT, dataMask, N*N));
    printf("  DCT DC coefficient = %f, sum(data)/N = %f\n", dctCoeffLUT[0], averageData);

    // Image output
    for (int y = 0; y < N; y++) {
        for (int x = 0; x < N; x++) {
            int dataIdx = N*y + x;

            for (int padX = 0; padX < PAD; padX++) {
                for (int padY = 0; padY < PAD; padY++) {

                    int ibIdx = (PAD*PAD*N*(y) + PAD*N*padY + (PAD*x + padX));

                    dataChar[ibIdx]       = toByte(data[dataIdx]);
                    // Signed DCT coefficients (centered on gray)
                    // Scaling factor is currently arbitrary, in order to normalize to
                    // practical range, instead of theoretical maximum which would be
                    // much larger and reduce constrast of most real DCT images
                    float DCT_SCALE = 0.15f;
                    // Unsigned magnitude of coefficient
                    dctCoeffLUTAbsChar[ibIdx]     = fabsf(dctCoeffLUT[dataIdx]) * DCT_SCALE * 255.0f;
                    dctCoeffDirectAbsChar[ibIdx]  = fabsf(dctCoeffDirect[dataIdx]) * DCT_SCALE * 255.0f;
                    invDirectChar[ibIdx]  = toByte(invDirect[dataIdx]);
                    invLUTChar[ibIdx]     = toByte(invLUT[dataIdx]);

                    if (dataMask[dataIdx] != MASK_CHECK) {
                        invLUTMaskedChar[ibIdx] = invLUTChar[dataIdx];
                    } else {
                        invLUTMaskedChar[ibIdx] = 0x40;
                    }
                }
            }
        }
    }
    stbi_write_png("data.png",                N*PAD, N*PAD, 1, dataChar, N*PAD);
    stbi_write_png("dct_coeff_LUT_abs.png",   N*PAD, N*PAD, 1, dctCoeffLUTAbsChar, N*PAD);
    stbi_write_png("dct_coeff_Direct_abs.png",N*PAD, N*PAD, 1, dctCoeffDirectAbsChar, N*PAD);
    stbi_write_png("reconst_direct.png",      N*PAD, N*PAD, 1, invDirectChar, N*PAD);
    stbi_write_png("reconst_LUT.png",         N*PAD, N*PAD, 1, invLUTChar, N*PAD);
    stbi_write_png("reconst_LUT_Masked.png",  N*PAD, N*PAD, 1, invLUTMaskedChar,N*PAD);

    delete[] DCT_LUT;
    delete[] dctLUTImage;
    return 0;
}
