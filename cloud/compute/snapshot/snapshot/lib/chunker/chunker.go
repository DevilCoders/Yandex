package chunker

import (
	"bytes"
	"encoding/hex"
	"io"
	"strings"
	"sync"

	"golang.org/x/xerrors"

	"a.yandex-team.ru/cloud/compute/go-common/tracing"

	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/logging"

	werrors "github.com/pkg/errors"
	"go.uber.org/zap"
	"golang.org/x/net/context"

	log "a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/common"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/misc"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/storage"
)

const (
	maxBuffSize  = 64 * 1024 * 1024 // 64 Mb
	chunkerBlock = 256 * 1024
)

func chunkSize(size int) zap.Field {
	return zap.Int("chunk_size", size)
}

func getHashAlgFromSum(sum string) (string, error) {
	parts := strings.Split(sum, ":")
	if len(parts) != 2 {
		return "", xerrors.Errorf("failed to get hash name: %v", sum)
	}

	return parts[0], nil

}

func buildHashConstructorFromSum(sum string) (HashConstructor, error) {
	if alg, err := getHashAlgFromSum(sum); err == nil {
		return BuildHasher(alg)
	} else {
		return nil, err
	}
}

func decodeChunkBody(chunk *storage.LibChunk, encoded []byte) (decoded []byte, hashsum string, err error) {
	decoded = make([]byte, chunk.Size)
	hashsum, err = decodeChunkBodyBuffer(chunk, encoded, decoded)
	return
}

//nolint:errcheck
func decodeChunkBodyBuffer(chunk *storage.LibChunk, encoded, buffer []byte) (hashsum string, err error) {
	if chunk.Zero {
		t := misc.DecodeZeroChunkTimer.Start()
		defer t.ObserveDuration()
	} else {
		t := misc.DecodeDataChunkTimer.Start()
		defer t.ObserveDuration()
	}

	rc := bytes.NewReader(encoded)
	hc, err := buildHashConstructorFromSum(chunk.Sum)
	if err != nil {
		return
	}
	h := hc()

	dc, err := BuildDecompressor(chunk.Format)
	if err != nil {
		return
	}
	dec, err := dc(rc)
	if err != nil {
		return
	}
	defer dec.Close()

	_, err = io.ReadFull(io.TeeReader(dec, h), buffer)
	if err != nil {
		return
	}

	hashsum = h.Name + ":" + hex.EncodeToString(h.Sum(nil))
	return
}

func calculateSnapshotChecksumFromChunks(chunks storage.ChunkMap, size, chunkSize int64, currentAlg string) (string, error) {
	builder, err := BuildHasher(currentAlg)
	if err != nil {
		return "", err
	}

	h := builder()

	for off := int64(0); off < size; off += chunkSize {
		if c := chunks.Get(off); c != nil {
			if _, err := h.Write([]byte(c.Sum)); err != nil {
				return "", err
			}
		} else {
			if _, err := h.Write([]byte(zeroHash.Get(chunkSize, builder()))); err != nil {
				return "", err
			}
		}
	}

	checksum := h.Name + ":" + hex.EncodeToString(h.Sum(nil))
	return checksum, nil
}

type chunkReader struct {
	ctx    context.Context
	ch     *Chunker
	info   *common.SnapshotInfo
	offset int64
	io.Reader
}

func (r *chunkReader) Read(b []byte) (int, error) {
	if r.Reader == nil {
		if r.offset >= r.info.Size {
			return 0, io.EOF
		}
		data, err := r.ch.ReadChunkBody(r.ctx, r.info.ID, r.offset)
		if err != nil {
			log.G(r.ctx).Warn("chunkReader.Read fail with ReadChunkBody error", zap.Error(err), logging.SnapshotOffset(r.offset))
			return 0, err
		}
		r.Reader = bytes.NewReader(data)
	}

	n, err := r.Reader.Read(b)
	r.offset += int64(n)
	switch err {
	case io.EOF:
		if n == 0 {
			r.Reader = nil
			return r.Read(b)
		}
	case nil:
	default:
		log.G(r.ctx).Error("chunkReader.Read failed", zap.Error(err), logging.SnapshotOffset(r.offset))
		return n, werrors.WithStack(err)
	}
	return n, nil
}

