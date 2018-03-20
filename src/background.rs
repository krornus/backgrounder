use std::rc::Rc;
use std::cell::Cell;
use std::process::Command;
use std::path::Path;

use std::env;
use url::Url;

pub fn set_background(uri: String) {}

/*
pub struct Background { }
pub fn set_background(uri: String) {

    if !in_path("feh") {
        return;
    }

    // canonicalize can panic on characters like ~ ???
    // TODO: make wrapper
    if Path::new(uri).is_file() {

    } else if Url::parse(uri).is_ok() {
        match download_file(uri) {
            Ok(path) => set_background

        }
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
*/
