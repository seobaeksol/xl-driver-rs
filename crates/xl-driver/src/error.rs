use core::fmt;

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct XlError {
    pub code: i16,
    pub message: String,
}

impl XlError {
    pub fn new(code: i16, message: impl Into<String>) -> Self {
        Self {
            code,
            message: message.into(),
        }
    }
}

impl fmt::Display for XlError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "XL error {}: {}", self.code, self.message)
    }
}

impl std::error::Error for XlError {}
