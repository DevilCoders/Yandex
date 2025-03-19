package dummy

import (
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/logbroker/writer"
)

type Writer struct {
	responses      chan writer.WriteResponse
	WriteErrors    map[int64]error
	FeedbackErrors map[int64]error
}

var _ writer.Writer = &Writer{}

func New() *Writer {
	return &Writer{
		responses:      make(chan writer.WriteResponse, 100),
		WriteErrors:    make(map[int64]error),
		FeedbackErrors: make(map[int64]error),
	}
}

func (wm *Writer) Write(id int64, message []byte, createdAt time.Time) error {
	if writeErr, ok := wm.WriteErrors[id]; ok {
		return writeErr
	}
	if feedbackErr, ok := wm.FeedbackErrors[id]; ok {
		wm.responses <- writer.WriteResponse{ID: id, Err: feedbackErr}
		return nil
	}
	wm.responses <- writer.WriteResponse{ID: id, Err: nil}
	return nil
}

func (wm *Writer) WriteResponses() <-chan writer.WriteResponse {
	return wm.responses
}
