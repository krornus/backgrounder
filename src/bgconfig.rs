use std::env;
use std::path::{Path,PathBuf};

use config;
use xwallpaper::render::Mode;

const CONFIG: &str = ".bgrc";

pub enum Error {
    Parser(config::error::ConfigError),
}

impl From<config::error::ConfigError> for Error {
    fn from(e: config::error::ConfigError)-> Self {
        Error::Parser(e)
    }
}

pub struct Config {
    settings: Option<config::types::Config>,
}

impl Config {
    fn path() -> Option<PathBuf> {
        let home = if let Some(home) = env::home_dir() {
            let path = home.join(CONFIG);

            if path.exists() {
                Some(path)
            } else {
                None
            }
        } else {
            None
        };

        if let Some(path) = home {
            Some(path)
        } else {
            let path = Path::new("/etc").join(CONFIG);
            if path.exists() {
                Some(path)
            } else {
                None
            }
        }
    }

    pub fn load() -> Self {

        if let Some(path) = Self::path() {
            let parsed = config::reader::from_file(&path)
                .expect("failed to parse config");

            Config {
                settings: Some(parsed),
            }
        } else {
            Config {
                settings: None,
            }
        }
    }

    pub fn mode(&self) -> Mode {
        if let Some(ref settings) = self.settings {
            match settings.lookup_str_or("mode", "max") {
                "center" => Mode::Center,
                "fill" => Mode::Fill,
                "max" => Mode::Max,
                "scale" => Mode::Scale,
                _ => Mode::Max,
            }
        } else {
            Mode::Max
        }
    }

    pub fn interval(&self) -> u64 {
        let default = 120;

        if let Some(ref settings) = self.settings {
            settings.lookup_integer64_or("interval", default) as u64
        } else {
            default as u64
        }
    }

    pub fn shuffle(&self) -> bool {
        let default = false;

        if let Some(ref settings) = self.settings {
            settings.lookup_boolean_or("shuffle", default)
        } else {
            default
        }
    }

    pub fn play(&self) -> bool {
        let default = true;
        if let Some(ref settings) = self.settings {
            settings.lookup_boolean_or("play", default)
        } else {
            default
        }
    }

    pub fn uris(&self) -> Vec<String> {

        let settings = match self.settings {
            Some(ref s) => s,
            None => { return vec![]; },
        };

        let value = match settings.lookup("path") {
            Some(v) => v,
            None => { return vec![]; },
        };

        let iterable = {
            use config::types::Value::*;

            match value {
                &List(ref v) => Some(v.clone()),
                &Array(ref v) => Some(v.clone()),
                _ => None
            }
        };

        if let Some(values) = iterable {
            values.into_iter()
                .filter_map(Self::value_to_string)
                .collect()
        } else {
            vec![settings.lookup_str("path").unwrap().to_string()]
        }
    }

    fn value_to_string(value: &config::types::Value) -> Option<String> {
        use config::types::Value::*;
        use config::types::ScalarValue::*;
        match value {
            &Svalue(ref v) => match v {
                &Str(ref s) => Some(s.clone()),
                _ => None,
            }
            _ => None
        }
    }
}
