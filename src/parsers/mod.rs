use image::DynamicImage;

use std::io::{Read};

use crate::error::Error;

pub mod file;

const MAGIC_LEN: usize = 32;

pub struct Parser {
    expanders: Vec<Box<Expander>>,
    loaders: Vec<Box<Loader>>,
}

impl Parser {
    pub fn new() -> Self {
        Parser {
            expanders: vec![],
            loaders: vec![],
        }
    }

    pub fn expander<E: Expander + 'static>(mut self, exp: E) -> Self {
        self.expanders.push(Box::new(exp));
        self
    }

    pub fn loader<P: Loader + 'static>(mut self, loader: P) -> Self {
        self.loaders.push(Box::new(loader));
        self
    }

    pub fn load(&self, value: &str) -> Result<DynamicImage, Error> {
        self.loaders.iter()
            .filter(|loader| {
                loader.is_valid(value).unwrap_or(false)
            })
            .map(|loader| {
                loader.load(value)
            })
            .find(|img| {
                 img.is_ok()
            })
            .unwrap_or(Err(Error::NoLoader))
    }

    /* recursively expand items */
    pub fn expand(&self, value: &str) -> Option<Vec<String>> {
        self.expanders.iter()
            .filter(|exp| {
                exp.is_valid(value).unwrap_or(false)
            })
            /* multiple expanders may return true for is_valid */
            /* we take the first one that is entirely successful */
            .find_map(|exp| {
                exp.expand(value).ok()
                    .map(|paths| { paths.into_iter()
                        .flat_map(|path| {
                             self.expand(&path)
                                /* if there is no expansion, don't discard */
                                .unwrap_or_else(|| {
                                    if self.can_load(&path) {
                                        vec![path]
                                    } else {
                                        vec![]
                                    }
                                }).into_iter()
                        }).collect()
                    })
            })
    }

    pub fn can_load(&self, value: &str) -> bool {
        self.loaders.iter()
            .any(|loader| loader.is_valid(value).unwrap_or(false))
    }

    pub fn can_expand(&self, value: &str) -> bool {
        self.loaders.iter()
            .any(|loader| loader.is_valid(value).unwrap_or(false))
    }
}

pub trait Expander {
    fn is_valid(&self, path: &str) -> Result<bool, Error>;
    fn expand(&self, path: &str) -> Result<Vec<String>, Error>;
}

pub trait Loader {
    fn is_valid(&self, path: &str) -> Result<bool, Error>;
    fn load(&self, path: &str) -> Result<DynamicImage, Error>;
}

/* check for a valid image format without loading the entire image into memory */
pub fn check_magic<R: Read>(stream: &mut R) -> bool {

    let mut buf = vec![0; MAGIC_LEN];

    if stream.read(&mut buf[..]).is_err() {
        false
    } else {
        image::guess_format(&buf[..])
            .is_ok()
    }
}
