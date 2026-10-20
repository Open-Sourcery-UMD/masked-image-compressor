use libmicrs::image::Image;
use std::error::Error;
use clap::Parser;

#[derive(Parser, Debug)]
#[command(version, about, long_about = "Dumps a list of pixels transformed by the type II DCT.")]
struct Args {
    #[arg(short, long)]
    /// The name of the file to dump the coefficients of.
    filename: String
}

fn main() -> Result<(), Box<dyn Error>> {
    let args = Args::parse();

    let img = Image::load(args.filename)?;

    let dct = img.dct_2();

    let get_dct = |x, y| dct[(y * img.width() + x) as usize];

    for y in 0..img.height() {
        for x in 0..img.width() {
            println!("{:?} -> {:?}", (x, y), get_dct(x, y));
        }
    }

    Ok(())
}
