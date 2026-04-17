use std::path::{Path, PathBuf};

pub const DEFAULT_XL_API_DLL: &str = "vxlapi64.dll";

/// Repository-level representation of the target XL API DLL location.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct LibraryLocation {
    path: PathBuf,
}

impl LibraryLocation {
    pub fn new(path: impl Into<PathBuf>) -> Self {
        Self { path: path.into() }
    }

    pub fn default_dll() -> Self {
        Self::new(DEFAULT_XL_API_DLL)
    }

    pub fn as_path(&self) -> &Path {
        &self.path
    }
}
