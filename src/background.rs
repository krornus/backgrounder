use std::fs::File;
use std::path::{Path,PathBuf};
use std::env;
use std::io;

use url::Url;
use xwallpaper::{self, render::Mode};
use reqwest;

use error;
use parser;

const TMP_IMG: &str = "backgrounder.img";

pub fn try_set_old(mode: Mode) -> Result<(), error::Error> {
    let tmp = env::temp_dir();
    let path = tmp.join(Path::new(TMP_IMG));

    if path.exists() {
        Ok(xwallpaper::set_wallpaper(path.as_path(), mode)?)
    } else {
        Err(error::Error::MissingFile)
    }
}

pub fn set_background(uri: String, mode: Mode) {

    let local = Path::new(&uri);

    if local.is_file() {
        xwallpaper::set_wallpaper(local, mode).expect("failed to set background");
    } else if Url::parse(&uri.clone()).is_ok() {
        match download_file(uri.clone()) {
            Ok(path) => xwallpaper::set_wallpaper(path, mode).expect("failed to set background"),
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
