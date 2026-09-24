#include <cstdio>
#include <cmath>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

// N is the block size of the DCT
// There is only one block, so the block size and the image size is the same
static const int N = 32;

static const float PI = 3.14159265358979f;

// Layout conventions used everywhere:
//   pixels        : [y*N + x]
//   coefficients  : [v*N + u]   (u = x-frequency, v = y-frequency)

int main() {

    // Stores the input image data converted to float
    float data[N*N];

    // DCT Coefficients, computed directly
    float dctCoeffDirect[N*N];

    // Inverse DCT computed directly
    float invDirect[N*N];

    // Used to temporarily store the input image when reading
    unsigned char* dataTemp;

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
        }
    }

    // Orthonormal DCT-II scale factors
    // Because the DCT is symmetric, this means that when transforming from
    // image to coefficients and then back, by applying the same transformation
    // twice, these coefficients will end up being squared. They end up being
    // 1.0/N and 2.0/N, which is like taking an average over the N samples.
    float a[N];

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

    // Initializes inverse with 0
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

    // Individual image buffers for data converted back to unsigned char
    unsigned char dataChar[N*N];
    unsigned char dctCoeffDirectAbsChar[N*N];
    unsigned char invDirectChar[N*N];

    // Image output
    for (int k = 0; k < N*N; k++) {

        // Converts back to [0, 255] unsigned char from [-1.0, 1.0] float
        dataChar[k]       = (data[k] + 1.0f)/2.0f * 255.0f;

        // DCT coefficients
        // Scaling factor is currently arbitrary, in order to normalize to
        // practical range, instead of theoretical maximum which would be
        // much larger and reduce constrast of most real DCT images
        float DCT_SCALE = 0.15f;
        // Unsigned magnitude of coefficient
        dctCoeffDirectAbsChar[k]  = fabsf(dctCoeffDirect[k]) * DCT_SCALE * 255.0f;

        // Converts back to [0, 255] unsigned char from [-1.0, 1.0] float
        invDirectChar[k]  = (invDirect[k] + 1.0f)/2.0f * 255.0f;

    }
    stbi_write_png("data.png",                N, N, 1, dataChar, N);
    stbi_write_png("dct_coeff_Direct_abs.png",N, N, 1, dctCoeffDirectAbsChar, N);
    stbi_write_png("reconst_direct.png",      N, N, 1, invDirectChar, N);

    return 0;
}
