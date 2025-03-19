package lbtypes

import (
	"bytes"
	"io"
)

var _ DataReader = &TestReader{}

type TestReader struct {
	bytes.Reader
}

func NewTestReader(data []byte) *TestReader {
	return &TestReader{
		Reader: *bytes.NewReader(data),
	}
}

func (r *TestReader) Close() error {
	_, _ = r.Reader.Seek(0, io.SeekStart)
	return nil
}

func (r *TestReader) RawSize() int {
	return int(r.Size())
}
