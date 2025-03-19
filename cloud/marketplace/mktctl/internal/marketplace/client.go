//
// ToDo:
//	* embed logging
//
package marketplace

import (
	"encoding/json"
	"fmt"
	"os"
	"time"

	"github.com/go-resty/resty/v2"
)

func NewClient(token, endpoint string) *Client {
	// add context for logger
	// check token
	// check endpoint
	// check context
	// add errors.go

	return &Client{api: newClient(token, endpoint)}
}

const (
	defaultTokenKey     = "X-YaCloud-SubjectToken"
	defaultRequestIDKey = "X-Request-ID"

	defaultUserAgent   = "YC.Marketplace.Internal.CLI"
	defaultLanguage    = "eng"
	defaultContentType = "application/json"
)

func newClient(token, endpoint string) *resty.Client {
	return resty.New().
		SetRetryCount(5).
		SetRetryWaitTime(1*time.Second).
		SetHostURL(endpoint).
		SetRetryMaxWaitTime(20*time.Second).
		SetRedirectPolicy(resty.NoRedirectPolicy()).
		SetHeader(defaultTokenKey, token).
		SetHeader("User-Agent", defaultUserAgent).
		SetHeader("Accept-Language", defaultLanguage).
		SetHeader("Content-type", defaultContentType).
		OnBeforeRequest(func(c *resty.Client, req *resty.Request) error {
			requestID, err := newRequestID()
			if err != nil {
				return err
			}
			req.SetHeader(defaultRequestIDKey, requestID)

			return nil
		}).
		OnAfterResponse(func(c *resty.Client, r *resty.Response) error {
			token := r.Header().Get(defaultTokenKey)
			if token != "" {
				r.Header().Set(defaultTokenKey, "***hidden***")
			}

			token = r.Request.Header.Get(defaultTokenKey)
			if token != "" {
				r.Request.Header.Set(defaultTokenKey, "***hidden***")
			}

			rb, _ := json.MarshalIndent(r.Request.Body, "", "\t")

			if r.IsError() {
				fmt.Printf("request-id: %v\n", r.Request.Header.Get(defaultRequestIDKey))
				fmt.Printf("received: %v\n", r.ReceivedAt().String())
				fmt.Printf("time: %v\n", r.Time().String())
				fmt.Printf("status: %v\n", r.Status())
				fmt.Printf("proto: %v\n", r.Proto())
				fmt.Printf("method: %v\n", r.Request.Method)
				fmt.Printf("url: %v\n", r.Request.URL)
				fmt.Printf("\nrequest-body: \n%+v\n", string(rb))
				fmt.Printf("\nresponse-body: \n%v\n", string(r.Body()))
				os.Exit(1)
			}

			return nil
		})
}

type Client struct {
	api *resty.Client
}

func (c *Client) NewRequest() *resty.Request {
	return c.api.R()
}
