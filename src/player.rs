use std::fs::File;
use std::io::prelude::*;

use rand::{thread_rng, Rng, ThreadRng};

use bgconfig::Config;
use xwallpaper::render::Mode;
use parser::SiteParser;

#[derive(Clone, Debug)]
pub struct Player {
    pub index: usize,
    pub interval: u64,
    shuffle: bool,
    play: bool,
    playlist: Vec<String>,
    history: Vec<(usize,usize,String)>,
    shuffled_playlist: Vec<String>,
    // Option so that we can derive Default
    index_changed: Option<fn(String, Mode)>,
    parsers: Vec<Box<SiteParser>>,
    mode: Mode,
    rng: ThreadRng,
}


impl Player {
    pub fn new(pl: Vec<String>, parsers: Vec<Box<SiteParser>>) -> Self {

        let mut rng = thread_rng();
        let mut pl_shuffle = pl.clone();

        rng.shuffle(&mut pl_shuffle);

        Player {
            index: 0,
            shuffle: false,
            play: false,
            playlist: pl,
            shuffled_playlist: pl_shuffle,
            history: vec![],
            index_changed: None,
            parsers: parsers,
            mode: Mode::Max,
            rng: rng,
            interval: 120,
        }
    }

    pub fn mode(&self) -> Mode {
        self.mode.clone()
    }

    pub fn initialize(&mut self, config: Config) {
        self.mode = config.mode();
        let images = self.expand_paths(config.uris());

        self.history = vec![];
        self.playlist = images;
        self.index = 0;
        self.interval = config.interval();

        self.init_shuffled_list();
        self.shuffle(config.shuffle());
        self.play(config.play());
        self.callback();
    }

    pub fn new_callback(pl: Vec<String>, parsers: Vec<Box<SiteParser>>, f: fn(String, Mode)) -> Self {
        let mut player = Self::new(pl, parsers);
        player.index_changed = Some(f);

        player
    }

    pub fn save(&self, uri: &str) {
        if let Ok(mut f) = File::create(uri) {
            for path in self.playlist.iter() {
                if let Err(_) = write!(f, "{}\n", path) {
                    break;
                }
            }
        }
    }

    // Want to add a parser that can load a file of sub-paths
    fn expand_paths(&self, paths: Vec<String>) -> Vec<String> {
        paths.iter()
            .flat_map(|uri| {
                self.expand_all_path(&uri)
            }).collect()
    }

    // Pass in path, get recursively expanded path
    fn expand_all_path(&self, path: &str) -> Vec<String> {
        if let Some(v) = self.expand_path(path) {
            if v.len() > 1 {
                v.iter()
                    .flat_map(|uri| {
                        self.expand_all_path(&uri)
                    }).collect()
            } else {
                v
            }
        } else {
            vec![path.to_string()]
        }
    }

    fn expand_path(&self, path: &str) -> Option<Vec<String>> {
        self.parsers.iter().find(|p| {
            p.is_valid(path)
        }).map_or(None, |p| {
            Some(p.parse(path))
        })
    }

    pub fn shuffle(&mut self, s: bool) {
        self.shuffle = s;
    }

    pub fn interval(&mut self, i: u64) {
        self.interval = i;
    }

    pub fn is_shuffled(&self) -> bool {
        self.shuffle
    }

    pub fn play(&mut self, p: bool) {
        self.play = p;
    }

    pub fn is_playing(&self) -> bool {
        self.play
    }

    pub fn next(&mut self) -> &str {
        if self.playlist.len() == 0 {
            return "";
        }

        self.index = (self.index + 1) % self.playlist.len();
        println!("Next: {}/{}", self.index, self.playlist.len());

        self.callback();
        self.current()
    }

    pub fn prev(&mut self) -> &str {
        if self.playlist.len() == 0 {
            return "";
        }

        if self.index == 0 {
            self.index = self.playlist.len() - 1;
        } else {
            self.index = (self.index - 1) % self.playlist.len();
        }

        println!("Prev: {}/{}", self.index, self.playlist.len());

        self.callback();
        self.current()
    }

    pub fn current(&mut self) -> &str {

        self.cleanse_index();

        if self.playlist.len() == 0 {
            ""
        } else if self.shuffle {
            &self.shuffled_playlist[self.index]
        } else {
            &self.playlist[self.index]
        }
    }

    pub fn remove(&mut self) {

        self.cleanse_index();

        let item = self.playlist.remove(self.index);
        let idx = self.shuffled_playlist.iter().position(|x| {
            x == &item
        }).expect("shuffled and standard playlists differ");

        self.shuffled_playlist.remove(idx);
        self.history.push((self.index, idx, item));

        self.cleanse_index();
        self.callback();
    }

    pub fn undo_remove(&mut self) {
        if let Some((idx, shuf_idx, val)) = self.history.pop() {
            if idx > self.playlist.len() || shuf_idx > self.playlist.len() {
                panic!("invalid history")
            }

            self.playlist.insert(idx, val.clone());
            self.shuffled_playlist.insert(shuf_idx, val);
        } else {
            println!("skipping undo");
        }

        self.callback();
    }

    pub fn set_list(&mut self, list: Vec<String>) {
        let images = self.expand_paths(list);

        self.history = vec![];
        self.playlist = images;
        self.index = 0;
        self.init_shuffled_list();

        self.callback();
    }

    pub fn next_if_play(&mut self) {
        if self.play {
            self.next();
        }
    }

    pub fn append_list(&mut self, list: Vec<String>) {
        let images = self.expand_paths(list);

        self.playlist.extend_from_slice(&images);

        if self.shuffle {
            self.index = 0;
        }

        self.init_shuffled_list();
    }

    fn callback(&mut self) {

        let img = self.current().to_string();
        let mode = self.mode.clone();

        println!("Callback({:?},{:?})",img, mode);

        if let Some(f) = self.index_changed {
            f(img, mode);
        }
    }

    fn init_shuffled_list(&mut self) {
        self.shuffled_playlist = self.playlist.clone();
        self.rng.shuffle(&mut self.shuffled_playlist);
    }

    fn cleanse_index(&mut self) {
        if self.shuffled_playlist.len() != self.playlist.len() {
            panic!("playlists are out of sync!");
        }

        if self.index > self.playlist.len() {
            self.index = self.index % self.playlist.len();
        }
    }
}
