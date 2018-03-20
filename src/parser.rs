use std::fmt::Debug;
use std::io;
use reqwest;

pub trait SiteParser: Debug + SiteParserClone {
    fn parse(&self, uri: &str) -> Vec<String>;
    fn is_valid(&self, uri: &str) -> bool;
}

/* Have to do this stupid hack to make clone work for stored trait */
/* https://stackoverflow.com/questions/30353462/how-to-clone-a-struct-storing-a-trait-object */
pub trait SiteParserClone {
    fn boxed_clone(&self) -> Box<SiteParser>;
}

impl<T> SiteParserClone for T where T: 'static + SiteParser + Clone {
    fn boxed_clone(&self) -> Box<SiteParser> {
        Box::new(self.clone())
    }
}

impl Clone for Box<SiteParser> {
    fn clone(&self) -> Box<SiteParser> {
        self.boxed_clone()
    }
}
/* end stupid hack */


#[derive(Debug)]
pub enum Error {
    Request(reqwest::Error),
    IO(io::Error),
    Route(RouteError),
}

#[derive(Debug)]
pub enum RouteError {
    InvalidUrl,
    InvalidGallery,
}

impl From<reqwest::Error> for Error {
    fn from(e: reqwest::Error) -> Self {
        Error::Request(e)
    }
}


impl From<io::Error> for Error {
    fn from(e: io::Error) -> Self {
        Error::IO(e)
    }
}
