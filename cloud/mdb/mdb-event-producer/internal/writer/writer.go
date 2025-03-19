package writer

import (
	"time"
)

// WriteResponse is write response for your write.
// Err == nil is success write.
type WriteResponse struct {
	ID  int64
	Err error
}

// Writer is tiny wrapper around persqueue.Writer
type Writer interface {
	// Write message with given id
	Write(id int64, message []byte, createdAt time.Time) error
	// WriteResponses returns a channel emitting writer feedback messages.
	WriteResponses() <-chan WriteResponse
}
