// rehttp is a package with wrappers for making common http requests with retries
package rehttp

import (
	"encoding/json"
	"fmt"
	"github.com/cenkalti/backoff/v4"
	"io/ioutil"
	"net/url"
	"time"
)

func NewExponentialBackOff() *backoff.ExponentialBackOff {
	b := &backoff.ExponentialBackOff{
		InitialInterval:     backoff.DefaultInitialInterval,
		RandomizationFactor: backoff.DefaultRandomizationFactor,
		Multiplier:          backoff.DefaultMultiplier,
		MaxInterval:         10 * time.Second,
		MaxElapsedTime:      20 * time.Second,
		Stop:                backoff.Stop,
		Clock:               backoff.SystemClock,
	}
	b.Reset()
	return b
}

// Make a GET request with an json request body and return an response body as a string
func (rh Client) GetText(u *url.URL, v interface{}) (string, error) {
	var bodyText []byte

	op := func() error {
		code, body, err := rh.RequestRaw("GET", u, v, "")

		if err != nil {
			return err
		}

		bodyText, err = ioutil.ReadAll(body)
		if err != nil {
			return err
		}

		for _, ex := range rh.ExpectedCode {
			if ex == code {
				return nil
			}
		}

		return fmt.Errorf("got status %d from %s. %s", code, rh.ServiceName, bodyText)
	}

	b := rh.BackOff
	if b == nil {
		b = NewExponentialBackOff()
	}
	b.Reset()

	err := backoff.Retry(op, b)

	return string(bodyText), err
}

// Make a GET request with an json request and response bodies
func (rh Client) GetJSON(u *url.URL, v interface{}, w interface{}) error {
	op := func() error {
		code, body, err := rh.RequestRaw("GET", u, v, "")

		if err != nil {
			return err
		}

		for _, ex := range rh.ExpectedCode {
			if ex == code {
				err = json.NewDecoder(body).Decode(&w)
				return err
			}
		}

		bodyText, err := ioutil.ReadAll(body)
		if err != nil {
			return err
		}

		return fmt.Errorf("got status %d from %s. %s", code, rh.ServiceName, bodyText)
	}

	b := rh.BackOff
	if b == nil {
		b = NewExponentialBackOff()
	}
	b.Reset()

	err := backoff.Retry(op, b)

	return err
}
