use std::thread;
use std::sync::Arc;
use std::sync::atomic::{AtomicBool, Ordering};

/* method to spawn threads and check if they have failed */
pub struct FallibleThread<T, E> {
    handle: thread::JoinHandle<Result<T, E>>,
    error: Arc<AtomicBool>,
}

impl<T, E> FallibleThread<T, E> {
    pub fn spawn<F>(f: F) -> Self
    where
        F: FnOnce() -> Result<T, E>,
        F: Send + 'static,
        T: Send + 'static,
        E: Send + 'static,
    {
        let error = Arc::new(AtomicBool::new(false));
        let flag = error.clone();

        let handle = thread::spawn(move || {
            f().map_err(|e| {
                flag.store(true, Ordering::Relaxed);
                e
            })
        });

        FallibleThread {
            handle,
            error,
        }
    }

    pub fn is_err(&self) -> bool {
        self.error.load(Ordering::Relaxed)
    }

    pub fn join(self) -> thread::Result<Result<T, E>> {
        self.handle.join()
    }
}
