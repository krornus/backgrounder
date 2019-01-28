use std::path::Path;
use std::fs::{self, File};

use image::DynamicImage;

use crate::parsers;
use crate::error::Error;
use crate::parsers::{Loader, Expander};

pub struct FileParser;
pub struct DirectoryParser(pub bool);

impl Loader for FileParser {
    fn load(&self, path: &str) -> Result<DynamicImage, Error> {
        Ok(image::open(path)?)
    }

    fn is_valid(&self, uri: &str) -> Result<bool, Error> {

        let path = Path::new(uri);

        if path.is_file() {
            let mut file =  File::open(path)?;
            Ok(parsers::check_magic(&mut file))
        } else {
            Ok(false)
        }
    }
}

impl Expander for DirectoryParser {
    fn expand(&self, path: &str) -> Result<Vec<String>, Error> {

        let path = Path::new(path);

        Ok(fs::read_dir(path).map(|res| {
            res.filter_map(|dir_res| {
                dir_res.ok().and_then(|ent| {

                    let path = ent.path();
                    let name = match path.to_str().map(String::from) {
                        Some(x) => x,
                        None => { return None; },
                    };

                    if path.is_dir() {
                        if self.0 {
                            Some(name)
                        } else {
                            None
                        }
                    } else {
                        Some(name)
                    }
                })
            }).collect()
        })?)
    }

    fn is_valid(&self, uri: &str) -> Result<bool, Error> {
        let path = Path::new(uri);
        Ok(path.is_dir())
    }
}
