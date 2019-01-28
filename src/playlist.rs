use rand::Rng;
use rand::seq::index;
use rand::rngs::ThreadRng;

use std::ops::Index;

trait Expand<T> {
    fn expand(items: Self) -> Vec<T>;
}

#[derive(Debug)]
struct RemoveItem<T> {
    ordered: usize,
    random:  usize,
    state:   usize,
    item:    T,
}

impl<T> RemoveItem<T> {
    fn new(ordered: usize, random: usize, state: usize, item: T) -> Self {
        RemoveItem {
            ordered,
            random,
            state,
            item
        }
    }
}

#[derive(Debug)]
enum HistoryItem<T> {
    Add(usize),
    AddAll(Vec<usize>),
    Remove(RemoveItem<T>),
    RemoveAll(Vec<RemoveItem<T>>),
    Undo(Box<HistoryItem<T>>),
}

impl<T> HistoryItem<T> {
    fn as_remove(&self) -> &RemoveItem<T> {
        match self {
            HistoryItem::Remove(ref remove) => remove,
            _ => panic!("Invalid HistoryItem::Remove access"),
        }
    }

    fn into_add(self) -> usize {
        match self {
            HistoryItem::Add(add) => add,
            _ => panic!("Invalid HistoryItem::Add access"),
        }
    }

    fn into_remove(self) -> RemoveItem<T> {
        match self {
            HistoryItem::Remove(remove) => remove,
            _ => panic!("Invalid HistoryItem::Remove access"),
        }
    }

    fn into_undo(self) -> HistoryItem<T> {
        match self {
            HistoryItem::Undo(undo) => *undo,
            _ => panic!("Invalid HistoryItem::Undo access"),
        }
    }
}

#[derive(Debug)]
/* XXX: REMOVE PUB */
pub struct ShuffleList<T> {
    pub ordered: Vec<T>,
    random:  Vec<usize>,
    shuffle: bool,
    state:   usize,
    rng: ThreadRng,
}

impl<T> ShuffleList<T> {
    fn new() -> Self {
        ShuffleList {
            ordered: vec![],
            random:  vec![],
            shuffle: false,
            state:   0,
            rng: rand::thread_rng(),
        }
    }

    fn push(&mut self, item: T) -> HistoryItem<T> {

        let ordered = self.len();
        let random  = self.random_index();

        self.insert(ordered, random, item);

        HistoryItem::Add(self.ordered.len() - 1)
    }

    fn extend(&mut self, items: Vec<T>) -> HistoryItem<T> {
        let mut ext = Vec::with_capacity(items.len());
        for item in items {
            self.push(item);
            ext.push(self.ordered.len() - 1);
        }

        HistoryItem::AddAll(ext)
    }

    fn insert(&mut self, ordered: usize, random: usize, item: T) -> HistoryItem<T> {

        if self.shuffle {
            /* the mappings which are after the inserted items are now off by one */
            self.random.iter_mut()
                .filter(|x| **x >= ordered)
                .for_each(|x| *x += 1);
        }

        self.ordered.insert(ordered, item);
        self.random.insert(random, ordered);

        HistoryItem::Add(ordered)
    }

    /* remove - map index if shuffle is true */
    fn remove(&mut self, index: usize) -> HistoryItem<T> {

        let index = if self.shuffle { self.random[index] } else { index };

        self.remove_ordered(index)
    }

    /* remove - dont map index if shuffle is true */
    fn remove_ordered(&mut self, index: usize) -> HistoryItem<T> {

        let item = self.ordered.remove(index);
        let mut rdx = None;

        for (i,x) in self.random.iter_mut().enumerate() {
            if *x > index {
                *x -= 1;
            } else if *x == index {
                rdx = Some(i);
            }
        }

        let rdx = rdx.expect("Shuffle and standard playlist mismatch");

        self.random.remove(rdx);

        HistoryItem::Remove(RemoveItem::new(index, rdx, self.state, item))
    }

    fn len(&self) -> usize {
        self.ordered.len()
    }

    fn shuffle(&mut self, shuffle: bool) {

        if !self.shuffle && shuffle {
            self.reshuffle();
        }

        self.shuffle = shuffle;
    }

    fn reshuffle(&mut self) {
        let len = self.len();
        self.random = index::sample(&mut self.rng, len, len)
            .into_vec();
        self.state += 1;
    }

    fn random_index(&mut self) -> usize {
        if self.len() == 0 {
            0
        } else {
            self.rng.gen_range(0, self.len())
        }
    }


