use clap::Parser;
use image::{ColorType, DynamicImage, Rgba, GenericImage};
use libmicrs::image::Image;
use std::error::Error;

#[derive(Parser, Debug)]
#[command(
    version,
    about,
    long_about = "Dumps an image of the file transformed by the type II DCT."
)]
struct Args {
    /// The name of the file to generate a DCT image from
    #[arg(short, long)]
    filename: String,

    /// The output file name
    #[arg(short, long)]
    output: String,
}

fn main() -> Result<(), Box<dyn Error>> {
    let args = Args::parse();

    let img = Image::load(args.filename)?;

    let dct_img = dct_2_image(&img);

    dct_img.save(args.output)?;

    Ok(())
}

/// Gets a new image that is derived from the DCT II coefficients of the supplied image.
pub fn dct_2_image(image: &Image) -> Image {
    let dct = image.dct_2();

    let max = dct
        .iter()
        .reduce(|acc, e| if e.abs() > acc.abs() { &e } else { &acc })
        .expect("This iterator should never be empty")
        .abs();

    let dct_scaled: Box<[u8]> = dct.iter().map(|x| (x / max * 255.0) as u8).collect();

    let mut dyn_img = DynamicImage::new(image.width(), image.height(), ColorType::Rgb8);

    for y in 0..image.height() {
        for x in 0..image.width() {
            let component = dct_scaled[(image.width() * y + x) as usize];

            dyn_img.put_pixel(x, y, Rgba([component, component, component, component]));
        }
    }

    Image::new(dyn_img)
}
