# Backgrounder
A tool for manipulating your desktop background using feh. Currently written in Rust, with an old implementation written in Python.
## Compiling
```bash
git clone https://github.com/krornus/backgrounder.git
cd backgrounder
cargo build --release
```
> The compiled file is located in ./target/release/backgrounder-rs
## Running
```bash
backgrounder-rs --server
backgrounder-rs set https://imgur.com/gallery/hOF1g
backgrounder-rs append https://imgur.com/gallery/XnnPe
backgrounder-rs append /home/user/pictures
backgrounder-rs next
backgrounder-rs shuffle
backgrounder-rs interval 120
```
# Commands
| Command        | Purpose                         |
| :------------: | ---                             |
| set | Set the playlist of backgrounds
| append | Append a path to the playlist
| next | Skip current background
| prev | Go to previous background
| current | Print the path to the current backkground (may be file or url)
| play | Toggle pause/play for cycling through backgrounds
| shuffle | Toggle the playlist being shuffled
| interval | Set the interval (s) for cycling through the playlist
| index | Get the current index in the playlist
| remove | Remove current image from playlist
| undo | Undo remove command
| quit | Close the server daemon
| save | Save current list to file (**Not  implemented**)
# Config
The program looks for  a file named `.bgrc` first in `$HOME`, then in `/etc/`.  The config takes the following fields:

>The syntax for the config file requires quoted strings, and semi-colon line endings.
 
| Field | Type | Purpose | Default |
| - | :-: | :-: | -: |
| path | string or list of strings | paths that will be automatically loaded on start | [""]
| mode | string | how the background is displayed, see below for modes | "max"
| shuffle | boolean | whether the paths start shuffled | false
| play | boolean | whether the paths will cycle | true
| interval | integer (s) | how long to spend between cycles | 120 

>The possible modes for the mode option are: "center", "fill", "max", "scale", "tile".

# Server
Run with `-s` or `--server` to start the server. Run with `-d` or `--no-daemon` to run the server in the foreground
Run with `-c` or `--config` to provide a custom path to the config
# Custom Paths
Backgrounder has parser modules for each type of "path" such as an imgur gallery, local file, or folder. If you want your own website parser you can add it by creating a new file which implements the `SiteParser` trait for a given struct. The struct must then be created and added to the list in the `run()` function in `server.rs`, and imported as a module in `main.rs`

> **Note**: Any paths returned by your parser will be recursively parsed until nothing new is returned. For example, this means you can return a link to an imgur gallery from your parser, which will be recursively expanded to a list of images by the imgur parser. The parsing is done in order of appearance in the `parsers` list in `server.rs`, as shown below. The first parser to return true from the `is_valid()` function is used.

## Example

`foo.rs`
```rust
use parser::{SiteParser,Error};

#[derive(Debug,Clone)]
pub struct FooParser;

impl SiteParser for FooParser {
    fn parse(&self, uri: &str) -> Vec<String> {
        /* your logic here */
        /* return vector of Strings which represent parsed images */
    }
    fn is_valid(&self, uri: &str) -> bool {
        /* return a bool based on the given string to test if the path is valid */
    }
}
```
`main.rs`
```rust
...
mod foo;
...
```
`server.rs`
```rust
use foo::FooParser;
...
pub fn run() {
    ...
    let fooparser = FooParser;
    let parsers: Vec<Box<SiteParser>> = vec![
        ...
        Box::new(fooparser),
        ...
    ];
    ...
}
```
# Autocomplete
Bash, Zsh, and Fish auto generated completion files are available. Generation may be done by assigning a directory to one of the following environment variables respectively: 
```bash
BCOMPLETE_DIR
ZCOMPLETE_DIR
FCOMPLETE_DIR
```
The file will be placed in the given directory.

# TODO
- dmesg style logger
- retry on connection failure for retrieving web paths
    - currently fails, returns none, and ends up with permanently empty playlist until set again
- ping function on dbus to check if up
    - also need to use ping function on invocation
- use direct X11 calls instead of feh
    - code mostly done, needs modes implemented and cleanup
