use std::path::Path;
use std::fs::File;
use std::io::Read;

use image;

use parser::{SiteParser,Error};

const MAGIC_LEN: usize = 32;


#[derive(Debug,Clone)]
pub struct ListParser;

impl SiteParser for ListParser {
    fn parse(&self, uri: &str) -> Vec<String> {
        if self.is_valid(uri) {

            let path = Path::new(uri);
            let mut list = String::new();

            File::open(path).map(|mut file| {
                 if let Ok(_) = file.read_to_string(&mut list) {
                     list.lines().collect()
                 } else {
                     vec![]
                 }
            }).unwrap_or(vec![])

        } else {
            vec![]
        }
    }

    // TODO: add header to list file format?
    fn is_valid(&self, uri: &str) -> bool {
        let path = Path::new(uri);

        if path.is_file() {

            let mut f = match File::open(path) {
                Ok(f) => f,
                Err(_) => { return false; }
            };

            let mut buf: [u8; MAGIC_LEN] = [0; MAGIC_LEN];

            if let Err(_) = f.read_exact(&mut buf) {
                return false;
            }

            !image::guess_format(&buf).is_ok()

        } else if path.is_dir() {
            false
        } else {
            false
        }
    }
}
