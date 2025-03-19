package proxy

import (
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/misc"
	"crypto/rand"
	"errors"
	"fmt"
	"golang.org/x/xerrors"
	"net"
	"net/http"
	"net/http/httputil"
	"net/url"
	"strings"
	"sync"
)

// redirecting proxy/<id> -> target/<url> if <id> : <url> in idToURL mapping
// AddURL / RemoveID to work with mapping
type Proxy struct {
	*httputil.ReverseProxy
	*http.Server

	clientHost string
	idLength   int

	mutex   sync.RWMutex
	idToURL map[string]*url.URL
}

func NewProxy(clientHost string, idLength int) *Proxy {
	proxy := &Proxy{idToURL: map[string]*url.URL{}, ReverseProxy: &httputil.ReverseProxy{}, clientHost: clientHost, idLength: idLength}
	proxy.Director = proxy.direct
	proxy.ModifyResponse = proxy.modifyResponse
	proxy.Server = &http.Server{
		Handler: proxy,
	}
	return proxy
}

func (pr *Proxy) Serve(listener net.Listener) error {
	return pr.Server.Serve(listener)
}

func (pr *Proxy) Stop() error {
	return pr.Server.Close()
}

func (pr *Proxy) AddURL(targetURL string) (string, string, error) {
	client := &http.Client{CheckRedirect: func(req *http.Request, via []*http.Request) error { return misc.ErrDenyRedirect }}
	if resp, err := client.Get(targetURL); err == nil {
		_ = resp.Body.Close()
		switch resp.StatusCode {
		case http.StatusOK:
			// test GET request success, continue proxying
		case http.StatusForbidden:
			return "", "", misc.ErrSourceURLAccessDenied
		case http.StatusNotFound:
			return "", "", misc.ErrSourceURLNotFound
		default:
			return "", "", fmt.Errorf("test GET to %s failed with %d code", targetURL, resp.StatusCode)

		}
	} else {
		if errors.Is(err, misc.ErrDenyRedirect) {
			return "", "", misc.ErrDenyRedirect
		}
		return "", "", xerrors.Errorf("test GET to %s failed: %w", targetURL, err)
	}

	u, err := url.Parse(targetURL)
	if err != nil {
		return "", "", xerrors.Errorf("on parsing url on proxy.AddURL: %w", err)
	}

	rawID := make([]byte, (pr.idLength+1)/2)
	_, err = rand.Read(rawID)
	if err != nil {
		return "", "", xerrors.Errorf("on generating random sequence in proxy.AddURL: %w", err)
	}

	id := fmt.Sprintf("%x", rawID)[:pr.idLength]
	pr.mutex.Lock()
	defer pr.mutex.Unlock()
	pr.idToURL[id] = u

	return fmt.Sprintf("http://%s/%s", pr.clientHost, id), id, nil
}

func (pr *Proxy) RemoveID(id string) error {
	pr.mutex.Lock()
	defer pr.mutex.Unlock()
	if _, ok := pr.idToURL[id]; ok {
		delete(pr.idToURL, id)
		return nil
	} else {
		return xerrors.New("delete unexpected id from proxy")
	}
}

// Redirecting GET/HEAD request with existing ID is redirected to target with corresponding path. Else - stopping request
func (pr *Proxy) direct(r *http.Request) {
	pr.mutex.RLock()
	defer pr.mutex.RUnlock()
	if u, ok := pr.idToURL[strings.Trim(r.URL.Path, "/")]; ok && (r.Method == http.MethodGet || r.Method == http.MethodHead) {
		r.URL = u
		r.Host = u.Host
		r.RequestURI = u.Path
	} else {
		r.URL = nil
		r.Host = ""
		r.RequestURI = ""
	}
}

func (pr *Proxy) modifyResponse(rp *http.Response) error {
	// response from redirect
	if rp.StatusCode >= 300 && rp.StatusCode < 400 && rp.StatusCode != http.StatusNotModified {
		return misc.ErrDenyRedirect
	} else {
		return nil
	}
}
