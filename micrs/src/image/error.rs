use std::error::Error;
use std::fmt::{Display, Formatter};

/// The ImageError enum is used for indicating what errors we have while processing.
#[derive(Debug)]
pub enum ImageError {
    NotSquareImage,
}

/// Display implementation for ImageError.
impl Display for ImageError {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        write!(f, "{:?}", self)
    }
}

/// Marks ImageError as a valid error.
impl Error for ImageError {}
