use dbus::{Connection,BusType,Props,Message};


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

        props.get("play")
            .expect("failed to get playing bool")
            .inner()
            .expect("failed to parse object into bool")
    }

    pub fn play(&mut self, p: bool) {

        let props = Props::new(&self.conn, NAME, "/player", NAME, 5000);

        props.set("play", p.into()).expect("failed");
    }

    pub fn shuffle(&mut self, p: bool) {

        let props = Props::new(&self.conn, NAME, "/player", NAME, 5000);

        props.set("shuffle", p.into()).expect("failed");
    }

    pub fn is_shuffled(&self) -> bool {

        let props = Props::new(&self.conn, NAME, "/player", NAME, 5000);

        props.get("shuffle")
            .expect("failed to get playing bool")
            .inner()
            .expect("failed to parse object into bool")
    }

    pub fn current(&mut self) -> String {

        let props = Props::new(&self.conn, NAME, "/player", NAME, 5000);

        props.get("current")
            .expect("failed to get current value")
            .inner::<&str>()
            .expect("failed to parse object into string")
            .to_string()
    }

    pub fn index(&mut self) -> u32 {

        let props = Props::new(&self.conn, NAME, "/player", NAME, 5000);

        props.get("index")
            .expect("failed to get current index")
            .inner()
            .expect("failed to parse object into integer")
    }

    pub fn remove(&mut self) {

        let m = Message::new_method_call(NAME, "/player", NAME, "remove").unwrap();
        self.conn.send_with_reply_and_block(m, 2000).unwrap();
    }

    pub fn undo(&mut self) {

        let m = Message::new_method_call(NAME, "/player", NAME, "undo").unwrap();
        self.conn.send_with_reply_and_block(m, 2000).unwrap();
    }

    pub fn next(&mut self) -> String {

        let m = Message::new_method_call(NAME, "/player", NAME, "next").unwrap();
        let r = self.conn.send_with_reply_and_block(m, 2000).unwrap();

        r.get1().unwrap()
    }

    pub fn prev(&mut self) -> String {

        let m = Message::new_method_call(NAME, "/player", NAME, "prev").unwrap();
        let r = self.conn.send_with_reply_and_block(m, 2000).unwrap();

        r.get1().unwrap()
    }

    pub fn set_list(&mut self, list: Vec<String>) {

        let m = Message::new_method_call(NAME, "/player", NAME, "set_list").unwrap()
            .append1(list);
        self.conn.send_with_reply_and_block(m, 2000).unwrap();
    }

    pub fn append_list(&mut self, list: Vec<String>) {

        let m = Message::new_method_call(NAME, "/player", NAME, "append_list").unwrap()
            .append1(list);
        self.conn.send_with_reply_and_block(m, 2000).unwrap();
    }
}
