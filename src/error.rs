use xwallpaper;

#[derive(Debug)]
pub enum Error {
    XWallpaper(xwallpaper::Error),
    MissingFile,
}

impl From<xwallpaper::Error> for Error {
    fn from(err: xwallpaper::Error) -> Self {
        Error::XWallpaper(err)
    }
}
