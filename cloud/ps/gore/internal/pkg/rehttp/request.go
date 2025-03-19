// rehttp is a package with wrappers for making common http requests with retries
package rehttp

import (
	"bytes"
	"encoding/json"
	"fmt"
	"io"
	"io/ioutil"
	"net/http"
	"net/url"
	"time"

	"a.yandex-team.ru/cloud/ps/gore/internal/pkg/auth"
)

// Make a raw request with an json request and raw stream with response body
func (rh Client) RequestRaw(t string, u *url.URL, v interface{}, contentType string) (int, io.ReadCloser, error) {
	var body []byte
	var err error

	if v != nil {
		body, err = json.Marshal(v)
		if err != nil {
			return 0, nil, err
		}
	}

	request, err := http.NewRequest(t, u.String(), bytes.NewBuffer(body))
	if err != nil {
		return 0, nil, err
	}

	if rh.Token != "" {
		switch rh.AuthType {
		case auth.BasicAuth:
			request.SetBasicAuth("x-oauth-token", rh.Token)
		case auth.OAuth:
			request.Header.Set("Authorization", fmt.Sprintf("OAuth %s", rh.Token))
		}
	}

	if contentType != "" {
		request.Header.Set("Content-Type", contentType)
	}

	timeout := rh.Timeout

	// set timeout to 10 second if not specified
	if timeout == time.Duration(0) {
		timeout = 10 * time.Second
	}

	client := &http.Client{
		Timeout: timeout,
	}
	resp, err := client.Do(request)
	if err != nil {
		return 0, nil, err
	}

	return resp.StatusCode, resp.Body, nil
}

// Make a request with an json request body and return an response body as a string
func (rh Client) RequestText(t string, u *url.URL, v interface{}) (string, error) {
	code, body, err := rh.RequestRaw(t, u, v, "")
	if err != nil {
		return "", err
	}

	bodyText, err := ioutil.ReadAll(body)
	if err != nil {
		return "", err
	}

	for _, ex := range rh.ExpectedCode {
		if ex == code {
			return string(bodyText), nil
		}
	}

	return "", fmt.Errorf("got status %d from %s. %s", code, rh.ServiceName, bodyText)
}

// Make a request with an json request and response bodies
func (rh Client) RequestJSON(t string, u *url.URL, v interface{}, w interface{}) error {
	code, body, err := rh.RequestRaw(t, u, v, "application/json")
	if err != nil {
		return err
	}

	for _, ex := range rh.ExpectedCode {
		if ex == code {
			return json.NewDecoder(body).Decode(&w)
		}
	}

	bodyText, err := ioutil.ReadAll(body)
	if err != nil {
		return err
	}

	return fmt.Errorf("got status %d from %s. %s", code, rh.ServiceName, bodyText)
}
