package proxy

import (
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/misc"
	"errors"
	"fmt"
	"github.com/stretchr/testify/assert"
	"io/ioutil"
	"net"
	"net/http"
	"net/http/httptest"
	"net/url"
	"strings"
	"testing"
)

func localProxy(assertions *assert.Assertions, addr string) *Proxy {
	proxy := NewProxy(addr, 20)
	l, err := net.Listen("tcp", addr)
	if err != nil {
		assertions.NoError(err)
	}

	go func() {
		err = proxy.Serve(l)
		if err != http.ErrServerClosed {
			panic(err)
		}
	}()
	return proxy
}

func TestProxy(t *testing.T) {
	a := assert.New(t)
	proxy := localProxy(a, "localhost:2000")
	defer proxy.Stop()
	ts := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) { _, _ = fmt.Fprintln(w, "all ok") }))
	proxyedURL, id, err := proxy.AddURL(ts.URL)
	defer func() {
		a.NoError(proxy.RemoveID(id))
	}()
	a.NoError(err)
	newURL, err := url.Parse(proxyedURL)
	a.NoError(err)
	a.Equal("localhost:2000", newURL.Host, "Request should be to the proxy")
	a.Equal(id, strings.Trim(newURL.Path, "/"), "proxy hides real path via id")
	resp, err := http.Get(proxyedURL)
	a.NoError(err)
	a.Equal(200, resp.StatusCode)
	data, err := ioutil.ReadAll(resp.Body)
	a.NoError(err)
	_ = resp.Body.Close()
	a.Equal("all ok\n", string(data))

	resp, err = http.DefaultClient.Head(proxyedURL)
	a.NoError(err)
	a.Equal(int64(len(data)), resp.ContentLength)
}

func TestRedirect(t *testing.T) {
	a := assert.New(t)
	proxy := localProxy(a, "0.0.0.0:0")
	defer proxy.Stop()
	ts := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) { _, _ = fmt.Fprintln(w, "all ok") }))
	director := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		http.Redirect(w, r, ts.URL, http.StatusSeeOther)
	}))
	_, _, err := proxy.AddURL(director.URL)
	a.True(errors.Is(err, misc.ErrDenyRedirect))
}
