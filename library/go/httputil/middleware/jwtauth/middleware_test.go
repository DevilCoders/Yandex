package jwtauth

import (
	"net/http"
	"net/http/httptest"
	"strings"
	"testing"

	"github.com/go-resty/resty/v2"
	"github.com/golang-jwt/jwt/v4"
	jwtreq "github.com/golang-jwt/jwt/v4/request"
	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/library/go/httputil/headers"
)

func TestVerifyToken(t *testing.T) {
	testCases := []struct {
		name        string
		opts        []MiddlewareOpt
		expectPanic bool
	}{
		{
			"default",
			nil,
			true,
		},
		{
			"signing_method_given",
			[]MiddlewareOpt{
				WithSigningMethod(jwt.SigningMethodHS512),
			},
			true,
		},
		{
			"key_func_given",
			[]MiddlewareOpt{
				WithKeyFunc(func(_ *jwt.Token) (i interface{}, e error) {
					return []byte("my_secret"), nil
				}),
			},
			true,
		},
		{
			"success",
			[]MiddlewareOpt{
				WithSigningMethod(jwt.SigningMethodHS512),
				WithKeyFunc(func(_ *jwt.Token) (i interface{}, e error) {
					return []byte("my_secret"), nil
				}),
			},
			false,
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			if tc.expectPanic {
				assert.Panics(t, func() { VerifyToken(tc.opts...) })
			} else {
				assert.NotPanics(t, func() { VerifyToken(tc.opts...) })
			}
		})
	}
}

func TestVerifyToken_wrap(t *testing.T) {
	t.Run("empty_token", func(t *testing.T) {
		var onErrorCalled bool

		mw := VerifyToken(
			WithSigningMethod(jwt.SigningMethodHS512),
			WithKeyFunc(func(_ *jwt.Token) (i interface{}, e error) {
				return []byte("my_secret"), nil
			}),
			OnError(func(w http.ResponseWriter, r *http.Request, err error) {
				assert.EqualError(t, err, jwtreq.ErrNoTokenInRequest.Error())
				onErrorCalled = true
				w.WriteHeader(http.StatusUnauthorized)
			}),
		)

		handler := http.HandlerFunc(func(_ http.ResponseWriter, r *http.Request) {})

		srv := httptest.NewServer(mw(handler))
		defer srv.Close()

		resp, err := resty.New().
			SetBaseURL(srv.URL).
			R().
			Get("/")

		assert.NoError(t, err)
		assert.Equal(t, http.StatusUnauthorized, resp.StatusCode())
		assert.True(t, onErrorCalled)
	})

	t.Run("invalid_signing_method", func(t *testing.T) {
		var onErrorCalled bool

		mw := VerifyToken(
			WithSigningMethod(jwt.SigningMethodHS512),
			WithKeyFunc(func(_ *jwt.Token) (i interface{}, e error) {
				return []byte("ololo"), nil
			}),
			OnError(func(w http.ResponseWriter, r *http.Request, err error) {
				assert.EqualError(t, err, "signing method HS256 is invalid")
				onErrorCalled = true
				w.WriteHeader(http.StatusUnauthorized)
			}),
		)

		handler := http.HandlerFunc(func(_ http.ResponseWriter, r *http.Request) {})

		srv := httptest.NewServer(mw(handler))
		defer srv.Close()

		token := jwt.New(jwt.SigningMethodHS256)
		signed, err := token.SignedString([]byte("ololo"))
		assert.NoError(t, err)

		resp, err := resty.New().
			SetBaseURL(srv.URL).
			R().
			SetHeader(headers.AuthorizationKey, signed).
			Get("/")

		assert.NoError(t, err)
		assert.Equal(t, http.StatusUnauthorized, resp.StatusCode())
		assert.True(t, onErrorCalled)
	})

	t.Run("invalid_signature", func(t *testing.T) {
		var onErrorCalled bool

		mw := VerifyToken(
			WithSigningMethod(jwt.SigningMethodHS512),
			WithKeyFunc(func(_ *jwt.Token) (i interface{}, e error) {
				return []byte("ololo"), nil
			}),
			OnError(func(w http.ResponseWriter, r *http.Request, err error) {
				assert.EqualError(t, err, "signature is invalid")
				onErrorCalled = true
				w.WriteHeader(http.StatusUnauthorized)
			}),
		)

		handler := http.HandlerFunc(func(_ http.ResponseWriter, r *http.Request) {})

		srv := httptest.NewServer(mw(handler))
		defer srv.Close()

		token := jwt.New(jwt.SigningMethodHS512)
		signed, err := token.SignedString([]byte("trololo"))
		assert.NoError(t, err)

		resp, err := resty.New().
			SetBaseURL(srv.URL).
			R().
			SetHeader(headers.AuthorizationKey, signed).
			Get("/")

		assert.NoError(t, err)
		assert.Equal(t, http.StatusUnauthorized, resp.StatusCode())
		assert.True(t, onErrorCalled)
	})

	t.Run("valid_token", func(t *testing.T) {
		var onErrorCalled, handlerCalled bool

		token := jwt.NewWithClaims(jwt.SigningMethodHS512, jwt.MapClaims{
			"name": "ololoshka",
		})
		signed, err := token.SignedString([]byte("ololo"))
		assert.NoError(t, err)

		mw := VerifyToken(
			WithSigningMethod(jwt.SigningMethodHS512),
			WithKeyFunc(func(_ *jwt.Token) (i interface{}, e error) {
				return []byte("ololo"), nil
			}),
			OnError(func(w http.ResponseWriter, r *http.Request, err error) {
				onErrorCalled = true
				w.WriteHeader(http.StatusUnauthorized)
			}),
		)

		handler := http.HandlerFunc(func(_ http.ResponseWriter, r *http.Request) {
			expectedToken := &jwt.Token{
				Raw:    signed,
				Method: jwt.SigningMethodHS512,
				Header: token.Header,
				Claims: jwt.MapClaims{"name": "ololoshka"},
				Signature: func() string {
					parts := strings.Split(signed, ".")
					return parts[2]
				}(),
				Valid: true,
			}

			ctxToken := r.Context().Value(TokenCtxKey)
			assert.Equal(t, expectedToken, ctxToken)
			handlerCalled = true
		})

		srv := httptest.NewServer(mw(handler))
		defer srv.Close()

		resp, err := resty.New().
			SetBaseURL(srv.URL).
			R().
			SetHeader(headers.AuthorizationKey, signed).
			Get("/")

		assert.NoError(t, err)
		assert.Equal(t, http.StatusOK, resp.StatusCode())
		assert.False(t, onErrorCalled)
		assert.True(t, handlerCalled)
	})
}
