package server

import (
	"io/ioutil"
	"net"
	"net/http"
	"testing"

	"github.com/stretchr/testify/assert"
)

const (
	file     = "/"
	redirect = "/redirect"
)

func TestServer(t *testing.T) {
	a := assert.New(t)
	srv := NewServer()
	defer func() {
		a.NoError(srv.Close())
	}()
	l, err := net.Listen("tcp", "127.0.0.1:0")
	a.NoError(err)
	go func() {
		err := srv.Serve(l)
		if err != http.ErrServerClosed {
			panic(err)
		}
	}()
	resp, err := http.Get(FileURL(l))
	a.NoError(err)
	a.Equal(http.StatusOK, resp.StatusCode)
	content, err := ioutil.ReadAll(resp.Body)
	a.NoError(err)
	a.Equal(13267968, len(content))
	_ = resp.Body.Close()

	resp, err = http.Get(RedirectURL(l))
	a.NoError(err)
	a.NotEqual(RedirectURL(l), resp.Request.URL.String())
	_ = resp.Body.Close()

	resp, err = http.DefaultClient.Head(FileURL(l))
	a.NoError(err)
	a.Equal(int64(13267968), resp.ContentLength)
	content, err = ioutil.ReadAll(resp.Body)
	a.NoError(err)
	a.Equal(0, len(content))
}
