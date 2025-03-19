package streamingcrypto

import (
	"io"
)

type ReadSeekerCloser interface {
	io.ReadSeeker
	io.Closer
}

// StreamingEncrypter wraps another WriteCloser. It encrypts the plaintext written into it and writes ciphertext
// into the wrapped WriteCloser.
type StreamingEncrypter interface {
	// Write() writes to buffer and may call Write() one or more times to write out the ciphertext.
	// Close() calls Close() of the wrapped WriteCloser.
	io.WriteCloser
	// Returns metadata, necessary for decrypting the ciphertext. May be called only after Close().
	Metadata() (string, error)
	// Returns KeyID, used for encryption.
	KeyID() string
	// Returns the ciphertext size for given plaintext size.
	CiphertextSize(plaintextSize int64) int64
}

// StreamingReadEncrypter wraps another ReadCloser. It encrypts the plaintext which is read from the wrapped ReadCloser
// and returns the ciphertext to the caller.
type StreamingReadEncrypter interface {
	// Read() reads from buffered ciphertext and may call wrapped Read() one or more times to refill and encrypt the buffer.
	// Close() calls Close() of the wrapped ReadCloser.
	io.ReadCloser
	// Returns metadata, necessary for decrypting the ciphertext. May be called only after Close().
	// May not be called if the encrypter was closed before all the plaintext has been read.
	Metadata() (string, error)
	// Returns KeyID, used for encryption.
	KeyID() string
	// Returns the ciphertext size for given plaintext size.
	CiphertextSize(plaintextSize int64) int64
}

// StreamingDecrypter wraps another ReadSeekerCloser. It decrypts the ciphertext which is read from the wrapped ReadSeekerCloser
// and returns the plaintext to the caller.
type StreamingDecrypter interface {
	// Read() reads from buffer plaintext and may call wrapped Read() one or more times to refill and decrypt the buffer.
	// Seek() may result in wrapped Seek() being called upon next Read().
	// Close() calls Close() of the wrapped ReadSeekerCloser
	ReadSeekerCloser
}

type Range struct {
	Start int64
	Stop  int64
}

// StreamingRangeDecrypter allows decrypting explicitly passed ciphertext ranges: CiphertextRange() must be called
// to return the required ciphertext range which matches the plaintext range, then the ciphertext reader
// for the corresponding ciphertext range must be passed to Plaintext() for decryption.
type StreamingRangeDecrypter interface {
	io.Closer
	// Returns the range of ciphertext require to be read in order to decrypt given plaintext range.
	CiphertextRange(ptStart, ptStop int64) (int64, int64, error)
	// Returns the reader which reads given plaintext range. ciphertextReader must read the range which was returned
	// from the previous CiphertextRange(plaintextRange) call.
	//
	// Closing the returned ReadCloser will close the ciphertextReader.
	//
	// Returned ReadClosers are independent of each other and do not share any state (including ReadClosers for the same
	// plaintextRange).
	Plaintext(plaintextRange Range, ciphertextReader io.ReadCloser) (io.ReadCloser, error)
}
