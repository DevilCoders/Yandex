package compressor

import (
	"bytes"
	"fmt"
	"io"

	"github.com/pierrec/lz4"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/errors"
)

////////////////////////////////////////////////////////////////////////////////

func Compress(format string, data []byte) ([]byte, error) {
	compressor, err := newCompressor(format)
	if err != nil {
		return nil, err
	}

	compressedData, err := compressor.compress(data)
	if err != nil {
		return nil, &errors.NonRetriableError{Err: err}
	}

	return compressedData, nil
}

func Decompress(format string, data []byte, result []byte) error {
	compressor, err := newCompressor(format)
	if err != nil {
		return err
	}

	err = compressor.decompress(data, result)
	if err != nil {
		return &errors.NonRetriableError{Err: err}
	}

	return nil
}

////////////////////////////////////////////////////////////////////////////////

type compressor interface {
	compress(data []byte) ([]byte, error)
	decompress(data []byte, result []byte) error
}

func newCompressor(format string) (compressor, error) {
	switch format {
	case "lz4":
		return &lz4Compressor{}, nil
	case "":
		return &nullCompressor{}, nil
	default:
		return nil, &errors.NonRetriableError{
			Err: fmt.Errorf("invalid compression format: %v", format),
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

type nullCompressor struct{}

func (c *nullCompressor) compress(data []byte) ([]byte, error) {
	return data, nil
}

func (c *nullCompressor) decompress(data []byte, result []byte) error {
	copy(result, data)
	return nil
}

////////////////////////////////////////////////////////////////////////////////

type lz4Compressor struct{}

func (c *lz4Compressor) compress(data []byte) ([]byte, error) {
	var buf bytes.Buffer
	buf.Grow(len(data))

	writer := lz4.NewWriter(&buf)
	_, err := writer.Write(data)
	if err != nil {
		return nil, err
	}

	err = writer.Close()
	if err != nil {
		return nil, err
	}

	return buf.Bytes(), nil
}

func (c *lz4Compressor) decompress(data []byte, result []byte) error {
	reader := lz4.NewReader(bytes.NewReader(data))
	_, err := reader.Read(result)
	if err == io.EOF {
		return nil
	}
	return err
}