    fn undo(&mut self, item: HistoryItem<T>) -> HistoryItem<T> {
        match item {
            HistoryItem::Add(ordered) => self.undo_add(ordered),
            HistoryItem::AddAll(all) => self.undo_add_all(all),
            HistoryItem::Remove(remove) => self.undo_remove(remove),
            HistoryItem::RemoveAll(all) => self.undo_remove_all(all),
            HistoryItem::Undo(_) => panic!("ShuffleList::undo(HistoryItem::Undo) is unimplemented"),
        }
    }

    fn undo_add(&mut self, i: usize) -> HistoryItem<T> {
        /* dont use remove() - history items are absolute ordered values */
        HistoryItem::Undo(Box::new(self.remove_ordered(i)))
    }

    fn undo_add_all(&mut self, mut all: Vec<usize>) -> HistoryItem<T> {
        let mut removes = Vec::with_capacity(all.len());
        /* sort so that subtraction works */
        all.sort_unstable();

        for (x, idx) in all.into_iter().enumerate() {
            /* dont use remove() - history items are absolute ordered values */
            removes.push(self.remove_ordered(idx - x).into_remove())
        }

        HistoryItem::Undo(Box::new(HistoryItem::RemoveAll(removes)))
    }

    fn undo_remove(&mut self, remove: RemoveItem<T>) -> HistoryItem<T> {
        /*
         * insert at old position or end if the old position
         * is past the end of the list
         */
        fn clamp_index(i: usize, len: usize) -> usize {
            if i >= len {
                len
            } else {
                i
            }
        }

        let ordered = clamp_index(remove.ordered, self.len());
        /* get a new random index if our state changed */
        let random = if self.state == remove.state {
            clamp_index(remove.random, self.len())
        } else {
            self.random_index()
        };

        HistoryItem::Undo(Box::new(self.insert(ordered, random, remove.item)))
    }

    fn undo_remove_all(&mut self, all: Vec<RemoveItem<T>>) -> HistoryItem<T> {
        let mut adds = Vec::with_capacity(all.len());
        for rem in all {
            adds.push(self.undo_remove(rem).into_undo().into_add())
        }

        HistoryItem::Undo(Box::new(HistoryItem::AddAll(adds)))
    }
}

impl<T> Index<usize> for ShuffleList<T> {
    type Output = T;

    fn index(&self, i: usize) -> &T {
        if self.shuffle {
            &self.ordered[self.random[i]]
        } else {
            &self.ordered[i]
        }
    }
}

#[derive(Debug)]
pub struct Playlist<T> {
    //XXX REMOVE PUB
    pub items: ShuffleList<T>,
    history: Vec<HistoryItem<T>>,
    idx: usize,
}

impl<T> Playlist<T> {
    pub fn new() -> Self {
        Playlist {
            items: ShuffleList::new(),
            history: vec![],
            idx: 0,
        }
    }
    pub fn push(&mut self, item: T) {
        let hist = self.items.push(item);
        self.history.push(hist);
    }

    pub fn extend(&mut self, items: Vec<T>) {
        let hist = self.items.extend(items);
        self.history.push(hist);
    }

    pub fn remove(&mut self) -> Option<&T> {

        if self.len() != 0 {
            let hist = self.items.remove(self.idx);

            if self.idx == self.len() {
                self.idx = self.idx.saturating_sub(1);
            }

            self.history.push(hist);

            let remove = self.history[self.history.len() - 1].as_remove();

            Some(&remove.item)
        } else {
            None
        }
    }

    pub fn undo(&mut self) {
        let _undo = self.history.pop()
            .map(|x| {
                self.items.undo(x)
            });
    }

    /* these functions panic - check length outside */
    pub fn next(&mut self) -> &T {
        if self.len() != 0 {
            self.idx = (self.idx + 1) % self.len();
        }

        &self.items[self.idx]
    }

    /* these functions panic - check length outside */
    pub fn previous(&mut self) -> &T {
        if self.len() != 0 {
            self.idx = if self.idx == 0 {
                self.len() - 1
            } else {
                self.idx - 1
            }
        }

        &self.items[self.idx]
    }

    pub fn current(&mut self) -> Option<&T> {
        if self.len() != 0 {
            Some(&self.items[self.idx])
        } else {
            None
        }
    }

    pub fn len(&self) -> usize {
        self.items.len()
    }

    fn update(&mut self) {
        if self.idx >= self.len() {
            self.idx = self.len().saturating_sub(1);
        }
    }
}

impl<T> Playlist<T> {
    pub fn shuffle(&mut self, shuffle: bool) {
        self.items.shuffle(shuffle)
    }

    pub fn index(&mut self, i: usize) -> &T {
        /* panics if i is out of bounds */
        self.idx = i;
        &self.items[i]
    }
}
