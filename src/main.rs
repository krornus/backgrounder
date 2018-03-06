extern crate dbus;
#[macro_use]
extern crate clap;
extern crate rand;

use clap::App;

mod client;
mod server;
mod player;


fn main() {
    let yaml = load_yaml!("./args.yaml");
    let matches = App::from_yaml(yaml).get_matches();

    if matches.is_present("server") {
        server::run();
    } else {
        test_client();
    }

}

fn test_client() {
    let mut player = client::Player::new();
    println!("Playing: {}", player.is_playing());
    println!("Setting play = true");
    player.play(true);
    println!("Playing: {}", player.is_playing());
    println!("Current: {}", player.current());
    println!("Next -> {}", player.next());
    println!("Next -> {}", player.next());
    println!("Next -> {}", player.next());
    println!("Prev -> {}", player.prev());
    println!("Prev -> {}", player.prev());
    println!("Prev -> {}", player.prev());
    println!("Current: {}", player.current());
    println!("Shuffled: {}", player.is_shuffled());
    println!("Setting shuffle = true");
    player.shuffle(true);
    println!("Shuffled: {}", player.is_shuffled());
    println!("Current: {}", player.current());
    println!("Next -> {}", player.next());
    println!("Next -> {}", player.next());
    println!("Next -> {}", player.next());
    println!("Prev -> {}", player.prev());
    println!("Prev -> {}", player.prev());
    println!("Prev -> {}", player.prev());
    println!("Unshuffling");
    player.shuffle(false);
    println!("Current: {}", player.current());
    println!("Next -> {}", player.next());
    println!("Next -> {}", player.next());
    println!("Next -> {}", player.next());
    println!("Prev -> {}", player.prev());
    println!("Prev -> {}", player.prev());
    println!("Prev -> {}", player.prev());
    println!("Removing {}", player.current());
    player.remove();
    println!("Current: {}", player.current());
    println!("Removing {}", player.current());
    player.remove();
    println!("Current: {}", player.current());
    println!("Removing {}", player.current());
    player.remove();
    println!("Current: {}", player.current());
    println!("Undoing remove");
    player.undo();
    println!("Current: {}", player.current());
    println!("Undoing remove");
    player.undo();
    println!("Current: {}", player.current());
    println!("Undoing remove");
    player.undo();
    println!("Current: {}", player.current());
    println!("Next -> {}", player.next());
    println!("Next -> {}", player.next());
    println!("Next -> {}", player.next());
    println!("Setting list to x:");
    player.set_list(vec!["x".to_string()]);
    println!("Current: {}", player.current());
    println!("Appending a,b,c:");
    player.append_list(vec!["a","b","c"].into_iter().map(String::from).collect());
    println!("Next -> {}", player.next());
    println!("Next -> {}", player.next());
    println!("Next -> {}", player.next());
    println!("Next -> {}", player.next());
    println!("Removing {}", player.current());
    println!("At index {}", player.index());
    player.remove();
    println!("Next -> {}", player.next());
    println!("Next -> {}", player.next());
    println!("Next -> {}", player.next());
}
