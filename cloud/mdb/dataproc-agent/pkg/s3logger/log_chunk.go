package s3logger

import (
	"io"
	"sync"
)

// LogChunk represents fixed-sized chunk of log.
type LogChunk struct {
	index          int        // chunk serial number
	buffer         []byte     // data stored in this chunk
	offset         int        // how many bytes written to this chunk
	uploadedOffset int        // how many bytes were synced
	mutex          sync.Mutex // protects changes to LogChunk's data
	full           bool       // there's no more free space in chunk
	dirty          bool       // there's new data that has not been synced
	closed         bool       // full and synced
}

// NewChunk creates new chunk with current offset and index
func NewChunk(index int, size int) *LogChunk {
	return &LogChunk{
		index:  index,
		buffer: make([]byte, size),
		offset: 0,
	}
}

// asString returns buffer content as string.
func (chunk *LogChunk) asString() string {
	return string(chunk.buffer[:chunk.offset])
}

// updateDirty updates dirty flag.
func (chunk *LogChunk) updateDirty() {
	chunk.dirty = chunk.offset > chunk.uploadedOffset
}

// revisionUploaded must be called after chunk was successfully saved
// to storage; updates internal state.
func (chunk *LogChunk) revisionUploaded(offset int) {
	chunk.mutex.Lock()
	defer chunk.mutex.Unlock()

	chunk.uploadedOffset = offset
	chunk.updateDirty()
	chunk.closed = chunk.full && !chunk.dirty
}

// buildUploadRequest builds request to be sent to storage.
func (chunk *LogChunk) buildUploadRequest() (index int, content []byte, offset int) {
	chunk.mutex.Lock()
	defer chunk.mutex.Unlock()

	offset = chunk.offset
	index = chunk.index
	content = chunk.buffer[:chunk.offset]
	return
}

// bufferUpdated must be called after new data was written to chunk's buffer.
// Updates internal state.
func (chunk *LogChunk) bufferUpdated(n int) {
	chunk.offset += n
	chunk.full = chunk.offset == cap(chunk.buffer)
	if n > 0 {
		chunk.updateDirty()
	}
}

// ReadFrom reads data from reader until it ends or chunk is full
func (chunk *LogChunk) ReadFrom(reader io.Reader) (int64, error) {
	chunk.mutex.Lock()
	defer chunk.mutex.Unlock()

	n, err := reader.Read(chunk.buffer[chunk.offset:])
	chunk.bufferUpdated(n)
	return int64(n), err
}