func (r *chunkReader) Close() error {
	return nil
}

// StreamChunk is a chunk of data to be stored.
type StreamChunk struct {
	Offset   int64
	Data     []byte
	Progress float64
}

// ZeroStreamChunk is a chunk of zero data to be stored.
type ZeroStreamChunk struct {
	Offset   int64
	Size     int
	Progress float64
}

// StreamContext is a data stream to be stored.
type StreamContext struct {
	io.Reader

	ChunkSize int64
	Offset    int64
	Progress  float64
}

type multiWriter struct {
	writers []io.Writer
}

func (t *multiWriter) Write(p []byte) (n int, err error) {
	var wg sync.WaitGroup
	wg.Add(len(t.writers))
	errors := make([]error, len(t.writers))
	ns := make([]int, len(t.writers))

	for i, w := range t.writers {
		i, w := i, w
		go func() {
			defer wg.Done()
			j := 0
			for j < len(p) {
				r := j + chunkerBlock
				if r > len(p) {
					r = len(p)
				}
				n, err := w.Write(p[j:r])
				if n == 0 {
					err = io.ErrShortWrite
				}
				j += n
				errors[i] = err
				if err != nil {
					break
				}
			}
			ns[i] = j
		}()
	}
	wg.Wait()

	for i := range t.writers {
		if errors[i] != nil {
			return ns[i], errors[i]
		}

		if ns[i] != len(p) {
			return ns[i], io.ErrShortWrite
		}
	}

	return len(p), nil
}

// Chunker is a storage wrapper responsible for
// chunk compression, hashing and storing consistent chunk map,
type Chunker struct {
	st storage.Storage

	newHasher     HashConstructor
	newCompressor CompressorConstructor

	// used to store compressed user-data chunks
	pool sync.Pool
}

// NewChunker returns a new Chunker instance
func NewChunker(st storage.Storage, newHasher HashConstructor, newCompressor CompressorConstructor) *Chunker {
	return &Chunker{
		st:            st,
		newHasher:     newHasher,
		newCompressor: newCompressor,
		pool: sync.Pool{New: func() interface{} {
			return make([]byte, 0, storage.DefaultChunkSize)
		}},
	}
}

func (ch *Chunker) StoreChunkBlob(ctx context.Context, snapshotID string, c *StreamChunk) (res storage.LibChunk, resErr error) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, resErr) }()

	ctx = log.WithLogger(ctx, log.G(ctx).With(logging.SnapshotOffset(c.Offset)))

	// Avoid uncontrolled memory consumption
	if len(c.Data) > maxBuffSize {
		log.G(ctx).Error("StoreChunkBlob failed", zap.Error(misc.ErrChunkSizeTooBig),
			chunkSize(len(c.Data)))
		return res, misc.ErrChunkSizeTooBig
	}
	if c.Offset%int64(len(c.Data)) != 0 {
		log.G(ctx).Error("StoreChunkBlob failed", zap.Error(misc.ErrInvalidOffset), chunkSize(len(c.Data)))
		return res, misc.ErrInvalidOffset
	}

	// No need for hashing and compression of zeroes
	if isZero(c.Data) {
		return ch.StoreZeroChunkBlob(ctx, snapshotID, ZeroStreamChunk{
			Offset:   c.Offset,
			Size:     len(c.Data),
			Progress: c.Progress,
		})
	}

	t := misc.StoreDataChunkBlob.Start()

	h := ch.newHasher()
	tempBytes := ch.pool.Get().([]byte)
	defer ch.pool.Put(tempBytes)

	buff := bytes.NewBuffer(tempBytes)
	cmpss := ch.newCompressor(buff)

	nn, err := (&multiWriter{[]io.Writer{cmpss, h}}).Write(c.Data)
	if nn != len(c.Data) {
		err = werrors.WithMessage(err, "unable to Compress/Hashing chunk")
		log.G(ctx).Error("StoreChunkBlob failed", zap.Error(err))
		return res, xerrors.Errorf("unable to compress or hash chunk %d: %w", c.Offset, err)
	}

	res = storage.LibChunk{
		Sum:    h.Name + ":" + hex.EncodeToString(h.Sum(nil)),
		Format: cmpss.format,
		Size:   int64(len(c.Data)),
		Zero:   false,
		Offset: c.Offset,
	}

	if err = cmpss.Close(); err != nil {
		log.G(ctx).Error("Compressor.Close failed", zap.Error(err))
		return res, xerrors.Errorf("can't close compressor: %w", err)
	}

	resErr = ch.st.StoreChunkData(ctx, snapshotID, res.Offset, buff.Bytes())

	t.ObserveDuration()
	return
}

