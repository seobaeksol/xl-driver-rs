use core::fmt;

use xl_driver_sys::loader::XlApiLoadError;

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct XlError {
    pub code: Option<i16>,
    pub message: String,
}

impl XlError {
    pub fn new(code: Option<i16>, message: impl Into<String>) -> Self {
        Self {
            code,
            message: message.into(),
        }
    }

    pub fn from_status(code: i16, message: impl Into<String>) -> Self {
        Self::new(Some(code), message)
    }
}

impl fmt::Display for XlError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self.code {
            Some(code) => write!(f, "XL error {code}: {}", self.message),
            None => write!(f, "XL error: {}", self.message),
        }
    }
}

impl std::error::Error for XlError {}

impl From<XlApiLoadError> for XlError {
    fn from(value: XlApiLoadError) -> Self {
        Self::new(None, value.to_string())
    }
}
