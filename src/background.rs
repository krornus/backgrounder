use std::process::Command;
use std::fs::File;
use std::path::{Path,PathBuf};
use std::error::Error;
use std::env;
use std::io;

use url::Url;
use reqwest;


use parser;

const TMP_IMG: &str = "backgrounder.img";

#[derive(Clone, Debug)]
pub enum Mode {
    Center,
    Fill,
    Max,
    Scale,
    Tile,
}

impl ToString for Mode {
    fn to_string(&self) -> String {
        match self {
            &Mode::Center => "--bg-center".to_string(),
            &Mode::Fill=> "--bg-fill".to_string(),
            &Mode::Max=> "--bg-max".to_string(),
            &Mode::Scale=> "--bg-scale".to_string(),
            &Mode::Tile=> "--bg-tile".to_string(),
        }
    }
}

pub fn try_set_old(mode: Mode) {
    let tmp = env::temp_dir();
    let path = tmp.join(Path::new(TMP_IMG));

    if path.exists() {
        set_file(path.as_path(), mode);
    }
}

fn in_path(prog: &str) -> bool {
    if let Ok(path) = env::var("PATH") {
        path.split(":").any(|x| {
            Path::new(x).join(prog).exists()
        })
    } else {
        false
    }
}

pub fn set_background(uri: String, mode: Mode) {

    if !in_path("feh") {
        return;
    }

    // canonicalize can panic on characters like ~ ???
    // TODO: make wrapper
    let local = Path::new(&uri);
    if local.is_file() {
        set_file(local, mode);
    } else if Url::parse(&uri.clone()).is_ok() {
        match download_file(uri.clone()) {
            Ok(path) => set_file(path.as_path(), mode),
            Err(e) => {
                println!("error downloading file '{}'",uri);
                println!("\t{:?}",e);
            },
        }
    };
}


fn download_file(url: String) -> Result<PathBuf, parser::Error> {

    match reqwest::get(&url) {
        Ok(mut resp) => {
            let tmp = env::temp_dir();
            let path = tmp.join(Path::new(TMP_IMG));
            let mut file = File::create(&path)?;
            io::copy(&mut resp, &mut file)?;
            Ok(path.to_path_buf())
        },
        Err(e) => {
            println!("{:?}",e);
            println!("{:?}",e.status());
            Err(e.into())
        }
    }
}

fn set_file(path: &Path, mode: Mode) {
    match Command::new("feh")
        .arg(mode.to_string())
        .arg(path.to_str().unwrap())
        .spawn() {
            Ok(_) => {},
            Err(e) => {
                println!("failed to spawn feh, is it installed?");
                println!("\t{}", e.description());
            }
    }
}
