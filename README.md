## Minimal Example

The most basic example of loading an image, running the DCT (Discrete Cosine Transform) on it to convert it into DCT coefficients, running the IDCT (Inverse Discrete Cosine Transform) on the coefficient array to turn it back into the original image, and then writing the output.

This example is a good place to start when learning about the DCT, it can be easily modified, or it can serve as the template for converting into your favorite language.

One reccomended modification to play around with is to see what happens when you run the IDCT Image reconstruction on just one coefficent (set the outper loop veriables to a constant) or set it to run on a subset of the coefficients, for example running the loop to N/2 or N/4 instead of N.

## Comprehensive Examples

This is a collection of examples. In these examples, there are two separate ways of performing the DCT and IDCT that are implemented: Direct and LUT (look up table)  based. The direct method computes the DCT cosine values for each loop, while the LUT version computes a table first that stores the DCT values, which the DCT will use instead of computing cosine values in the loops. Since the DCT is chosen to be symmetric, the LUT can be used to compute both the DCT and IDCT, which this example does.

The LUT is also stored as an image, which appears as a N x N grid of N x N images. Each image corresponds to one coefficient of the DCT, with the top-left image corresponding to the DC (Constant) term, and the bottom-right is the highest frequency term. Each image is essentially a 2D graph of the cosine function, with separate horizontal and vertical frequencies for the horizontal and vertical cosine terms. It is reccomended to study the resulting LUT image, as it helps the intuition for how the DCT is able to 'find' patterns in data.

## Running the Examples

To compile an example, run make in the example directory. 
Alternatively, the example can be compiled just by running g++ directly on the example cpp file.
Clang has not been tested, but should work fine as well.

Each example can be run using the output .x file after compilation.
