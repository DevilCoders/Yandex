package oauth

import (
	"io"
	"time"
)

func WithRandReader(reader io.Reader) Option {
	return func(opts *options) error {
		opts.randReader = reader
		return nil
	}
}

func WithTime(t time.Time) Option {
	return func(opts *options) error {
		opts.time = t
		return nil
	}
}
