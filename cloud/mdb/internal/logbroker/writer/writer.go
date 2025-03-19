package writer

import (
	"fmt"
	"time"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/nop"
)

// WriteResponse is write response for your write.
// Err == nil is success write.
type WriteResponse struct {
	ID  int64
	Err error
}

//go:generate ../../../scripts/mockgen.sh Writer

// Writer is tiny wrapper around persqueue.Writer
type Writer interface {
	// Write message with given id
	Write(id int64, message []byte, createdAt time.Time) error
	// WriteResponses returns a channel emitting writer feedback messages.
	WriteResponses() <-chan WriteResponse
}

type Doc struct {
	ID        int64
	Data      []byte
	CreatedAt time.Time
}

type WriteAndWaitOptions struct {
	L               log.Logger
	FeedbackTimeout time.Duration
}

type WriteAndWaitOption func(*WriteAndWaitOptions)

func FeedbackTimeout(tm time.Duration) WriteAndWaitOption {
	return func(args *WriteAndWaitOptions) {
		args.FeedbackTimeout = tm
	}
}

func Logger(l log.Logger) WriteAndWaitOption {
	return func(args *WriteAndWaitOptions) {
		args.L = l
	}
}

type WriteAndWaitFunc func(w Writer, docs []Doc, opts ...WriteAndWaitOption) ([]int64, error)

var _ WriteAndWaitFunc = WriteAndWait

// WriteAndWait sent docs to writer and wait until them written
func WriteAndWait(writer Writer, docs []Doc, opts ...WriteAndWaitOption) ([]int64, error) {
	o := &WriteAndWaitOptions{
		L:               &nop.Logger{},
		FeedbackTimeout: time.Minute,
	}
	for _, optSetter := range opts {
		optSetter(o)
	}
	if len(docs) == 0 {
		return nil, nil
	}

	var written int
	for _, d := range docs {
		if err := writer.Write(d.ID, d.Data, d.CreatedAt); err != nil {
			writeError := fmt.Errorf("unable to write %d doc to LB: %w", d.ID, err)
			if written == 0 {
				return nil, writeError
			}
			// don't simple exit at that point, cause we write some docs.
			// We should receive feedback for them.
			o.L.Warnf("%s. Try receive feedback for %d written docs", writeError, written)
			break
		}
		written++
	}

	o.L.Debugf("write %d docs to LB", written)

	successSent := make([]int64, 0)
	feedbackChan := writer.WriteResponses()
	var feedbackErr error
	timer := time.NewTimer(o.FeedbackTimeout)
	for i := 0; i < written; i++ {
		select {
		case rsp := <-feedbackChan:
			if rsp.Err != nil {
				o.L.Warnf("got error for %d doc: %s", rsp.ID, rsp.Err)
				feedbackErr = rsp.Err
				continue
			}
			o.L.Debugf("got success feedback for %d doc", rsp.ID)
			successSent = append(successSent, rsp.ID)
		case <-timer.C:
			return nil, fmt.Errorf("didn't get feedback in %s timeout", o.FeedbackTimeout)
		}
	}

	if len(successSent) > 0 {
		o.L.Infof(
			"successfully sent %d docs from %d",
			len(successSent),
			len(docs),
		)
		return successSent, nil
	}

	return nil, fmt.Errorf("all %d document was unsent: %w", written, feedbackErr)
}
