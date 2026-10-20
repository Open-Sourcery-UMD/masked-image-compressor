use crate::math;
use error::ImageError;
use image::{ColorType, DynamicImage, GenericImage, GenericImageView, ImageReader, Rgba};
use std::error::Error;
use std::path::Path;

/// Contains errors that may occur when processing images.
pub mod error;

/// The Image struct is responsible for loading and processing images.
pub struct Image {
    img: DynamicImage,
}

impl Image {
    /// Creates a new image based on a path.
    pub fn load(filename: impl AsRef<Path>) -> Result<Image, Box<dyn Error>> {
        let img = ImageReader::open(filename)?.decode()?;

        let i = Image { img };

        // Maybe later we'll support non-square images
        match i.width() == i.height() {
            true => Ok(i),
            false => Err(Box::new(ImageError::NotSquareImage)),
        }
    }

    /// Returns width.
    pub fn width(&self) -> u32 {
        self.img.width()
    }

    /// Returns height.
    pub fn height(&self) -> u32 {
        self.img.height()
    }

    /// Returns the value of a pixel.
    pub fn pixel(&self, x: u32, y: u32) -> (u8, u8, u8, u8) {
        // I have no idea why there's an anonymous public struct field that does this.
        self.img.get_pixel(x, y).0.into()
    }

    /// Calculates the DCT Type 2 for this image and returns each pixel's coefficient in a vector
    /// of f64s.
    pub fn dct_2(&self) -> Vec<f64> {
        // This gets channel zero of the RGBA image converted on a scale from -1.0 to 1.0
        let pixel_f64 = |x: u32, y: u32| (self.pixel(x, y).0 as f64) / 255.0 * 2.0 - 1.0;

        let mut dct_input = vec![0.0; (self.width() * self.height()) as usize];

        for y in 0..self.height() {
            for x in 0..self.height() {
                dct_input[(self.width() * y + x) as usize] = pixel_f64(x, y);
            }
        }

        // Do the entire image (the image is square)
        math::dct_type_2(&dct_input, self.width())
    }

    /// Gets a new image that is derived from the DCT II coefficients of the supplied image.
    pub fn dct_2_image(&self) -> Self {
        let dct = self.dct_2();

        let max = dct
            .iter()
            .reduce(|acc, e| if e.abs() > acc.abs() { &e } else { &acc })
            .expect("This iterator should never be empty")
            .abs();

        let dct_scaled: Box<[u8]> = dct.iter().map(|x| (x / max * 255.0) as u8).collect();

        let mut dyn_img = DynamicImage::new(self.width(), self.height(), ColorType::Rgb8);

        for y in 0..self.height() {
            for x in 0..self.width() {
                let component = dct_scaled[(self.width() * y + x) as usize];

                dyn_img.put_pixel(x, y, Rgba([component, component, component, component]));
            }
        }

        Self { img: dyn_img }
    }

    /// Saves the image to a specified path and returns an error if unsuccessful.
    pub fn save(&self, filename: impl AsRef<Path>) -> Result<(), Box<dyn Error>> {
        self.img.save(filename)?;
        Ok(())
    }
}
