use dbus::{Connection,BusType,Props,Message};
use std::process::exit;

macro_rules! try_dbus {
    ( $get:expr ) => {
        match $get {
            Ok(x) => x,
            Err(e) => {
                println!("Failed to connect to dbus, is the server running?");
                println!("\t{}", e);
                exit(1);
            }
        }
    }
}

const NAME: &'static str = "com.backgrounder.dbus";

pub struct Player {
    conn: Connection,
}

impl Player {
    pub fn new() -> Self {
        let c = Connection::get_private(BusType::Session).unwrap();

        Player {
            conn: c,
        }
    }

    pub fn is_playing(&self) -> bool {

        let props = Props::new(&self.conn, NAME, "/player", NAME, 5000);

        try_dbus!(props.get("play"))
            .inner()
            .expect("failed to parse object into bool")
    }

    pub fn toggle_play(&mut self) {
        let playing = self.is_playing();
        self.play(!playing);
    }

    pub fn play(&mut self, p: bool) {

        let props = Props::new(&self.conn, NAME, "/player", NAME, 5000);

        try_dbus!(props.set("play", p.into()))
    }

    pub fn toggle_shuffle(&mut self) {
        let shuffled = self.is_shuffled();
        self.shuffle(!shuffled);
    }

    pub fn shuffle(&mut self, p: bool) {

        let props = Props::new(&self.conn, NAME, "/player", NAME, 5000);

        try_dbus!(props.set("shuffle", p.into()));
    }

    pub fn is_shuffled(&self) -> bool {

        let props = Props::new(&self.conn, NAME, "/player", NAME, 5000);

        try_dbus!(props.get("shuffle"))
            .inner()
            .expect("failed to parse object into bool")
    }

    pub fn current(&mut self) -> String {

        let props = Props::new(&self.conn, NAME, "/player", NAME, 5000);

        try_dbus!(props.get("current"))
            .inner::<&str>()
            .expect("failed to parse object into string")
            .to_string()
    }

    pub fn index(&mut self) -> u32 {

        let props = Props::new(&self.conn, NAME, "/player", NAME, 5000);

        try_dbus!(props.get("index"))
            .inner()
            .expect("failed to parse object into integer")
    }

    pub fn remove(&mut self) {

        let m = Message::new_method_call(NAME, "/player", NAME, "remove").unwrap();
        try_dbus!(self.conn.send_with_reply_and_block(m, 2000));
    }

    pub fn undo(&mut self) {

        let m = Message::new_method_call(NAME, "/player", NAME, "undo").unwrap();
        try_dbus!(self.conn.send_with_reply_and_block(m, 2000));
    }

    pub fn next(&mut self) -> String {

        let m = Message::new_method_call(NAME, "/player", NAME, "next").unwrap();
        let r = try_dbus!(self.conn.send_with_reply_and_block(m, 2000));

        r.get1().unwrap()
    }

    pub fn prev(&mut self) -> String {

        let m = Message::new_method_call(NAME, "/player", NAME, "prev").unwrap();
        let r = try_dbus!(self.conn.send_with_reply_and_block(m, 2000));

        r.get1().unwrap()
    }

    pub fn quit(&mut self) {

        let m = Message::new_method_call(NAME, "/player", NAME, "quit").unwrap();
        try_dbus!(self.conn.send_with_reply_and_block(m, 2000));
    }

    pub fn set_list(&mut self, list: Vec<String>) {

        let m = Message::new_method_call(NAME, "/player", NAME, "set_list").unwrap()
            .append1(list);
        try_dbus!(self.conn.send_with_reply_and_block(m, 2000));
    }

    pub fn append_list(&mut self, list: Vec<String>) {

        let m = Message::new_method_call(NAME, "/player", NAME, "append_list").unwrap()
            .append1(list);
        try_dbus!(self.conn.send_with_reply_and_block(m, 2000));
    }
}

