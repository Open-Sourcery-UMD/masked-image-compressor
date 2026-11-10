#include <cmath>
#include <image.hpp>

#ifndef NDEBUG
#include <cassert>
#endif

// Define needed to make the functions are actually implemented in the header.
// This header can only be included once, and it will only be included here
#ifndef STBI_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#endif


Image::Image(const std::filesystem::path& path) : m_width{ 0 }, m_height{ 0 }, m_channels{ 0 } {
    FILE* file{ fopen(path.c_str(), "rb") };

    if (file == nullptr) {
        throw std::runtime_error("Failed to open file");
    }


    std::unique_ptr<unsigned char[]> p_data{
        stbi_load_from_file(file, &m_width, &m_height, &m_channels, 4)
    };

    if (p_data == nullptr || m_width != m_height) {
        throw std::runtime_error("Image can't be read or is not a square");
    }

    m_data = std::move(p_data);
}

Image::Image(std::unique_ptr<unsigned char[]>&& data, const int width, const int height, const int channels) :
    m_width{ width }, m_height{ height }, m_channels{ channels } {
    if (data == nullptr || width < 0 || height < 0 || height != width) {
        throw std::runtime_error("Invalid parameters");
    }

    this->m_data = std::move(data);
}

Image::~Image() {
    unsigned char* p_data{ m_data.get() };
    free(p_data);

    m_data.release();
}

Adaptor2D<std::vector<double>> Image::dct() const {
    std::vector<double> dct{};
    dct.reserve(m_width * m_height);

    auto cos_term{
        [this](const std::size_t a, const std::size_t b) -> double {
            const auto a_double{ static_cast<double>(a) };
            const auto b_double{ static_cast<double>(b) };

            return std::cos(
                (2.0 * a_double + 1.0) * b_double * std::numbers::pi / (2.0 * static_cast<double>(m_width)));
        }
    };

    auto coefficient{
        [this](const std::size_t x) -> double {
            if (x == 0) {
                return 1.0 / std::sqrt(static_cast<double>(m_width));
            }

            return std::sqrt(2.0 / static_cast<double>(m_width));
        }
    };

    for (std::size_t v{ 0 }; v < m_height; ++v) {
        for (std::size_t u{ 0 }; u < m_width; ++u) {
            double sum{ 0.0 };


            for (std::size_t y{ 0 }; y < m_height; ++y) {
                for (std::size_t x{ 0 }; x < m_width; ++x) {
                    const std::size_t index{ (y * m_width + x) * m_channels };

                    const double channel_0{ m_data[index] / 255.0 * 2.0 - 1.0 };

                    sum += channel_0 * cos_term(y, v) * cos_term(x, u) * coefficient(u) * coefficient(v);
                }
            }

            dct.push_back(sum);
        }
    }

#ifndef NDEBUG
    assert(dct.size() == m_width * m_height);
#endif

    return Adaptor2D{ std::move(dct), static_cast<std::size_t>(m_width), static_cast<std::size_t>(m_height) };
}

Image::Pixel Image::pixel_rgba(const int x, const int y) const {
    Pixel p {};

#ifndef NDEBUG
    assert(m_channels <= 4);
#endif

    for (int i {0}; i < m_channels; ++i) {
        //
        p.rgba |=
            static_cast<std::uint32_t>(m_data[(y * m_width + x) * m_channels + i]) << (8 * i);
    }

    return p;
}
