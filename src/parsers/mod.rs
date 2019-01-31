use image::DynamicImage;

use std::io::{Read};

use crate::error::Error;

pub mod file;

const MAGIC_LEN: usize = 32;

/* handles parser */
/* makes it really easy to have parsers interact with each other */
/* e.g. a dir is a loader, and it returns strings that can be dirs or files */
/*      where FileParser is a loader (loads an image) */
/*      and DirectoryParser is an expander (expands into more paths) */
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

    /* builder syntax because im a masochist and dealing with boxing is ugly */
    pub fn expander<E: Expander + 'static>(mut self, exp: E) -> Self {
        self.expanders.push(Box::new(exp));
        self
    }

    pub fn loader<P: Loader + 'static>(mut self, loader: P) -> Self {
        self.loaders.push(Box::new(loader));
        self
    }

    /* loads a path as an image */
    /* iterates loaders and uses the first one that works */
    /* is_valid isn't technically required, but helps eliminate options */
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

    /* recursively expand paths into more paths */
    /* control over the recursion is done in each expander instance */
    pub fn expand(&self, value: &str) -> Option<Vec<String>> {
        self.expanders.iter()
            /* get all valid expanders */
            .filter(|exp| {
                exp.is_valid(value).unwrap_or(false)
            })
            /* multiple expanders may return true for is_valid */
            /* we take the first one that is entirely successful */
            /* therefore find_map - stop searching when the option is Some() */
            .find_map(|exp| {
                /* try expanding */
                exp.expand(value).ok()
                    /* for each path - flat map its recursed expansion */
                    .map(|paths| { paths.into_iter()
                        .flat_map(|path| {
                             self.expand(&path)
                                /* if there is no expansion, don't discard the path */
                                .unwrap_or_else(|| {
                                    /* checking if we can load does entangle the code a bit */
                                    /* but saves massively on time where the user does something like */
                                    /* supply root as a path to add */
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
}

/* Expanders take a path and turn it into multiple paths */
/* e.g. directories, galleries */
pub trait Expander {
    fn is_valid(&self, path: &str) -> Result<bool, Error>;
    fn expand(&self, path: &str) -> Result<Vec<String>, Error>;
}

/* Loaders take a path and return an image */
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
