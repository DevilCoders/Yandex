package client

import (
	"crypto/sha512"
	"encoding/hex"
	"fmt"
	"io"

	"golang.org/x/net/context"
	"google.golang.org/grpc"

	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/snapshot_rpc"
)

// SnapshotReaderError is useful for type switch
type SnapshotReaderError struct {
	message string
}

func (e SnapshotReaderError) Error() string {
	return e.message
}

func newSnapshotReaderError(f string, args ...interface{}) error {
	return SnapshotReaderError{
		message: fmt.Sprintf(f, args...),
	}
}

// Subset of GRPC Client interface
type chunkGRPCReader interface {
	Read(ctx context.Context, in *snapshot_rpc.ReadRequest, opts ...grpc.CallOption) (*snapshot_rpc.ReadResponse, error)
}

type chunkGRPCReceiver interface {
	Recv() (*snapshot_rpc.MapResponse, error)
}

// SnapshotReader hides chunk map operation
type SnapshotReader interface {
	ReadAt(ctx context.Context, p []byte, offset int64) (int, error)
	GetBlockChecksum(size, offset int64) (string, error)
	GetID() string
	GetSnapshotChecksum() string
	GetSize() int64
	GetChunkSize() int64
	IsZeroChunk(size, offset int64) (bool, error)
	io.Closer
}

type snapshotReader struct {
	chunks     [][]*snapshot_rpc.MapResponse_Chunk
	totalCount int
	columnsize int
	chunksize  int64

	id       string
	checkSum string

	client snapshot_rpc.SnapshotServiceClient
}

func newSnapshotReader(ctx context.Context, client snapshot_rpc.SnapshotServiceClient, id string) (SnapshotReader, error) {
	sr := snapshotReader{
		// NOTE: eliminate reallocs here by reserver some space
		chunks: make([][]*snapshot_rpc.MapResponse_Chunk, 0),
		client: client,
		id:     id,
	}

	stream, err := sr.client.GetMap(ctx, &snapshot_rpc.MapRequest{Id: id})
	if err != nil {
		return nil, err
	}

	if err = sr.collectChunks(ctx, stream); err != nil {
		return nil, err
	}

	return &sr, nil
}

func (s *snapshotReader) collectChunks(_ context.Context, stream chunkGRPCReceiver) error {
	h := sha512.New()
	offset := int64(-1)
	size := int64(-1)
	for {
		resp, err := stream.Recv()
		switch err {
		case nil:
			for _, chunk := range resp.Chunks {
				// Check that order is sorted
				if chunk.Offset <= offset {
					return newSnapshotReaderError("increasing offset order violated")
				}
				offset = chunk.Offset
				if size == -1 {
					size = chunk.Chunksize
				}

				if chunk.Chunksize != size {
					return newSnapshotReaderError("received a chunk with different size")
				}

				// Calculate hashsum for verification
				if _, err = h.Write([]byte(chunk.Hashsum)); err != nil {
					return err
				}
			}

			s.totalCount += len(resp.Chunks)
			s.chunks = append(s.chunks, resp.Chunks)
		case io.EOF:
			if len(s.chunks) == 0 || len(s.chunks[0]) == 0 {
				return newSnapshotReaderError("snapshot map is empty")
			}

			var length = len(s.chunks[0])
			for i := 1; i < len(s.chunks)-1; i++ {
				if length != len(s.chunks[i]) {
					return newSnapshotReaderError("not all columns have equal length")
				}
			}
			s.columnsize = length

			s.chunksize = s.chunks[0][0].Chunksize
			s.checkSum = "sha512:" + hex.EncodeToString(h.Sum(nil))
			return nil
		default:
			return err
		}
	}
}

func (s *snapshotReader) ReadAt(ctx context.Context, p []byte, offset int64) (int, error) {
	return s.readAt(ctx, s.client, p, offset)
}

func (s *snapshotReader) getChunkUnaligned(offset int64) (*snapshot_rpc.MapResponse_Chunk, error) {
	var empty *snapshot_rpc.MapResponse_Chunk
	pos := offset / s.chunksize
	if pos >= int64(s.totalCount) {
		return empty, newSnapshotReaderError("chunk #%d is out of range #%d", pos, s.totalCount)
	}

	column := pos / int64(s.columnsize)
	row := pos % int64(s.columnsize)

	return s.chunks[column][row], nil
}

func (s *snapshotReader) getChunk(size, offset int64) (*snapshot_rpc.MapResponse_Chunk, error) {
	var empty *snapshot_rpc.MapResponse_Chunk
	if size != s.chunksize {
		return empty, newSnapshotReaderError("no hashsum for given size %d", size)
	}

	if offset%s.chunksize != 0 {
		return empty, newSnapshotReaderError("no hashsum for given offset %d", offset)
	}

	return s.getChunkUnaligned(offset)
}
func (s *snapshotReader) GetChunkSize() int64 {
	return s.chunksize
}

func (s *snapshotReader) GetBlockChecksum(size, offset int64) (string, error) {
	chunk, err := s.getChunk(size, offset)
	return chunk.Hashsum, err
}

func (s *snapshotReader) GetID() string {
	return s.id
}

func (s *snapshotReader) GetSize() int64 {
	return s.chunksize * int64(s.totalCount)
}

func (s *snapshotReader) GetSnapshotChecksum() string {
	return s.checkSum
}

// size must be chunk size and offset must be zero-indexed of start chunk
func (s *snapshotReader) IsZeroChunk(size, offset int64) (bool, error) {
	chunk, err := s.getChunk(size, offset)
	if err != nil {
		return false, err
	}
	return chunk.Zero, nil
}

func (s *snapshotReader) readAt(ctx context.Context, chunkReader chunkGRPCReader, p []byte, offset int64) (int, error) {
	nn := 0
	for len(p) > 0 {
		chunk, err := s.getChunkUnaligned(offset)
		if err != nil {
			return nn, err
		}

		// number of bytes to copy
		length := s.chunksize - offset%s.chunksize
		if length > int64(len(p)) {
			length = int64(len(p))
		}

		var src []byte
		if chunk.Zero {
			src = make([]byte, length)
		} else {
			resp, err := chunkReader.Read(ctx, &snapshot_rpc.ReadRequest{
				Id: s.id,
				Coord: &snapshot_rpc.Coord{
					Offset:    chunk.Offset,
					Chunksize: chunk.Chunksize,
				},
			})
			if err != nil {
				return nn, err
			}
			src = resp.Data[offset%s.chunksize : offset%s.chunksize+length]
		}

		nn += copy(p, src)
		offset += length
		p = p[length:]
	}
	return nn, nil
}

func (s *snapshotReader) Close() error {
	// NOTE: reserve for possible cleaning
	return nil
}
