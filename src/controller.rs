use converse_derive::Converse;
use parapet::{ImageMode, Manager};

use crate::playlist::Playlist;
use crate::parsers::{Parser, file};
use crate::error::Error;

pub struct Controller {
    playlist: Playlist<String>,
    parser: Parser,
    wallpaper: Manager,
    mode: ImageMode,
}

impl Controller {
    pub fn new() -> Result<Self, Error> {
        let parser = Parser::new()
            .expander(file::DirectoryParser(false))
            .loader(file::FileParser);

        let wallpaper = Manager::new()?;

        Ok(Controller {
            playlist: Playlist::new(1000),
            parser: parser,
            wallpaper: wallpaper,
            mode: ImageMode::Max,
        })
    }
}

#[Converse(backgrounder)]
impl Controller {
    pub fn add(&mut self, item: &str) {
        let empty = self.playlist.len() == 0;

        /* if we cannot load the item, try expanding it */
        if self.parser.can_load(item) {
            self.playlist.push(item.to_string())
        } else {
            self.parser.expand(item.as_ref())
                .map(|x| self.playlist.extend(x));
        }

        if empty {
            self.next().map_err(|e| { eprintln!("{}", e); }).ok();
        }
    }

    pub fn remove(&mut self) {
        self.playlist.remove();
    }

    pub fn clear(&mut self) {
        self.playlist.clear();
    }

    pub fn undo(&mut self) {
        self.playlist.undo();
    }

    pub fn current(&mut self) -> Option<String> {
        self.playlist.current().cloned()
    }

    pub fn shuffle(&mut self, shuffle: bool) {
        self.playlist.shuffle(shuffle);
    }

    pub fn next(&mut self) -> Result<String, Error> {

        if self.playlist.len() == 0 {
            return Err(Error::EmptyPlaylist);
        }

        let item = self.playlist.next();
        let image: image::DynamicImage = self.parser.load(item)?;

        for disp in self.wallpaper.displays()? {
            disp.set(&image, self.mode.clone())?;
        }

        Ok(item.clone())
    }

    pub fn prev(&mut self) -> Result<String, Error> {

        if self.playlist.len() == 0 {
            return Err(Error::EmptyPlaylist);
        }

        let item = self.playlist.previous();
        let image: image::DynamicImage = self.parser.load(item)?;

        for disp in self.wallpaper.displays()? {
            disp.set(&image, self.mode.clone())?;
        }

        Ok(item.clone())
    }
}
