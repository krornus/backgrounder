use serde_derive::{Serialize, Deserialize};
use std::fmt;

#[derive(Serialize, Deserialize, Debug)]
pub enum Error {
    ServerError,
    InvalidImagePath,
    NoLoader,
    EmptyPlaylist,
    Image(String),
    IOError(String),
    Wallpaper(String),
    Converse(String),
}

impl From<image::ImageError> for Error {
    fn from(e: image::ImageError) -> Error {
        Error::Image(e.to_string())
    }
}

impl From<std::io::Error> for Error {
    fn from(e: std::io::Error) -> Error {
        Error::IOError(e.to_string())
    }
}

impl From<parapet::Error> for Error {
    fn from(e: parapet::Error) -> Error {
        Error::Wallpaper(e.to_string())
    }
}
impl From<converse::error::Error> for Error {
    fn from(e: converse::error::Error) -> Error {
        Error::Converse(e.to_string())
    }
}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match self {
            Error::Image(e) => write!(f, "{}", e),
            Error::IOError(e) => write!(f, "{}", e),
            Error::Wallpaper(e) => write!(f, "{}", e),
            Error::Converse(e) => write!(f, "{}", e),
            Error::InvalidImagePath => write!(f, "Attempt to parse invalid image path"),
            Error::NoLoader => write!(f, "No loader found for given image"),
            Error::EmptyPlaylist => write!(f, "No images are in the current playlist"),
            Error::ServerError => write!(f, "Internal server error"),
        }
    }
}
