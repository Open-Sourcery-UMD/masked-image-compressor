/// How far we should look behind for the LZ77 compression portion of DEFLATE
const WINDOW_SIZE: usize = 32768;

/// How far we should look ahead for bytes to compress for the LZ77 compression portion of DEFLATE
const LOOKAHEAD_BUFF_SIZE: usize = 258;

/// How many bytes must we save using a BackRef for LZ77 to use the backref instead of just
/// emitting the literal bytes
const LZ77_MIN_BACKREF_BYTES: usize = 2;

/// Token for the LZ77 part of the DEFLATE Algorithm
#[derive(Debug, PartialEq)]
pub(super) enum LZ77Token {
    /// (literal byte)
    Byte(u8),

    /// (offset, length)
    BackRef(usize, usize),
}

/// Takes a slice of bytes and transforms them an array of LZ77Token(s) that indicate LZ77
/// compression
pub(super) fn lz77_compress(bytes: &[u8]) -> Box<[LZ77Token]> {
    let mut tokens: Vec<LZ77Token> = Vec::with_capacity(bytes.len());

    let mut i = 0usize;

    while i < bytes.len() {
        let mut max_offset: Option<usize> = None;
        let mut max_length: usize = 0;

        let start = i.checked_sub(WINDOW_SIZE).unwrap_or(0);

        for j in start..i {
            if bytes[j] != bytes[i] {
                continue;
            }

            let mut offset: Option<usize> = None;
            let mut length: usize = 0;

            let lookahead = (i - j).min(LOOKAHEAD_BUFF_SIZE).min(bytes.len() - i);

            for k in 0..lookahead {
                if bytes[j + k] != bytes[i + k] {
                    break;
                }

                if offset.is_none() {
                    offset = Some(i - j);
                }

                length += 1;
            }

            if length >= max_length {
                max_offset = offset;
                max_length = length;
            }
        }

        if max_offset.is_some() && max_length >= LZ77_MIN_BACKREF_BYTES {
            tokens.push(LZ77Token::BackRef(max_offset.unwrap(), max_length));
            i += max_length;
        } else {
            tokens.push(LZ77Token::Byte(bytes[i]));
            i += 1;
        }
    }

    tokens.into_boxed_slice()
}

/// Decompresses a slice of LZ77Tokens to an array of bytes
pub(super) fn lz77_decompress(tokens: &[LZ77Token]) -> Box<[u8]> {
    let mut bytes = Vec::with_capacity(tokens.len());

    for t in tokens {
        if let LZ77Token::Byte(b) = t {
            bytes.push(*b);
        } else if let LZ77Token::BackRef(offset, length) = t {
            let start = bytes.len().checked_sub(*offset).expect("start underflow");
            let end = start + length;

            if end > bytes.len() {
                panic!("End does not point to a valid subarray in bytes");
            }

            for j in start..end {
                // It's okay to read from the previous values in the array, since this is how
                // the compression works.
                bytes.push(bytes[j]);
            }
        }
    }

    bytes.into_boxed_slice()
}

/// All tests were written assuming WINDOW_SIZE = 32768, LOOKAHEAD_BUFF_SIZE = 258, and
/// LZ77_MIN_BACKREF_BYTES = 2
#[cfg(test)]
mod tests {
    use crate::lif::lz77::LZ77Token::{BackRef, Byte};
    use crate::lif::lz77::{LZ77Token, lz77_compress, lz77_decompress};
    use rand::{RngExt, rng};

    const fn byte(c: char) -> LZ77Token {
        Byte(c as u8)
    }

    #[test]
    fn test_lz77_compress_decompress_random_50() {
        let mut rng = rng();

        const BYTES_TO_COMPRESS: usize = 5000;

        for _ in 0..50 {
            let mut input: Vec<u8> = Vec::with_capacity(BYTES_TO_COMPRESS);

            for _ in 0..BYTES_TO_COMPRESS {
                input.push(rng.random());
            }

            let compressed = lz77_compress(&input);
            let decompressed = lz77_decompress(&compressed);


            println!(
                "Percentage saved {:.2}%",
                1.0 - (compressed.len() as f64) / (input.len() as f64)
            );

            assert_eq!(input.into_boxed_slice(), decompressed);
        }
    }

    #[test]
    fn test_lz77_decompress_1() {
        let bytes = "ha ha ha".as_bytes();
        let compressed = lz77_compress(bytes);
        let decompressed = lz77_decompress(&compressed);

        assert_eq!(bytes, decompressed.iter().as_slice());
    }

    #[test]
    fn test_lz77_compress_1() {
        let bytes = "ha ha ha".as_bytes();
        let candidate = lz77_compress(bytes);

        assert_eq!(
            vec![
                byte('h'),
                byte('a'),
                byte(' '),
                BackRef(3, 3),
                BackRef(3, 2)
            ]
            .into_boxed_slice(),
            candidate
        );
    }

    #[test]
    fn test_lz77_compress_2() {
        let bytes = "ba de bump de bump".as_bytes();
        let candidate = lz77_compress(bytes);

        assert_eq!(
            vec![
                byte('b'),
                byte('a'),
                byte(' '),
                byte('d'),
                byte('e'),
                byte(' '),
                byte('b'),
                byte('u'),
                byte('m'),
                byte('p'),
                BackRef(8, 8)
            ]
            .into_boxed_slice(),
            candidate
        )
    }

    #[test]
    fn test_lz77_compress_3() {
        let bytes =
            "Good day my friend! Well good day my friend as well. Ha ha ha ha ha!".as_bytes();
        let candidate = lz77_compress(bytes);

        let expected = vec![
            byte('G'),
            byte('o'),
            byte('o'),
            byte('d'),
            byte(' '),
            byte('d'),
            byte('a'),
            byte('y'),
            byte(' '),
            byte('m'),
            BackRef(3, 2),
            byte('f'),
            byte('r'),
            byte('i'),
            byte('e'),
            byte('n'),
            byte('d'),
            byte('!'),
            byte(' '),
            byte('W'),
            byte('e'),
            byte('l'),
            byte('l'),
            byte(' '),
            byte('g'),
            BackRef(25, 17),
            byte(' '),
            byte('a'),
            byte('s'),
            byte(' '),
            byte('w'),
            BackRef(27, 3),
            byte('.'),
            byte(' '),
            byte('H'),
            byte('a'),
            byte(' '),
            byte('h'),
            BackRef(3, 3),
            BackRef(6, 6),
            byte('a'),
            byte('!'),
        ]
        .into_boxed_slice();

        let byte_count = expected.iter().fold(0usize, |a, e| {
            if let BackRef(_, length) = e {
                a + length
            } else {
                a + 1
            }
        });

        assert_eq!(byte_count, bytes.len());
        assert_eq!(expected, candidate);
    }
}
