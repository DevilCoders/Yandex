package grpcinterceptors

import (
	"strings"

	"google.golang.org/grpc/metadata"
)

type MetadataReaderWriter struct {
	metadata.MD
}

func (w MetadataReaderWriter) Set(key, val string) {
	// The GRPC HPACK implementation rejects any uppercase keys here.
	//
	// As such, since the HTTP_HEADERS format is case-insensitive anyway, we
	// blindly lowercase the key (which is guaranteed to work in the
	// Inject/Extract sense per the OpenTracing spec).
	key = strings.ToLower(key)
	w.MD[key] = append(w.MD[key], val)
}

func (w MetadataReaderWriter) ForeachKey(handler func(key, val string) error) error {
	for k, vals := range w.MD {
		for _, v := range vals {
			if err := handler(k, v); err != nil {
				return err
			}
		}
	}

	return nil
}