func (ch *Chunker) StoreZeroChunkBlob(ctx context.Context, snapshotID string, c ZeroStreamChunk) (res storage.LibChunk, resErr error) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, resErr) }()

	if c.Offset%int64(c.Size) != 0 {
		log.G(ctx).Error("StoreZeroChunkBlob failed", zap.Error(misc.ErrInvalidOffset), chunkSize(c.Size), logging.SnapshotOffset(c.Offset))
		return res, misc.ErrInvalidOffset
	}

	t := misc.StoreZeroChunkBlob.Start()

	res = storage.LibChunk{
		Sum:    zeroHash.Get(int64(c.Size), ch.newHasher()),
		Size:   int64(c.Size),
		Zero:   true,
		Format: storage.Raw,
		Offset: c.Offset,
	}

	t.ObserveDuration()
	return
}

// StoreChunk stores a (possibly zero) chunk
func (ch *Chunker) StoreChunk(ctx context.Context, snapshotID string, c *StreamChunk) (resErr error) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, resErr) }()

	ctx = log.WithLogger(ctx, log.G(ctx).With(logging.SnapshotOffset(c.Offset)))

	// Avoid uncontrolled memory consumption
	if len(c.Data) > maxBuffSize {
		log.G(ctx).Error("StoreChunk failed", zap.Error(misc.ErrChunkSizeTooBig),
			chunkSize(len(c.Data)))
		return misc.ErrChunkSizeTooBig
	}
	if c.Offset%int64(len(c.Data)) != 0 {
		log.G(ctx).Error("StoreChunk failed", zap.Error(misc.ErrInvalidOffset), chunkSize(len(c.Data)))
		return misc.ErrInvalidOffset
	}

	// No need for hashing and compression of zeroes
	if isZero(c.Data) {
		return ch.StoreZeroChunk(ctx, snapshotID, ZeroStreamChunk{
			Offset:   c.Offset,
			Size:     len(c.Data),
			Progress: c.Progress,
		})
	}

	t := misc.StoreDataChunkFull.Start()
	if err := ch.st.BeginChunk(ctx, snapshotID, c.Offset); err != nil {
		return err
	}

	h := ch.newHasher()
	buff := bytes.NewBuffer(make([]byte, 0, len(c.Data)))
	cmpss := ch.newCompressor(buff)

	nn, err := (&multiWriter{[]io.Writer{cmpss, h}}).Write(c.Data)
	if nn != len(c.Data) {
		err = werrors.WithMessage(err, "unable to Compress/Hashing chunk")
		log.G(ctx).Error("StoreChunk failed", zap.Error(err))
		return xerrors.Errorf("storechunk failed %d != %d: %w", nn, len(c.Data), err)
	}

	chunk := &storage.LibChunk{
		Sum:    h.Name + ":" + hex.EncodeToString(h.Sum(nil)),
		Format: cmpss.format,
		Size:   int64(len(c.Data)),
		Zero:   false,
	}

	if err = cmpss.Close(); err != nil {
		log.G(ctx).Error("Compressor.Close failed", zap.Error(err))
		return xerrors.Errorf("compressor.close failed: %w", err)
	}
	// TODO: Storage injects metadata to the chunk
	if err = ch.st.EndChunk(ctx, snapshotID, chunk, c.Offset, buff.Bytes(), c.Progress); err != nil {
		return err
	}

	t.ObserveDuration()
	return nil
}

