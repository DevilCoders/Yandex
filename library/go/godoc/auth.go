package main

import (
	"log"
	"net"
	"net/http"

	"a.yandex-team.ru/library/go/yandex/blackbox"
)

var (
	blackboxClient blackbox.Client
)

func checkAuth(w http.ResponseWriter, r *http.Request) bool {
	sessionID, err := r.Cookie("Session_id")
	if err != nil {
		log.Printf("no cookie: %v", err)
		w.WriteHeader(http.StatusUnauthorized)
		return false
	}

	host, _, _ := net.SplitHostPort(r.RemoteAddr)

	_, err = blackboxClient.SessionID(r.Context(), blackbox.SessionIDRequest{
		SessionID: sessionID.Value,
		UserIP:    host,
		Host:      "godoc.yandex-team.ru",
	})

	if err != nil {
		log.Printf("blackbox error: %v", err)
		w.WriteHeader(http.StatusUnauthorized)
		return false
	}

	return true
}
