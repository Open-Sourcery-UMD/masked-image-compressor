#include <iostream>
#include <argparse.hpp>
#include <image.hpp>

// Responsible for handling the rest of the program after we parse the arguments
static int dctdump(const argparse::ArgumentParser& parser);

int main(int argc, char* argv[]) {
    argparse::ArgumentParser parser{ "imgdump" };

    parser.add_description("imgdump is used to gather the components of an image and print them.");

    parser
        .add_argument("--file")
        .help("The image file to get the components from.")
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

        for (int y{ 0 }; y < image.height(); ++y) {
            for (int x{ 0 }; x < image.width(); ++x) {
                const Image::Pixel pixel{ image.pixel_rgba(x, y) };

                std::cout << '(' << x << ", " << y << "): (" << +pixel.r() << ", " << +pixel.g() << ", "
                    << +pixel.b() << ", " << +pixel.a() << ")\n";
            }
        }

        return 0;
    } catch (std::exception& ex) {
        std::cout << ex.what() << '\n';
        return 1;
    }
}
