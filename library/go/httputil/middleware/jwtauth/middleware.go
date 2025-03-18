package jwtauth

import (
	"context"
	"net/http"

	"github.com/golang-jwt/jwt/v4"
	jwtreq "github.com/golang-jwt/jwt/v4/request"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/nop"
	"a.yandex-team.ru/library/go/httputil/headers"
)

type ctxKey string

const (
	// key to extract parsed JWT token from request context
	TokenCtxKey = ctxKey("jwtToken")
)

type middleware struct {
	keyFunc       jwt.Keyfunc
	signingMethod jwt.SigningMethod
	claimsFunc    func() jwt.Claims
	l             log.Structured
	extractors    jwtreq.MultiExtractor
	onError       func(http.ResponseWriter, *http.Request, error)
}

// VerifyToken returns middleware that parses and verifies JWT token from HTTP request.
func VerifyToken(opts ...MiddlewareOpt) func(next http.Handler) http.Handler {
	mw := middleware{
		claimsFunc: func() jwt.Claims {
			return make(jwt.MapClaims)
		},
		extractors: jwtreq.MultiExtractor{
			jwtreq.HeaderExtractor{headers.AuthorizationKey},
		},
		onError: func(w http.ResponseWriter, _ *http.Request, _ error) {
			w.WriteHeader(http.StatusUnauthorized)
		},
		l: new(nop.Logger),
	}

	for _, opt := range opts {
		opt(&mw)
	}

	if mw.signingMethod == nil {
		panic("signing method required")
	}
	if mw.keyFunc == nil {
		panic("key function required")
	}

	return mw.wrap
}

func (mw *middleware) wrap(next http.Handler) http.Handler {
	parser := jwt.Parser{ValidMethods: []string{mw.signingMethod.Alg()}}

	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		tokenString, err := mw.extractors.ExtractToken(r)
		if err != nil {
			mw.onError(w, r, err)
			return
		}

		token, err := parser.ParseWithClaims(tokenString, mw.claimsFunc(), mw.keyFunc)
		if err != nil {
			mw.onError(w, r, err)
			return
		}

		ctx := context.WithValue(r.Context(), TokenCtxKey, token)
		next.ServeHTTP(w, r.WithContext(ctx))
	})
}
