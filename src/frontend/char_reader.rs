
use std::io::BufRead;
use std::io;

use super::ext_char::ExtChar;

pub struct Chars<B: BufRead> {
    reader: CharReader<B>
}

impl<B: BufRead> Iterator for Chars<B> {
    type Item = Result<char, io::Error>;

    fn next(&mut self) -> Option<Result<char, io::Error>> {
        self.reader.read_char().map(|c| c.into()).transpose()
    }
}

/// The `CharReader` struct allows reading single characters from a `BufReader`.
pub struct CharReader<B: BufRead> {
    reader: B,
    line: Option<Vec<char>>,
    char_index: usize,
}

impl<B: BufRead> CharReader<B> {
    pub fn new(reader: B) -> CharReader<B> {
        CharReader {
            reader: reader,
            line: None,
            char_index: 0,
        }
    }

    /// Create an iterator that yields single characters from the `BufReader`
    pub fn iter(self) -> Chars<B> {
        Chars {
            reader: self
        }
    }

    fn next_line(&mut self) -> Result<bool, io::Error> {
        let mut buf = String::new();

        let num_bytes = self.reader.read_line(&mut buf)?;
        if num_bytes == 0 {
            return Ok(false);
        }

        self.line = Some(buf.chars().collect());
        self.char_index = 0;

        Ok(true)
    }

    /// Read a single character.
    ///
    /// If no character is available at the moment (e.g. the reader is at the end of the file),
    /// `None` is returned.
    ///
    /// The underlying `read()` might fail, in which case this method returns the error
    pub fn read_char(&mut self) -> Result<ExtChar, io::Error> {
        if self.line.is_none() {
            let have_next_line = self.next_line()?;
            if !have_next_line {
                return Ok(ExtChar::EOF);
            }
        }

        let line = self.line.as_ref().unwrap();

        if self.char_index < line.len() {
            let c = line[self.char_index];
            self.char_index += 1;
            Ok(ExtChar::Char(c))
        } else {
            let have_next_line = self.next_line()?;
            if have_next_line {
                self.read_char()
            } else {
                Ok(ExtChar::EOF)
            }
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    use std::io::{Cursor, BufReader};

    fn test_with_dummy_reader(text: &str) {
        let buf = Cursor::new(text);
        let reader = BufReader::new(buf);

        let char_reader = CharReader::new(reader);

        let result: Result<String, _> = char_reader.iter().collect();

        assert!(result.is_ok());

        assert_eq!(result.unwrap(), text);
    }

    #[test]
    fn simple() {
        let text = "You must be shapeless, formless, like water.\n[...]\nWater can drip and it can crash. Become like water my friend.";

        test_with_dummy_reader(text);
    }

    #[test]
    fn newlines() {
        let text = "\
All that is gold does not glitter,\n
Not all those who wander are lost;\r\n
The old that is strong does not wither,\n\r
Deep roots are not reached by the frost.\r";

        test_with_dummy_reader(text);
    }

    #[test]
    fn utf8() {
        let text = "𝐏𝖗ṑģ𝔯āṁ ƭᶒṡτ𝔦𝝅ց ϲảṇ Ѣẽ 𝕒 𝕧ệɽŷ 𝚎ẝꞙ℮çт𝒊𝞶ḝ ẇ𝒂ỿ ṯℴ ṣ𝕙өш 𝓉ḧᶒ ꝓⲅȩśě𝗻с℮ ô𝑓 𝗯ừ𝒈𐑈, ƀʉƭ ɨ𝜏 ḭś 𝗁𝜎⍴ěɬểꞩꜱɫ𝝲 𝘪ṇȁďȅɋʋ𝐚ҭḛ 𝖿ợř ᶊ𝓱ǫẘⅈ𝓃𝑔 𝞽𝓱ểï𝒓 ᶏ𝒷𝐬ḝň𝔠è.";

        test_with_dummy_reader(text);
    }
}
