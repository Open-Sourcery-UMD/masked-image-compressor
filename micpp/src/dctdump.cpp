#include <iostream>
#include <argparse.hpp>
#include <image.hpp>

// Responsible for handling the rest of the program after we parse the arguments
static int dctdump(const argparse::ArgumentParser& parser);

int main(int argc, char* argv[]) {
    argparse::ArgumentParser parser{ "dctdump" };

    parser.add_description("dctdump is used to calculate the DCT coefficients of an image and print them.");

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

        for (std::size_t y{ 0 }; y < adaptor.height(); ++y) {
            for (std::size_t x{ 0 }; x < adaptor.width(); ++x) {
                std::cout << '(' << x << ", " << y << "): " << adaptor[x, y] << '\n';
            }
        }

        return 0;
    } catch (std::exception& ex) {
        std::cout << ex.what() << '\n';
        return 1;
    }
}