// StoreZeroChunk stores a zero chunk
func (ch *Chunker) StoreZeroChunk(ctx context.Context, snapshotID string, c ZeroStreamChunk) (resErr error) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, resErr) }()

	ctx = log.WithLogger(ctx, log.G(ctx).With(logging.SnapshotOffset(c.Offset)))

	// Avoid uncontrolled memory consumption
	if c.Offset%int64(c.Size) != 0 {
		log.G(ctx).Error("StoreZeroChunk failed", zap.Error(misc.ErrInvalidOffset), chunkSize(c.Size))
		return misc.ErrInvalidOffset
	}

	t := misc.StoreZeroChunkFull.Start()
	if err := ch.st.BeginChunk(ctx, snapshotID, c.Offset); err != nil {
		return err
	}

	chunk := &storage.LibChunk{
		Sum:    zeroHash.Get(int64(c.Size), ch.newHasher()),
		Size:   int64(c.Size),
		Zero:   true,
		Format: storage.Raw,
	}

	// TODO: Storage injects metadata to the chunk
	if err := ch.st.EndChunk(ctx, snapshotID, chunk, c.Offset, nil, c.Progress); err != nil {
		return err
	}

	t.ObserveDuration()
	return nil
}

// StoreFromStream stores chunks from data stream.
// Deprecated.
func (ch *Chunker) StoreFromStream(ctx context.Context, snapshotID string, strctx StreamContext) (count int, e error) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, e) }()

	chunks := 0
	// Avoid uncontrolled memory consumption
	if strctx.ChunkSize > maxBuffSize {
		log.G(ctx).Error("StoreFromStream failed", zap.Error(misc.ErrChunkSizeTooBig), chunkSize(int(strctx.ChunkSize)),
			logging.SnapshotOffset(strctx.Offset))
		return 0, misc.ErrChunkSizeTooBig
	}
	if strctx.Offset%strctx.ChunkSize != 0 {
		log.G(ctx).Error("StoreFromStream failed", zap.Error(misc.ErrInvalidOffset), chunkSize(int(strctx.ChunkSize)),
			logging.SnapshotOffset(strctx.Offset))
		return 0, misc.ErrInvalidOffset
	}
	t := misc.StoreFromStream.Start()
	defer t.ObserveDuration()

	buff := make([]byte, strctx.ChunkSize)
	// Store as much chunks as we can read until EOF.
	// Alignment required: TotalSize = ChunkSize * Count.
	// If the last chunk is not large enough, it's dropped and the func returns error
	for ; ; chunks++ {
		offset := strctx.Offset + strctx.ChunkSize*int64(chunks)
		_, err := io.ReadFull(strctx.Reader, buff)
		switch err {
		case nil:
		case io.EOF:
			return chunks, nil
		case io.ErrUnexpectedEOF:
			log.G(ctx).Error("StoreFromStream failed", zap.Error(misc.ErrSmallChunk), logging.SnapshotOffset(offset))
			return chunks, misc.ErrSmallChunk
		default:
			log.G(ctx).Error("StoreFromStream failed", zap.Error(err), logging.SnapshotOffset(offset))
			return chunks, err
		}

		err = ch.StoreChunk(ctx, snapshotID, &StreamChunk{
			Offset:   offset,
			Data:     buff,
			Progress: strctx.Progress,
		})
		if err != nil {
			log.G(ctx).Error("Store chunk.", zap.Error(err), logging.SnapshotOffset(offset))
			return chunks, err
		}
	}
}

// GetReader returns a stream reader for snapshot.
// Deprecated.
func (ch *Chunker) GetReader(ctx context.Context, id string, offset int64) (rc io.ReadCloser, err error) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, err) }()

	var info *common.SnapshotInfo
	if info, err = ch.st.GetSnapshot(ctx, id); err != nil {
		log.G(ctx).Error("Can't get snapshot", zap.Error(err))
		return nil, err
	}
	// To preserve old behavior
	_, err = ch.ReadChunkBody(ctx, id, offset)
	if err != nil {
		log.G(ctx).Error("Can't read chunk", zap.Error(err))
		return nil, err
	}
	return &chunkReader{ctx: ctx, ch: ch, info: info, offset: offset}, nil
}

