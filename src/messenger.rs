use nanomsg::{Socket, Protocol, Error, Endpoint, PollFd, PollInOut};
use std::io::{Read,Write};

const SOCKET: &'static str = "ipc:///tmp/backgrounder.ipc";

pub struct Server;
pub struct Client;

pub trait Messenger<Method> where Self: Sized {
    fn new() -> Result<Self, Error>;
}


pub struct Logger {
    endpoint: Endpoint,
    socket: Socket,
    pollfd: PollFd,
}

impl Messenger<Client> for Logger {
    fn new() -> Result<Self, Error> {

        let mut socket = Socket::new(Protocol::Pull)?;
        let endpoint = socket.connect(SOCKET)?;
        let pollfd = socket.new_pollfd(PollInOut::In);

        Ok(Logger {
            socket,
            endpoint,
            pollfd,
        })
    }
}

impl Messenger<Server> for Logger {
    fn new() -> Result<Self, Error> {

        let mut socket = Socket::new(Protocol::Push)?;

        let endpoint = socket.connect(SOCKET)?;
        let pollfd = socket.new_pollfd(PollInOut::Out);
        socket.set_linger(-1)?;

        Ok(Logger {
            socket,
            endpoint,
            pollfd,
        })
    }
}

impl Logger where Self: Messenger<Server> {
    pub fn send(&mut self, s: &str) -> ::std::io::Result<usize> {

        let msg = s.to_string().into_bytes();
        println!("starting write");
        let r = self.socket.write(&msg);
        println!("finished write");
        r
    }
}

impl Logger where Self: Messenger<Client> {
    pub fn recv(&mut self) -> Option<String> {
//        if self.pollfd.can_read() {
            let mut msg = String::new();
            if let Err(_) = self.socket.read_to_string(&mut msg) {
                None
            } else {
                Some(msg)
            }
 //       } else {
 //           None
 //       }
    }
}

impl Drop for Logger {
    fn drop(&mut self) {
        self.endpoint.shutdown()
            .expect("failed to shutdown logger");
    }
}
