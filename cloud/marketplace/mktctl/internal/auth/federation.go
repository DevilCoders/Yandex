package auth

import (
	"errors"
	"fmt"
	"net"
	"net/http"
	"net/url"
	"os/exec"
	"runtime"
	"syscall"
	"time"
)

type Federation struct {
	Token     string
	ExpiresAt string
}

const defaultTimeout = 10 * time.Second

func GetFederationToken(federationID, federationEndpoint string) (*Federation, error) {

	captureToken := make(chan Federation, 1)

	//spin server, which will capture redirect
	l, _ := net.Listen("tcp", "127.0.0.1:0")
	redirectionEndpoint := l.Addr().String()
	defer func() { _ = l.Close() }()
	go func() {
		_ = http.Serve(l, http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			http.Redirect(w, r, fmt.Sprintf("https://%v", federationEndpoint), http.StatusSeeOther)
			v := r.URL.Query()
			captureToken <- Federation{
				Token:     v.Get("token"),
				ExpiresAt: v.Get("expiresAt"),
			}
		}))
	}()

	url, err := newFederationRedirectURL(federationEndpoint, federationID, redirectionEndpoint)
	if err != nil {
		return nil, err
	}

	//open browser
	err = openBrowser(url)
	if err != nil {
		return nil, err
	}

	// stop server
	select {
	case t := <-captureToken:
		return &t, nil
	case <-time.After(defaultTimeout):
		return nil, errors.New("timeout")
	}
}

func newFederationRedirectURL(federationEndpoint, federationID, redirectionEndpoint string) (string, error) {

	rawurl := fmt.Sprintf("https://%v/federations/%v", federationEndpoint, federationID)
	baseURL, err := url.Parse(rawurl)
	if err != nil {
		return "", err
	}

	params := url.Values{}
	params.Add("redirectUrl", fmt.Sprintf("http://%v", redirectionEndpoint))
	baseURL.RawQuery = params.Encode()

	return baseURL.String(), nil
}

func openBrowser(url string) error {

	var c *exec.Cmd

	switch runtime.GOOS {
	case "darwin":
		c = exec.Command("open", url)
	case "linux":
		c = exec.Command("xdg-open", url)
		c.SysProcAttr = &syscall.SysProcAttr{
			Setpgid: true,
		}
	default:
		return fmt.Errorf("%w: %v", ErrUnsupportedOS, runtime.GOOS)
	}

	if err := c.Run(); err != nil {
		return err
	}

	return nil
}
