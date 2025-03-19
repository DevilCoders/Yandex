package storage

import (
	"io"
	"io/ioutil"
	"os"
	"path/filepath"

	"golang.org/x/net/context"
)

// FsWarehouse is a test storage for blobs to reduce network traffic.
type FsWarehouse struct {
	BaseDir string
}

// StoreID stores a chunk of data by it's id.
func (fs *FsWarehouse) StoreByID(ctx context.Context, chunkID string, data []byte) error {
	return ioutil.WriteFile(filepath.Join(fs.BaseDir, chunkID), data, 0666)
}

// Store stores a chunk of data.
func (fs *FsWarehouse) Store(ctx context.Context, chunk *LibChunk, data []byte) error {
	return fs.StoreByID(ctx, chunk.ID, data)
}

// Delete removes a chunk of data.
func (fs *FsWarehouse) Delete(ctx context.Context, chunk *LibChunk) error {
	return os.Remove(filepath.Join(fs.BaseDir, chunk.ID))
}

func (fs *FsWarehouse) getReader(_ context.Context, chunk *LibChunk) (io.ReadCloser, error) {
	return os.Open(filepath.Join(fs.BaseDir, chunk.ID))
}

// ReadChunkBody returns chunk data.
func (fs *FsWarehouse) ReadChunkBody(ctx context.Context, chunk *LibChunk) ([]byte, error) {
	rc, err := fs.getReader(ctx, chunk)
	if err != nil {
		return nil, err
	}
	//nolint:errcheck
	defer rc.Close()
	return ioutil.ReadAll(rc)
}
