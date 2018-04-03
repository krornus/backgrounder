use std::path::Path;
use std::fs::File;
use std::io::Read;

use image;

use parser::{SiteParser,Error};


#[derive(Debug,Clone)]
pub struct FileParser;

const MAGIC_LEN: usize = 32;

impl SiteParser for FileParser {
    fn parse(&self, uri: &str) -> Vec<String> {

        let path = Path::new(uri);

        if path.is_file() && self.is_valid(uri) {
            vec![uri.to_string()]
        } else if path.is_dir() {
            match self.parse_dir(path) {
                Ok(list) => list,
                Err(_) => vec![],
            }
        } else {
            vec![]
        }
    }

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

            image::guess_format(&buf).is_ok()

        } else if path.is_dir() {
            true
        } else {
            false
        }
    }
}

impl FileParser {
    fn parse_dir(&self, path: &Path) -> Result<Vec<String>, Error> {
        let entries = path.read_dir()?;

        Ok(entries.flat_map(|x| {
                if let Ok(entry) = x {
                    if let Some(s) = entry.path().to_str() {
                        self.parse(s)
                    } else {
                        vec![]
                    }
                } else {
                    vec![]
                }
            }).collect())
    }
}

