#ifndef MICPP_IMAGE_HPP
#define MICPP_IMAGE_HPP
#include <filesystem>
#include <memory>
#include <vector>
#include <adaptor2d.hpp>

// Image represents a generic image at runtime that we can perform processes on
class Image {
    int m_width;
    int m_height;

    // Image data
    std::unique_ptr<unsigned char[]> m_data;

    public:
    // Load image from path
    Image(const std::filesystem::path& /* path to image file */);

    // DANGEROUS!!! DO NOT FREE THE POINTER YOU PROVIDE HERE ELSEWHERE!!!
    Image(std::unique_ptr<unsigned char[]>&& /* data */, int /* width */, int /* height */);

    // Cleanup m_data with free to avoid UB by mixed memory allocation APIs
    ~Image();

    // Get DCT coefficients ONLY ON CHANNEL 0
    [[nodiscard]] Adaptor2D<std::vector<double>> dct() const;
};

#endif //MICPP_IMAGE_HPP
