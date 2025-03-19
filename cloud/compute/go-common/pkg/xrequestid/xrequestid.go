package xrequestid

import (
	"context"
	"net/http"

	"github.com/gofrs/uuid"
	"google.golang.org/grpc/metadata"

	"golang.org/x/xerrors"
)

type requestIDKey struct{}

const requestIDHeader = "x-request-id"

//nolint:errcheck
func NewRequestID() string {
	//nolint:errcheck
	u, _ := uuid.NewV4()
	return u.String()
}

func WithRequestID(ctx context.Context, requestID string) context.Context {
	return context.WithValue(ctx, requestIDKey{}, requestID)
}

func WithNewRequestID(ctx context.Context) (context.Context, string) {
	requestID := NewRequestID()
	return WithRequestID(ctx, requestID), requestID
}

func FromIncomingContext(ctx context.Context) (context.Context, string) {
	md, ok := metadata.FromIncomingContext(ctx)
	if !ok {
		panic("No metadata found")
	}

	requestIDs := md.Get(requestIDHeader)

	if len(requestIDs) != 1 {
		requestIDs = []string{NewRequestID()}
	}

	return WithRequestID(ctx, requestIDs[0]), requestIDs[0]
}

//nolint:golint
func FromHTTPRequest(ctx context.Context, req *http.Request) (context.Context, string) {
	requestID := req.Header.Get(requestIDHeader)
	if requestID == "" {
		requestID = NewRequestID()
	}

	return WithRequestID(ctx, requestID), requestID
}

func AppendToOutgoingContext(ctx context.Context) context.Context {
	requestID, ok := FromContext(ctx)
	if !ok {
		panic("requestID not found in context")
	}

	return metadata.AppendToOutgoingContext(ctx, requestIDHeader, requestID)
}

// SetHeader set header with requestID from ctx
// req must have initialized (not nil) Header field
// return true if context contains request id
func SetHeader(ctx context.Context, req *http.Request) bool {
	requestID, ok := FromContext(ctx)
	if !ok {
		return false
	}
	req.Header.Set(requestIDHeader, requestID)
	return true
}

// SetHeader set header with requestID from ctx
// req must have initialized (not nil) Header field
// panic if context haven't requestID
func MustSetHeader(ctx context.Context, req *http.Request) {
	if !SetHeader(ctx, req) {
		panic(xerrors.New("xrequestid: context doesn't contain request id"))
	}
}

// MustFromContext
// panic if context haven't requestID
func MustFromContext(ctx context.Context) string {
	requestID, ok := FromContext(ctx)
	if !ok {
		panic("requestID not found in context")
	}

	return requestID
}

func FromContext(ctx context.Context) (string, bool) {
	requestID, ok := ctx.Value(requestIDKey{}).(string)
	return requestID, ok
}
