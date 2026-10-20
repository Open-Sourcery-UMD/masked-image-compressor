use libmicrs::image::Image;
use std::error::Error;
use clap::Parser;

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
    output: String
}

fn main() -> Result<(), Box<dyn Error>> {
    let args = Args::parse();

    let img = Image::load(args.filename)?;

    let dct_img = img.dct_2_image();

    dct_img.save(args.output)?;

    Ok(())
}
