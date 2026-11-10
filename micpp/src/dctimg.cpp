#include <iostream>
#include <algorithm>
#include <argparse.hpp>
#include <image.hpp>

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
#endif

// Responsible for handling the rest of the program after we parse the arguments
static int dctdump(const argparse::ArgumentParser& parser);

int main(int argc, char* argv[]) {
    argparse::ArgumentParser parser{ "dctimg" };

    parser.add_description(
        "dctimg is used to calculate the DCT coefficients of an image and display them in another image.");

    parser
        .add_argument("--file")
        .help("The image file to get the DCT coefficients from.")
        .required();

    try {
        parser.parse_args(argc, argv);
    } catch (std::exception& ex) {
        std::cout << ex.what() << '\n';
        return 1;
    }

    return dctdump(parser);
}

int dctdump(const argparse::ArgumentParser& parser) {
    try {
        const std::filesystem::path input_filepath{ parser.get("--file") };
        const Image image{ input_filepath };

        const Adaptor2D adaptor{ image.dct() };

        std::vector<std::uint8_t> result_image{};
        result_image.reserve(adaptor.width() * adaptor.height() * 4 /* RGBA */);

        const double max{
            std::abs(*std::max_element(adaptor.cbegin(), adaptor.cend(), [](const double x, const double y) {
                return std::abs(x) < std::abs(y);
            }))
        };

        for (std::size_t y{ 0 }; y < adaptor.height(); ++y) {
            for (std::size_t x{ 0 }; x < adaptor.width(); ++x) {
                const auto scaled{ static_cast<std::uint8_t>(adaptor[x, y] / max * 255) };

                for (int i{ 0 }; i < 4; ++i) {
                    result_image.push_back(scaled);
                }
            }
        }

        const auto width{ static_cast<int>(adaptor.width()) };
        const auto height{ static_cast<int>(adaptor.height()) };

        // Ths stride is how many bytes are in a row (width * 4 for RGBA);
        if (stbi_write_png("dctimg.png", width, height, 4 /* RGBA */, result_image.data(), width * 4) <= 0) {
            std::cout << "An error occurred while writing the file!\n";
        }

        return 0;
    } catch (std::exception& ex) {
        std::cout << ex.what() << '\n';
        return 1;
    }
}
