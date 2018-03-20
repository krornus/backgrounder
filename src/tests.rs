

// tests were running async??
#[test]
fn all() {
    play_pause();
    set_cycle();
    shuffle();
    double_set();
    append_cycle();
    index();
    remove();
}


#[allow(dead_code)]
fn play_pause() {
    use client;

    let mut player = client::Player::new();
    player.set_list(vec![]);

    assert!(!player.is_playing());
    player.play(true);
    assert!(player.is_playing());
}


#[allow(dead_code)]
fn set_cycle() {
    use client;

    let pl = vec!["a","b","c"].into_iter().map(String::from).collect();
    let mut player = client::Player::new();
    player.set_list(pl);

    assert!(player.current() == "a");
    assert!(player.next() == "b");
    assert!(player.next() == "c");
    assert!(player.next() == "a");
    assert!(player.prev() == "c");
    assert!(player.prev() == "b");
    assert!(player.prev() == "a");
    assert!(player.current() == "a");
}



#[allow(dead_code)]
fn shuffle() {
    use client;

    let pl = vec!["a","b","c"].into_iter().map(String::from).collect();
    let mut player = client::Player::new();
    player.set_list(pl);

    assert!(!player.is_shuffled());
    player.shuffle(true);
    assert!(player.is_shuffled());
    player.next();
    player.next();
    player.next();
    player.prev();
    player.prev();
    player.prev();
    assert!(player.is_shuffled());
    player.shuffle(false);
    assert!(!player.is_shuffled());
    assert!(player.current() == "a");
    player.next();
    player.next();
    player.next();
    player.prev();
    player.prev();
    player.prev();
    assert!(player.current() == "a");
}


#[allow(dead_code)]
fn double_set() {
    use client;

    let pl = vec!["a","b","c"].into_iter().map(String::from).collect();
    let mut player = client::Player::new();
    player.set_list(pl);
    player.set_list(vec!["x".to_string()]);

    let p = player.prev();
    println!("{:?}", p);
    assert!(p == "x");

    let p = player.prev();
    println!("{:?}", p);
    assert!(p == "x");

    let c = player.current();
    println!("{:?}", c);
    assert!(c == "x");
}


#[allow(dead_code)]
fn append_cycle() {
    use client;

    let pl = vec!["x"].into_iter().map(String::from).collect();
    let mut player = client::Player::new();
    player.set_list(pl);

    assert!(player.prev() == "x");
    assert!(player.prev() == "x");
    assert!(player.next() == "x");
    assert!(player.next() == "x");
    assert!(player.current() == "x");

    player.append_list(vec!["a","b","c"].into_iter()
        .map(String::from).collect());

    assert!(player.next() == "a");
    assert!(player.next() == "b");
    assert!(player.next() == "c");
    assert!(player.next() == "x");

    player.remove();

    assert!(player.index() == 0);

    assert!(player.next() == "b");
    assert!(player.next() == "c");
    assert!(player.next() == "a");
}

#[allow(dead_code)]
fn index() {
    use client;

    let pl = vec!["a","b","c"].into_iter().map(String::from).collect();
    let mut player = client::Player::new();
    player.set_list(pl);

    assert!(player.current() == "a");
    assert!(player.index() == 0);
    assert!(player.next() == "b");
    assert!(player.index() == 1);
    assert!(player.next() == "c");
    assert!(player.index() == 2);
    assert!(player.next() == "a");
    assert!(player.index() == 0);
    assert!(player.prev() == "c");
    assert!(player.index() == 2);
    assert!(player.prev() == "b");
    assert!(player.index() == 1);
    assert!(player.prev() == "a");
    assert!(player.current() == "a");
    assert!(player.index() == 0);
}

#[allow(dead_code)]
fn remove() {
    use client;

    let pl = vec!["a","b","c"].into_iter().map(String::from).collect();
    let mut player = client::Player::new();
    player.set_list(pl);

    player.remove();
    player.remove();
    player.remove();

    assert!(player.current() == "");
    player.undo();
    assert!(player.current() == "c");
    player.undo();
    assert!(player.current() == "b");
    player.undo();
    assert!(player.current() == "a");

    player.undo();
    assert!(player.current() == "a");

    assert!(player.next() == "b");
    assert!(player.next() == "c");
    assert!(player.next() == "a");
    assert!(player.prev() == "c");
    assert!(player.prev() == "b");
    assert!(player.prev() == "a");

}
