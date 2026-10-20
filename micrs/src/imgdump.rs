use image::{GenericImageView, ImageReader};
use std::error::Error;
use clap::Parser;

#[derive(Parser, Debug)]
#[command(version, about, long_about = "Dumps a list of pixels found in an image.")]
struct Args {
    #[arg(short, long)]
    /// The name of the file to dump the pixels of.
    filename: String
}

fn main() -> Result<(), Box<dyn Error>> {
    let args = Args::parse();

    let img = ImageReader::open(args.filename)?.decode()?;

    for y in 0..img.height() {
        for x in 0..img.width() {
            let pixel = img.get_pixel(x, y).0;
            println!("{:?} -> {:?}", (x, y), pixel);
        }
    }

    Ok(())
}
