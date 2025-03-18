package jwtauth

import (
	"net/http"

	"github.com/golang-jwt/jwt/v4"
	jwtreq "github.com/golang-jwt/jwt/v4/request"

	"a.yandex-team.ru/library/go/core/log"
)

type MiddlewareOpt func(*middleware)

// WithKeyFunc sets function to specify verification key to current token.
// This option is required.
func WithKeyFunc(fn jwt.Keyfunc) MiddlewareOpt {
	return func(mw *middleware) {
		mw.keyFunc = fn
	}
}

// WithSigningMethod sets expected token signing method.
// This option is required.
func WithSigningMethod(m jwt.SigningMethod) MiddlewareOpt {
	return func(mw *middleware) {
		mw.signingMethod = m
	}
}

// WithClaims sets function that returns custom JWT claims to unmarshal payload to.
// If none given - jwt.MapClaims used by default.
func WithClaims(fn func() jwt.Claims) MiddlewareOpt {
	return func(mw *middleware) {
		mw.claimsFunc = fn
	}
}

// WithLogger sets custom logger to middleware.
// If none given - nop.Logger used by default.
func WithLogger(l log.Structured) MiddlewareOpt {
	return func(mw *middleware) {
		mw.l = l
	}
}

// WithExtractor sets extractors that searches token in FIFO order.
// If none given - HeaderExtractor{"Authorization"} used by default.
func WithExtractor(ex ...jwtreq.Extractor) MiddlewareOpt {
	return func(mw *middleware) {
		mw.extractors = ex
	}
}

// OnError sets callback function to intercept token parsing error and modify HTTP request/response objects.
// By default middleware will stop request processing with 401 HTTP status code on any error.
func OnError(fn func(http.ResponseWriter, *http.Request, error)) MiddlewareOpt {
	return func(mw *middleware) {
		mw.onError = fn
	}
}
