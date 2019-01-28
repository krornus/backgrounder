use clap::{Arg, App, SubCommand};

use std::thread;

mod error;
mod parsers;
mod playlist;
mod controller;

use crate::controller::Controller;
use crate::error::Error;

/* TODO: error.rs: better way of deriving serde? */
/*  currently errors hold to_strings so they can be serialized */

/* TODO: playlist.rs: should next/prev panic? (probably) */

/* TODO: main.rs: poll for the server lock support in converse */

fn args<'a, 'b>() -> App<'a, 'b> {
    App::new("backgrounder")
        .author("Spencer Powell")
        .version("0.4")
        .about("Desktop wallpaper manager")
        .subcommand(SubCommand::with_name("server")
            .about("start the server"))
            .arg(Arg::with_name("no-daemon")
                .short("d")
                .long("no-daemon")
                .help("Run the server without detaching from terminal"))
        .subcommand(SubCommand::with_name("next")
            .about("next wallpaper"))
        .subcommand(SubCommand::with_name("prev")
            .about("previous wallpaper"))
        .subcommand(SubCommand::with_name("shuffle")
            .about("previous wallpaper"))
        .arg(Arg::with_name("path")
            .help("path to a wallpaper")
            .multiple(true))
}


fn main() -> Result<(), Error> {

    let matches = args().get_matches();

    /* spawn server thread
         allows us to process commands within a server command */
    let server = match matches.subcommand() {
        ("server", _) => Some(server()),
        _ => None,
    };

    let mut controller = Controller::client();
    /* poll the filesystem for the socket/lockfile */
    /* see TODO at top */
    while controller.is_err() { controller = Controller::client() }
    let mut controller = controller.unwrap();

    /* add paths first */
    matches.values_of("path")
        .map(|values| {
            values.for_each(|x| {
                controller.add(x);
            })
        });

    /* we can now call next and it will include the just given paths */
    /* each call returns result on IPC and result on parapet etc...  */
    /* thus the double ?? */
    match matches.subcommand() {
        ("next", _) => { controller.next()??; },
        ("prev", _) => { controller.prev()??; },
        _ => {},
    }

    match server {
        /* be kind */
        Some(handle) => handle.join().unwrap(),
        _ => Ok(())
    }
}

fn server() -> thread::JoinHandle<Result<(), Error>> {
    thread::spawn(|| {
        Controller::new()?.server()?.run();
        Ok(())
    })
}
