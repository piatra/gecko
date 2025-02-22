// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

use std::sync::{Mutex, MutexGuard};

use once_cell::sync::Lazy;

const GLOBAL_APPLICATION_ID: &str = "org.mozilla.firefox.test";

/// UGLY HACK.
/// We use a global lock to force synchronization of all tests, even if run multi-threaded.
/// This allows us to run without `--test-threads 1`.`
pub fn lock_test() -> (MutexGuard<'static, ()>, tempfile::TempDir) {
    static GLOBAL_LOCK: Lazy<Mutex<()>> = Lazy::new(|| Mutex::new(()));

    let lock = GLOBAL_LOCK.lock().unwrap();

    let dir = setup_glean(None);
    (lock, dir)
}

// Create a new instance of Glean with a temporary directory.
// We need to keep the `TempDir` alive, so that it's not deleted before we stop using it.
fn setup_glean(tempdir: Option<tempfile::TempDir>) -> tempfile::TempDir {
    let dir = match tempdir {
        Some(tempdir) => tempdir,
        None => tempfile::tempdir().unwrap(),
    };
    let tmpname = dir.path().display().to_string();

    let cfg = glean_core::Configuration {
        data_path: tmpname,
        application_id: GLOBAL_APPLICATION_ID.into(),
        upload_enabled: true,
        max_events: None,
        delay_ping_lifetime_io: false,
        language_binding_name: "Rust".into(),
    };
    let glean = glean_core::Glean::new(cfg).unwrap();
    glean_core::setup_glean(glean).expect("can't set up global Glean object");

    // This might have been flushed by other tests already, so we ignore the return value.
    // The dispatch queue is definitely unblocked after this, no matter what.
    let _ = super::flush_init();

    dir
}