// ReadChunkBodyBuffer reads a chunk to provided buffer if chunk is not zero.
func (ch *Chunker) ReadChunkBodyBuffer(
	ctx context.Context,
	id string,
	offset int64,
	buffer []byte,
) (zero bool, err error) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, err) }()

	ctx = log.WithLogger(ctx, log.G(ctx).With(logging.SnapshotOffset(offset)))

	t1 := misc.ReadZeroChunkTimer.Start()
	t2 := misc.ReadDataChunkTimer.Start()
	chunk, encoded, err := ch.st.ReadChunkBody(ctx, id, offset)
	if err != nil {
		return false, err
	}
	if chunk == nil {
		return true, nil
	}

	hashsum, err := decodeChunkBodyBuffer(chunk, encoded, buffer)
	if err != nil {
		log.G(ctx).Error("decodeChunkBody failed", zap.Error(err))
		return false, xerrors.Errorf("decodeChunkBody: %w", err)
	}
	if chunk.Sum != hashsum {
		log.G(ctx).Error("data corruption detected %s %s",
			zap.String("actual", hashsum), zap.String("expected", chunk.Sum))
		return false, misc.ErrCorruptedSource
	}

	if chunk.Zero {
		t1.ObserveDuration()
	} else {
		t2.ObserveDuration()
	}

	return chunk.Zero, nil
}

// ReadChunkBody reads a chunk.
func (ch *Chunker) ReadChunkBody(ctx context.Context, id string, offset int64) (data []byte, resErr error) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, resErr) }()

	ctx = log.WithLogger(ctx, log.G(ctx).With(logging.SnapshotOffset(offset)))

	t1 := misc.ReadZeroChunkTimer.Start()
	t2 := misc.ReadDataChunkTimer.Start()
	chunk, encoded, err := ch.st.ReadChunkBody(ctx, id, offset)
	if err != nil {
		return nil, err
	}
	if chunk == nil {
		return encoded, nil
	}

	data, hashsum, err := decodeChunkBody(chunk, encoded)
	if err != nil {
		log.G(ctx).Error("decodeChunkBody failed", zap.Error(err))
		return nil, xerrors.Errorf("decodeChunkBody failed: %w", err)
	}
	if chunk.Sum != hashsum {
		log.G(ctx).Error("data corruption detected %s %s",
			zap.String("actual", hashsum), zap.String("expected", chunk.Sum))
		return nil, misc.ErrCorruptedSource
	}

	if chunk.Zero {
		t1.ObserveDuration()
	} else {
		t2.ObserveDuration()
	}

	return data, nil
}

// GetSnapshotChunksWithOffset returns a list of chunks with holes filled with zero chunks.
func (ch *Chunker) GetSnapshotChunksWithOffset(ctx context.Context, id string, offset int64, limit int) (resInfo []common.ChunkInfo, resErr error) {
	span, ctx := tracing.StartSpanFromContextFunc(ctx)
	defer func() { tracing.Finish(span, resErr) }()

	sh, err := ch.st.GetSnapshotFull(ctx, id)
	if err != nil {
		return nil, err
	}

	if err = storage.CheckReadableState(ctx, sh.State.Code); err != nil {
		return nil, err
	}

	chunks, err := ch.st.GetChunksFromCache(ctx, id, false)
	if err != nil {
		return nil, err
	}

	result := make([]common.ChunkInfo, 0, limit)

	firstChunkOffset := (offset + sh.ChunkSize) / sh.ChunkSize * sh.ChunkSize
	lastChunkOffset := offset + sh.ChunkSize*int64(limit)
	for off := firstChunkOffset; off <= lastChunkOffset && off < sh.Size; off += sh.ChunkSize {
		if chunk := chunks.Get(off); chunk != nil {
			result = append(result, common.ChunkInfo{
				Hashsum: chunk.Sum,
				Offset:  off,
				Size:    chunk.Size,
				Zero:    chunk.Zero,
			})
		} else {
			// No match, add null
			result = append(result, common.ChunkInfo{
				Hashsum: zeroHash.Get(sh.ChunkSize, ch.newHasher()),
				Offset:  off,
				Size:    sh.ChunkSize,
				Zero:    true,
			})
		}
	}

	return result, nil
}
