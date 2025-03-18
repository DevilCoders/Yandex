package jwtauth

import (
	"fmt"
	"net/http"
	"testing"

	"github.com/golang-jwt/jwt/v4"
	jwtreq "github.com/golang-jwt/jwt/v4/request"
	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
	"a.yandex-team.ru/library/go/httputil/headers"
)

func TestWithKeyFunc(t *testing.T) {
	var mw middleware

	input := func(_ *jwt.Token) (interface{}, error) {
		return []byte("symmetric_secret"), nil
	}
	opt := WithKeyFunc(input)
	opt(&mw)

	assert.Equal(t, fmt.Sprintf("%p", input), fmt.Sprintf("%p", mw.keyFunc))
}

func TestWithSigningMethod(t *testing.T) {
	var mw middleware

	input := jwt.SigningMethodHS512
	opt := WithSigningMethod(input)
	opt(&mw)

	assert.Same(t, input, mw.signingMethod)
}

func TestWithClaims(t *testing.T) {
	var mw middleware

	input := func() jwt.Claims {
		type CustomClaims struct {
			jwt.Claims
			UserID int64 `json:"user_id,omitempty"`
		}
		return new(CustomClaims)
	}
	opt := WithClaims(input)
	opt(&mw)

	assert.Equal(t, fmt.Sprintf("%p", input), fmt.Sprintf("%p", mw.claimsFunc))
}

func TestWithLogger(t *testing.T) {
	var mw middleware

	input, err := zap.NewQloudLogger(log.DebugLevel)
	require.NoError(t, err)

	opt := WithLogger(input)
	opt(&mw)

	assert.Same(t, input, mw.l)
}

func TestWithExtractor(t *testing.T) {
	var mw middleware

	exs := jwtreq.MultiExtractor{
		jwtreq.ArgumentExtractor{"token"},
		jwtreq.HeaderExtractor{headers.AuthorizationKey},
	}

	opt := WithExtractor(exs...)
	opt(&mw)

	assert.Equal(t, exs, mw.extractors)
}

func TestOnError(t *testing.T) {
	var mw middleware

	input := func(w http.ResponseWriter, _ *http.Request, _ error) {
		w.WriteHeader(http.StatusInternalServerError)
	}
	opt := OnError(input)
	opt(&mw)

	assert.Equal(t, fmt.Sprintf("%p", input), fmt.Sprintf("%p", mw.onError))
}
