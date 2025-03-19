package chunker

import (
	"compress/gzip"
	"fmt"
	"io"

	lz "github.com/pierrec/lz4"
	werrors "github.com/pkg/errors"

	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/storage"
)

var (
	_ CompressorConstructor = NewGZipCompressor
	_ CompressorConstructor = NewRawCompressor
	_ CompressorConstructor = NewLZ4Compressor
)

// CompressorConstructor builds a Compressor.
type CompressorConstructor func(io.Writer) Compressor

// Compressor is a chunk compression algorithm.
type Compressor struct {
	io.WriteCloser
	format storage.ChunkFormat
}

// NewGZipCompressor returns a gzip compressor.
func NewGZipCompressor(w io.Writer) Compressor {
	return Compressor{
		WriteCloser: gzip.NewWriter(w),
		format:      storage.GZip,
	}
}

type rawCompressor struct {
	w io.Writer
}

func (c *rawCompressor) Write(b []byte) (int, error) {
	return c.w.Write(b)
}

func (c *rawCompressor) Close() error {
	return nil
}

// NewRawCompressor returns a raw (nop) compressor.
func NewRawCompressor(w io.Writer) Compressor {
	return Compressor{
		WriteCloser: &rawCompressor{w},
		format:      storage.Raw,
	}
}

// NewLZ4Compressor returns a LZ4 (fast, but not so effective) compressor.
func NewLZ4Compressor(w io.Writer) Compressor {
	return Compressor{
		WriteCloser: lz.NewWriter(w),
		format:      storage.Lz4,
	}
}

// DecompressorConstructor builds a Decompressor.
type DecompressorConstructor func(io.Reader) (*Decompressor, error)

// Decompressor is a chunk decompression algorithm.
type Decompressor struct {
	io.ReadCloser
	format storage.ChunkFormat
}

func newGZipDecompressor(r io.Reader) (*Decompressor, error) {
	d, err := gzip.NewReader(r)
	if err != nil {
		return nil, werrors.Wrap(err, "unable to create gzip.Reader")
	}
	return &Decompressor{
		ReadCloser: d,
		format:     storage.GZip,
	}, nil
}

type nopCloser struct {
	io.Reader
}

func (c *nopCloser) Close() error {
	return nil
}

func newRawDecompressor(r io.Reader) (*Decompressor, error) {
	return &Decompressor{
		ReadCloser: &nopCloser{r},
		format:     storage.Raw,
	}, nil
}

func newLZ4Decompressor(r io.Reader) (*Decompressor, error) {
	// for Close() method
	return &Decompressor{
		ReadCloser: &nopCloser{lz.NewReader(r)},
		format:     storage.Lz4,
	}, nil
}

// BuildDecompressor builds a Decompressor for provided format.
func BuildDecompressor(format storage.ChunkFormat) (DecompressorConstructor, error) {
	switch format {
	case storage.GZip:
		return newGZipDecompressor, nil
	case storage.Lz4:
		return newLZ4Decompressor, nil
	case storage.Raw:
		return newRawDecompressor, nil
	default:
		return nil, fmt.Errorf("unknown chunk format: %s", format)
	}
}
