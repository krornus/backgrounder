use std::sync::Arc;
use std::rc::Rc;
use std::cell::{RefCell,Cell};

use dbus::{self,Path,NameFlag};
use dbus::tree::{self, MTFn, Access};
use time;

use bgconfig::Config;
use player::Player;
use background::{try_set_old,set_background};
use parser::SiteParser;
//use messenger::{Messenger,Server,Logger};

use imgur;
use fileparser;

const NAME: &'static str = "com.backgrounder.dbus";

#[derive(Debug)]
struct ServerInterface {
    path: Path<'static>,
    player: Rc<RefCell<Player>>,
}

impl ServerInterface {
    fn new(config: Config, parsers: Vec<Box<SiteParser>>) -> Self {

        let mut player = Player::new_callback(vec![], parsers, set_background);

        player.initialize(config);
        /* set old background while waiting */
        match try_set_old(player.mode()) {
            Ok(_) => {},
            Err(e) => {
                println!("Warning: Failed to set old background");
                println!("    {:?}", e);
            }
        }

        ServerInterface {
            path: "/player".into(),
            player: Rc::new(RefCell::new(
                player
            )),
        }
    }
}

#[derive(Clone, Default)]
struct TData;

impl tree::DataType for TData {
    type Tree = ();
    type ObjectPath = Arc<ServerInterface>;
    type Property = ();
    type Interface = ();
    type Method = ();
    type Signal = ();
}

fn create_tree(player: &Arc<ServerInterface>, done: Rc<Cell<bool>>) -> tree::Tree<MTFn<TData>, TData>  {

    let factory = tree::Factory::new_fn();

    let iface = factory.interface(NAME, ())
        .add_p(factory.property::<bool,_>("play",())
            .access(Access::ReadWrite)
            .on_get(|i,m| {

                let serv: &Arc<ServerInterface> = m.path.get_data();
                let playing = serv.player.borrow().is_playing();

                i.append(playing);
                Ok(())
            })
            .on_set(|i,m| {
                let serv: &Arc<ServerInterface> = m.path.get_data();
                let b = i.read::<bool>()?;

                serv.player.borrow_mut().play(b);

                Ok(())

            })
        )
        .add_p(factory.property::<&str,_>("current",())
            .access(Access::Read)
            .on_get(|i,m| {

                let serv: &Arc<ServerInterface> = m.path.get_data();
                let mut player = serv.player.borrow_mut();

                i.append(player.current());

                Ok(())
            })
        )
        .add_p(factory.property::<u32,_>("index",())
            .access(Access::Read)
            .on_get(|i,m| {

                let serv: &Arc<ServerInterface> = m.path.get_data();
                let index = serv.player.borrow().index as u32;

                i.append(index);
                Ok(())
            })
        )
        .add_p(factory.property::<u64,_>("interval",())
            .access(Access::ReadWrite)
            .on_get(|i,m| {

                let serv: &Arc<ServerInterface> = m.path.get_data();
                let inter = serv.player.borrow().interval;

                i.append(inter as u64);
                Ok(())
            })
            .on_set(|i,m| {
                let serv: &Arc<ServerInterface> = m.path.get_data();
                let inter = i.read::<u64>()?;

                serv.player.borrow_mut().interval(inter);

                Ok(())

            })
        )
        .add_p(factory.property::<bool,_>("shuffle",())
            .access(Access::ReadWrite)
            .on_get(|i,m| {

                let serv: &Arc<ServerInterface> = m.path.get_data();
                let shuffled = serv.player.borrow().is_shuffled();

                i.append(shuffled);
                Ok(())
            })
            .on_set(|i,m| {
                let serv: &Arc<ServerInterface> = m.path.get_data();
                let b = i.read::<bool>()?;

                serv.player.borrow_mut().shuffle(b);

                Ok(())

            })
        )
        .add_m(factory.method("ping", (), move |m| {
                Ok(vec![m.msg.method_return()])
            })
        )
        .add_m(factory.method("remove", (), move |m| {
                let serv: &Arc<ServerInterface> = m.path.get_data();
                let mut player = serv.player.borrow_mut();
                player.remove();

                Ok(vec![m.msg.method_return()])
            })
        )
        // TODO: make undo generic over enum instead of just removes
        .add_m(factory.method("undo", (), move |m| {
                let serv: &Arc<ServerInterface> = m.path.get_data();
                let mut player = serv.player.borrow_mut();
                player.undo_remove();

                Ok(vec![m.msg.method_return()])
            })
        )
        .add_m(factory.method("save", (), move |m| {
                let path: String = m.msg.read1()?;

                let serv: &Arc<ServerInterface> = m.path.get_data();
                let player = serv.player.borrow();
                player.save(&path);

                Ok(vec![m.msg.method_return()])
            }).inarg::<String,_>("path")
        )
        .add_m(factory.method("set_list", (), move |m| {
                let list: Vec<String> = m.msg.read1()?;

                let serv: &Arc<ServerInterface> = m.path.get_data();
                let mut player = serv.player.borrow_mut();
                player.set_list(list);

                Ok(vec![m.msg.method_return()])
            }).inarg::<Vec<String>,_>("list")
        )
        .add_m(factory.method("append_list", (), move |m| {

                let list: Vec<String> = m.msg.read1()?;

                let serv: &Arc<ServerInterface> = m.path.get_data();
                let mut player = serv.player.borrow_mut();
                player.append_list(list);

                Ok(vec![m.msg.method_return()])
            }).inarg::<Vec<String>,_>("list")
        )
        .add_m(factory.method("next", (), move |m| {
                let serv: &Arc<ServerInterface> = m.path.get_data();
                let mut player = serv.player.borrow_mut();

                Ok(vec![m.msg.method_return().append1(player.next())])
            }).outarg::<&str,_>("uri")
        )
        .add_m(factory.method("quit", (), move |m| {
                done.set(true);
                Ok(vec![m.msg.method_return()])
            })
        )
        .add_m(factory.method("prev", (), move |m| {
                let serv: &Arc<ServerInterface> = m.path.get_data();
                let mut player = serv.player.borrow_mut();

                Ok(vec![m.msg.method_return().append1(player.prev())])
            }).outarg::<&str,_>("uri")
        );

    factory.tree(())
        .add(factory.object_path(player.path.clone(), player.clone())
            .introspectable()
            .add(iface)
        )
}

pub fn run() {
    let done: Rc<Cell<bool>> = Default::default();
    let config = Config::load();
    let mut start = time::get_time().sec;

    //let mut logger = <Logger as Messenger<Server>>::new()
    //    .expect("failed to create logger");
    //logger.send("data")
    //    .expect("failed to log data");

    let imgur = imgur::ImgurParser::new();
    let fparser = fileparser::FileParser;

    //TODO: need retries when populating paths if no connection
    let parsers: Vec<Box<SiteParser>> = vec![
        Box::new(fparser),
        Box::new(imgur),
    ];

    let linter = Arc::new(ServerInterface::new(config, parsers));

    let tree = create_tree(&linter, done.clone());
    let conn = dbus::Connection::get_private(dbus::BusType::Session).unwrap();

    conn.register_name(NAME, NameFlag::ReplaceExisting as u32).unwrap();
    tree.set_registered(&conn, true).unwrap();

    conn.add_handler(tree);

    while !done.get() {
        conn.incoming(1000).next();

        let elapsed = (time::get_time().sec - start) as u64;
        let mut player = (&*linter).player.borrow_mut();

        if elapsed >= player.interval {
            player.next_if_play();
            start = time::get_time().sec;
        }
    }
}
