use clap::{Arg, App, SubCommand};

use std::thread;
use std::time::Duration;

mod error;
mod worker;
mod parsers;
mod playlist;
mod controller;

use crate::controller::Controller;
use crate::error::Error;
use crate::worker::FallibleThread;

fn args<'a, 'b>() -> App<'a, 'b> {
    App::new("backgrounder")
        .author("Spencer Powell")
        .version("0.4")
        .about("Desktop wallpaper manager")
        .subcommand(SubCommand::with_name("server")
            .about("start the server")
            .arg(Arg::with_name("no-daemon")
                .short("d")
                .long("no-daemon")
                .help("Run the server without detaching from terminal")))
        .subcommand(SubCommand::with_name("remove")
            .about("remove current wallpaper from playlist"))
        .subcommand(SubCommand::with_name("undo")
            .about("undo add/remove action"))
        .subcommand(SubCommand::with_name("current")
            .about("get current wallpaper"))
        .subcommand(SubCommand::with_name("shuffle")
            .about("set shuffle on/off for wallpapers")
            .arg(Arg::with_name("state")
                .takes_value(true)
                .default_value("true")
                .possible_values(&["on", "true", "off", "false"])))
        .subcommand(SubCommand::with_name("next")
            .about("next wallpaper"))
        .subcommand(SubCommand::with_name("prev")
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
        ("server", _) => {
            Some(server())
        },
        _ => None,
    };

    let mut controller = Controller::client();

    /* see TODO */
    /* poll the filesystem for the socket/lockfile */
    while controller.is_err() {

        let err = server.as_ref()
            .map(|x| x.is_err());

        if err  == Some(true) {
            eprintln!("server failed");
            return Err(Error::ServerError);
        }

        eprintln!("failed to establish server connection - retrying");
        match controller {
            Err(e) => println!("    {}", e),
            _ => {}
        }

        controller = Controller::client();
        thread::sleep(Duration::from_secs(2));
    }

    let mut controller = controller.unwrap();

    /* add paths first */
    if let Some(values) = matches.values_of("path") {
        for path in values {
            controller.add(path)?;
        }
    }

    /* we can now call next and it will include the just given paths */
    /* each call returns result on IPC and result on parapet etc...  */
    /* thus the double ?? */
    match matches.subcommand() {
        ("remove", _) => { controller.remove()?; },
        ("undo", _) => { controller.undo()?; },
        ("current", _) => { controller.current()?; },
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

fn server() -> FallibleThread<(), Error> {

    FallibleThread::spawn(|| {
        Controller::new()?
            .server()?
            .run()?;

        Ok(())
    })
}
