#ifndef MICPP_IMAGE_HPP
#define MICPP_IMAGE_HPP
#include <filesystem>
#include <memory>
#include <vector>
#include <adaptor2d.hpp>

#ifndef NDEBUG
#include <cassert>
#endif

// Image represents a generic image at runtime that we can perform processes on
class Image {
    int m_width;
    int m_height;
    int m_channels;

    // Image data
    std::unique_ptr<unsigned char[]> m_data;

    public:
    // Load image from path
    Image(const std::filesystem::path& /* path to image file */);

    // DANGEROUS!!! DO NOT FREE THE POINTER YOU PROVIDE HERE ELSEWHERE!!!
    Image(std::unique_ptr<unsigned char[]>&& /* data */, int /* width */, int /* height */, int /* channels */);

    // Cleanup m_data with free to avoid UB by mixed memory allocation APIs
    ~Image();

    // The pixel union holds information about a certain pixel as a record
    union Pixel {
        // Getters
        [[nodiscard]] std::uint8_t r() const {
            return channel[0];
        }

        [[nodiscard]] std::uint8_t g() const {
            return channel[1];
        }

        [[nodiscard]] std::uint8_t b() const {
            return channel[2];
        }

        [[nodiscard]] std::uint8_t a() const {
            return channel[3];
        }

    private:
        // 0 = r, 1 = g, 2 = b, 3 = 4
        std::uint8_t channel[4];
        std::uint32_t rgba;

        // Private constructor that only Image can access, copy and move are public however
        Pixel() = default;
        friend Image;
    };

    // Get DCT coefficients ONLY ON CHANNEL 0
    [[nodiscard]] Adaptor2D<std::vector<double>> dct() const;

    // Getters
    [[nodiscard]] int width() const { return m_width; }
    [[nodiscard]] int height() const { return m_height; }
    [[nodiscard]] int channels() const { return m_channels; }

    [[nodiscard]] Pixel pixel_rgba(int /* x */, int /* y */) const;
};

#endif //MICPP_IMAGE_HPP
