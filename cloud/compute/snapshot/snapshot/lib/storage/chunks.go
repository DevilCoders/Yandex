package storage

// ChunkFormat is a type of a chunk compression format
type ChunkFormat string

const (
	// GZip compression.
	GZip ChunkFormat = "gzip"
	// Raw (nop) format.
	Raw ChunkFormat = "raw"
	// Lz4 compression.
	Lz4 ChunkFormat = "lz4"

	// DefaultChunkSize is the default size of snapshot chunks.
	DefaultChunkSize = 4 * 1024 * 1024
)

// IsValidFormat checks format validity.
func IsValidFormat(fmt ChunkFormat) bool {
	return fmt == GZip || fmt == Raw || fmt == Lz4
}

// LibChunk is a an elementary data unit
type LibChunk struct {
	ID     string
	Sum    string // Sum is hash Sum of a raw (non-compressed) chunk
	Format ChunkFormat
	Size   int64
	Offset int64
	Zero   bool
}

// ChunkMap must support next operations:
// * Find next chunk with offset and further
// * Insert information about chunk effectively, even in the middle
// * Should be fast serializable
type ChunkMap struct {
	store map[int64]*LibChunk
}

// NewChunkMap returns an empty chunk map.
func NewChunkMap() ChunkMap {
	return ChunkMap{store: make(map[int64]*LibChunk)}
}

// Insert adds chunk to map by offset.
func (c ChunkMap) Insert(offset int64, chunk *LibChunk) {
	c.store[offset] = chunk
}

// Has checks chunk presence in map.
func (c ChunkMap) Has(offset int64) bool {
	_, ok := c.store[offset]
	return ok
}

// Get returns a chunk by offset.
func (c ChunkMap) Get(offset int64) *LibChunk {
	return c.store[offset]
}

// Len returns number of chunks in map.
func (c ChunkMap) Len() int {
	return len(c.store)
}

// Callback is a chunk operation.
type Callback func(int64, *LibChunk) error

// Foreach applies a given Callback to each chunk.
// The order is not determined
func (c ChunkMap) Foreach(f Callback) error {
	var err error
	for k, v := range c.store {
		err = f(k, v)
		if err != nil {
			return err
		}
	}
	return nil
}

// ChunkRef is a chunk weak reference.
type ChunkRef struct {
	Offset  int64
	ChunkID string
}
