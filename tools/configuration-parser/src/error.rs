#[derive(Debug, thiserror::Error)]
#[error("{msg}")]
pub struct Error {
    msg: String
}

impl Error {
    pub fn msg(msg: String) -> Self {
        Self { msg }
    }
}