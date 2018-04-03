#[macro_use]
extern crate serde_derive;
#[macro_use]
extern crate clap;
extern crate dbus;
extern crate image;
extern crate time;
extern crate rand;
extern crate regex;
extern crate url;
extern crate reqwest;
extern crate kuchiki;
extern crate serde;
extern crate serde_json;
extern crate config;
extern crate daemonize;
extern crate nanomsg;

mod client;
mod server;
mod player;
mod tests;
mod imgur;
mod bgconfig;
mod parser;
mod background;
mod fileparser;
mod messenger;

use std::process::exit;
use std::error::Error;
use std::path::Path;
use std::env;
use std::fs;

use clap::{App,ArgMatches,Shell};
use daemonize::{Daemonize};

use messenger::{Messenger,Client,Logger};

fn main() {

    let yaml = load_yaml!("./args.yaml");
    let app = App::from_yaml(yaml);

    gen_completions(app.clone());
    let matches = app.get_matches();

    let client = client::Player::new();

    let server = matches.is_present("server");
    let no_daemon = matches.is_present("no-daemon");

    if server && !no_daemon {

        let daemonize = Daemonize::new().pid_file("/tmp/backgrounder.pid")
            .chown_pid_file(true)
            .privileged_action(|| {
                server::run()
            });

        match daemonize.start() {
            Ok(_) => { },
            Err(e) => {
                println!("Error starting server");
                println!("{}", e);
                exit(1);
            }
        }
    } else if server {
        server::run()
    }

    subcommand(client, &matches);
}

fn gen_completions(mut app: clap::App) {
    let shell_vars = vec!["ZCOMPLETE_DIR","BCOMPLETE_DIR","FCOMPLETE_DIR"];
    let shell_enums = vec![Shell::Zsh, Shell::Bash, Shell::Fish];

    for (var,shell) in shell_vars.iter().zip(shell_enums.into_iter()) {
        let outdir = match env::var_os(var) {
            None => return,
            Some(outdir) => outdir,
        };

        let path = Path::new(&outdir);
        let abspath = match path.canonicalize() {
            Ok(p) => p,
            Err(e) => {
                println!("Failed to canonicalize given path");
                println!("\t{}", e.description());
                exit(1);
            }
        };

        if path.exists() {
            if path.is_dir() {
                app.gen_completions("backgrounder",
                                    shell,
                                    abspath);
            } else {
                println!("Error: given path is not a directory");
                exit(1);
            }
        } else {
            if let Err(e) = fs::create_dir_all(path) {
                println!("Failed to create directory");
                println!("\t{}", e.description());
                exit(1);
            }

            app.gen_completions("backgrounder",
                                shell,
                                abspath);
        }
    }
}

fn subcommand(mut client: client::Player, m: &ArgMatches) {
    match m.subcommand() {
        ("next", Some(_)) => {
            client.next();
        },
        ("prev", Some(_)) => {
            client.prev();
        },
        ("shuffle", Some(_)) => {
            client.toggle_shuffle();
        },
        ("play", Some(_)) => {
            client.toggle_play();
        },
        ("current", Some(_)) => {
            println!("{}",client.current());
        },
        ("index", Some(_)) => {
            println!("{}",client.index());
        },
        ("interval", Some(sub)) => {
            let inter = value_t!(sub, "time", u64).unwrap_or_else(|e| e.exit());
            client.interval(inter);
        },
        ("remove", Some(_)) => {
            client.remove();
        },
        ("quit", Some(_)) => {
            client.quit();
        },
        ("undo", Some(_)) => {
            client.undo();
        },
        ("logs", Some(_)) => {
            let mut logger = <Logger as Messenger<Client>>::new()
                .expect("failed to create logger");
            println!("{:?}",logger.recv());
        },
        ("save", Some(sub)) => {
            let fpath = value_t!(sub, "path", String).unwrap_or_else(|e| e.exit());
            let path = Path::new(&fpath);
            let force = sub.is_present("force");

            if let Ok(expanded) = path.canonicalize() {
                if let Some(name) = expanded.to_str() {
                    if expanded.exists() && !force {
                        println!("file '{}' exists, run with --force to overwrite", name);
                    } else {
                        client.save(name.to_string());
                    }
                } else {
                    println!("failed to get full path from '{}'", fpath);
                }
            } else {
                println!("failed to get full path from '{}'", fpath);
            }
        },
        ("append", Some(sub)) => {
            let paths = values_t!(sub, "path", String).unwrap_or_else(|e| e.exit());
            client.append_list(paths);
        },
        ("set", Some(sub)) => {
            let paths = values_t!(sub, "path", String).unwrap_or_else(|e| e.exit());
            client.set_list(paths);
        },
        _ => {},
    }
}
