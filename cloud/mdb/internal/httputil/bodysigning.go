package httputil

import (
	"bytes"
	"context"
	"encoding/base64"
	"io/ioutil"
	"net/http"

	"a.yandex-team.ru/cloud/mdb/internal/crypto"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

// Errors
var (
	ErrRequestBodySignFailed = xerrors.NewSentinel("failed to sign request body")
	ErrVerifyBodySignFailed  = xerrors.NewSentinel("failed to verify body")
)

type ctxValueType int

const (
	ctxRequestBody ctxValueType = iota
)

// ContextWithRequestBody adds request body to context
func ContextWithRequestBody(ctx context.Context, body []byte) context.Context {
	return context.WithValue(ctx, ctxRequestBody, body)
}

// RequestBodyFromContext retrieves request body from context if it exists, returns nil otherwise
func RequestBodyFromContext(ctx context.Context) []byte {
	if body, ok := ctx.Value(ctxRequestBody).([]byte); ok {
		return body
	}

	return nil
}

var (
	// SignatureHeader is a header for request body signature
	SignatureHeader = http.CanonicalHeaderKey("X-Signature")
)

// SigningRoundTripper implements signing of outgoing request body
type SigningRoundTripper struct {
	l         log.Logger
	transport http.RoundTripper
	key       *crypto.PrivateKey
}

// NewRoundSigningTripper ...
func NewRoundSigningTripper(
	transport http.RoundTripper,
	key *crypto.PrivateKey,
	l log.Logger,
) (*SigningRoundTripper, error) {
	if key == nil {
		return nil, xerrors.New("nil key in RoundSigningTripper")
	}
	return &SigningRoundTripper{
		l:         l,
		transport: transport,
		key:       key,
	}, nil
}

// RoundTrip performs signing of request body
func (srt *SigningRoundTripper) RoundTrip(r *http.Request) (*http.Response, error) {
	if r.Body == nil {
		return srt.transport.RoundTrip(r)
	}

	// TODO do not sign everything
	var buf bytes.Buffer
	if _, err := buf.ReadFrom(r.Body); err != nil {
		return nil, ErrRequestBodySignFailed.Wrap(xerrors.Errorf("request body not read: %w", err))
	}

	if err := r.Body.Close(); err != nil {
		return nil, ErrRequestBodySignFailed.Wrap(xerrors.Errorf("request body not closed: %w", err))
	}

	// Replace body with new buffer
	r.Body = ioutil.NopCloser(bytes.NewReader(buf.Bytes()))

	signature, err := srt.key.HashAndSign(buf.Bytes())
	if err != nil {
		return nil, ErrRequestBodySignFailed.Wrap(err)
	}

	base64Signature := base64.StdEncoding.EncodeToString(signature)

	// Add signature header
	ctxlog.Debugf(r.Context(), srt.l, "signing roundtripper: adding signature header %q: %s", SignatureHeader, base64Signature)
	r.Header[SignatureHeader] = append(r.Header[SignatureHeader], base64Signature)

	return srt.transport.RoundTrip(r)
}

// RequestBodySavingMiddleware saves request body in a header so that handler can verify its signature
func RequestBodySavingMiddleware(next http.Handler, l log.Logger) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		signatures := r.Header[SignatureHeader]

		if len(signatures) == 0 || r.Body == http.NoBody {
			ctxlog.Debug(r.Context(), l, "No SignatureHeader found in request")
			next.ServeHTTP(w, r)
			return
		}

		ctxlog.Debugf(r.Context(), l, "Signature header %q found in request. Preparing body verification...", SignatureHeader)
		var buf bytes.Buffer
		if _, err := buf.ReadFrom(r.Body); err != nil {
			// Failed to read request body, log error and report 500 to the user
			// This should never happen so 500 is ok if it will actually happen
			ctxlog.Errorf(r.Context(), l, "Request body not read: %s", err)
			http.Error(w, "Failed to verify request body.", http.StatusInternalServerError)
			return
		}

		if err := r.Body.Close(); err != nil {
			// Failed to close request body, log error and report 500 to the user
			// This should never happen so 500 is ok if it will actually happen
			ctxlog.Errorf(r.Context(), l, "Request body not closed: %s", err)
			http.Error(w, "Failed to verify request body.", http.StatusInternalServerError)
			return
		}

		// Add request body to context
		r = r.WithContext(ContextWithRequestBody(r.Context(), buf.Bytes()))
		// Replace body with new buffer
		r.Body = ioutil.NopCloser(bytes.NewReader(buf.Bytes()))

		next.ServeHTTP(w, r)
	})
}

// VerifyBodySignature verify signature by public key
func VerifyBodySignature(
	ctx context.Context,
	pubkey []byte,
	body []byte,
	base64Signature string,
) error {
	signature, err := base64.StdEncoding.DecodeString(base64Signature)
	if err != nil {
		return ErrVerifyBodySignFailed.Wrap(xerrors.Errorf("base64 signature %q not decoded: %w", base64Signature, err))
	}

	key, err := crypto.NewPublicKey(pubkey)
	if err != nil {
		return ErrVerifyBodySignFailed.Wrap(xerrors.Errorf("cluster public key not parsed: %w", err))
	}

	if err = key.HashAndVerify(body, signature); err != nil {
		return ErrVerifyBodySignFailed.Wrap(err)
	}

	return nil
}
