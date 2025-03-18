package jwtauth_test

import (
	"fmt"
	"net/http"

	"github.com/go-chi/chi/v5"
	"github.com/golang-jwt/jwt/v4"

	"a.yandex-team.ru/library/go/httputil/middleware/jwtauth"
)

func ExampleVerifyToken() {
	// Create HTTP router.
	r := chi.NewRouter()

	// Plug middleware in for any handler
	// Note: signing method and key func are required
	r.Use(jwtauth.VerifyToken(
		jwtauth.WithSigningMethod(jwt.SigningMethodHS512),
		jwtauth.WithKeyFunc(func(_ *jwt.Token) (interface{}, error) {
			return []byte("my_symmetric_secret"), nil
		}),
	))

	// add handlers and catch parsed and verified JWT
	r.Handle("/user/info", http.HandlerFunc(func(_ http.ResponseWriter, r *http.Request) {
		token := r.Context().Value(jwtauth.TokenCtxKey)
		fmt.Printf("user token: %+v", token)
	}))
}
