use rand::{thread_rng, Rng};

use bgconfig::Config;
use parser::SiteParser;

#[derive(Clone, Default, Debug)]
pub struct Player {
    pub index: usize,
    shuffle: bool,
    play: bool,
    playlist: Vec<String>,
    history: Vec<(usize,usize,String)>,
    shuffled_playlist: Vec<String>,
    // Option so that we can derive Default
    index_changed: Option<fn(String)>,
    parsers: Vec<Box<SiteParser>>,
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
        }
    }

    pub fn initialize(&mut self, config: Config) {
        let images = self.expand_paths(config.uris());

        self.set_list(images);
        self.shuffle(config.shuffle());
    }

    pub fn new_callback(pl: Vec<String>, parsers: Vec<Box<SiteParser>>, f: fn(String)) -> Self {
        let mut player = Self::new(pl, parsers);
        player.index_changed = Some(f);

        player
    }

    fn expand_paths(&self, paths: Vec<String>) -> Vec<String> {
        paths.iter()
            .flat_map(|uri| {
                self.parsers.iter().find(|p| {
                    p.is_valid(uri)
                }).map_or(vec![], |p| {
                    p.parse(uri)
                })
            }).collect()
    }

    pub fn shuffle(&mut self, s: bool) {
        self.shuffle = s;
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
    }

    pub fn set_list(&mut self, list: Vec<String>) {
        self.history = vec![];
        self.playlist = list;
        self.index = 0;
        self.set_shuffle();
    }

    pub fn append_list(&mut self, list: Vec<String>) {

        self.playlist.extend_from_slice(&list);

        if self.shuffle {
            self.index = 0;
        }

        self.set_shuffle();
    }

    fn set_shuffle(&mut self) {
        // ThreadRng does not implement Default
        let mut rng = thread_rng();
        self.shuffled_playlist = self.playlist.clone();
        rng.shuffle(&mut self.shuffled_playlist);
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
