
use std::path::{PathBuf, Path};

use crate::frontend::ast::RootNode;
use crate::shared::intermediate::Intermediate;

#[derive(PartialEq, Debug)]
pub enum CompilationState {
    Parsed,
    Compiled,
}

#[derive(Debug)]
pub struct CacheEntry {
    pub path: PathBuf,
    pub state: CompilationState,
    pub ast: Option<Box<RootNode>>,
    pub intermediate: Option<Intermediate>,
}

#[derive(Debug)]
pub struct Cache {
    entries: Vec<CacheEntry>,
}

impl Cache {
    pub fn new() -> Cache {
        Cache {
            entries: Vec::new(),
        }
    }

    pub fn add(&mut self, entry: CacheEntry) {
        self.entries.push(entry);
    }

    pub fn get(&self, path: &Path) -> Option<&CacheEntry> {
        self.entries.iter()
            .find(|entry| entry.path == path)
    }

    pub fn get_mut(&mut self, path: &Path) -> Option<&mut CacheEntry> {
        self.entries.iter_mut()
            .find(|entry| entry.path == path)
    }

    pub fn collect_intermediate(self) -> Vec<Intermediate> {
        let mut result = Vec::new();

        for entry in self.entries {
            if let Some(intermediate) = entry.intermediate {
                result.push(intermediate);
            }
        }

        result
    }
}
