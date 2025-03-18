package main

import (
	"encoding/json"
	"errors"
	"net/http"

	"github.com/golang-jwt/jwt/v4"
)

func SetJSONContentType(writer http.ResponseWriter) {
	writer.Header().Set("Content-Type", "application/json")
}

func WriteErrorResponse(writer http.ResponseWriter, err error) {
	writer.WriteHeader(http.StatusBadRequest)

	_ = json.NewEncoder(writer).Encode(map[string]string{
		"error": err.Error(),
	})
}

func JoinURL(prefix, path string) string {
	if len(prefix) > 0 && prefix[len(prefix)-1] == '/' {
		return prefix + path
	} else {
		return prefix + "/" + path
	}
}

func WriteTokenResponse(
	writer http.ResponseWriter,
	kid string,
	method jwt.SigningMethod,
	key interface{},
	ourClaims jwt.Claims,
) error {
	token := jwt.NewWithClaims(method, ourClaims)
	token.Header["kid"] = kid
	tokenStr, err := token.SignedString(key)
	if err != nil {
		WriteErrorResponse(writer, errors.New("token signing failed"))
		return err
	}

	return json.NewEncoder(writer).Encode(map[string]string{
		"token": tokenStr,
	})
}

func GetUserIP(request *http.Request) string {
	ip := request.Header.Get("X-Forwarded-For-Y")
	if ip == "" {
		ip = request.RemoteAddr
	}

	return ip
}
