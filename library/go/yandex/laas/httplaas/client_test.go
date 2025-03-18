package httplaas

import (
	"errors"
	"log"
	"testing"

	"github.com/go-resty/resty/v2"
	"github.com/google/go-cmp/cmp"
	"github.com/google/go-cmp/cmp/cmpopts"
	"github.com/stretchr/testify/assert"
)

func TestNewClientWithResty(t *testing.T) {
	testCases := []struct {
		name         string
		restyClient  *resty.Client
		opts         []ClientOpt
		expectClient *Client
		expectError  error
	}{
		{
			"default_resty_no_opts",
			resty.New(),
			nil,
			func() *Client {
				httpc := resty.New().
					SetBaseURL(DefaultHTTPHost).
					SetHeader("Host", DefaultHTTPHost).
					SetDoNotParseResponse(true)
				return &Client{httpc: httpc}
			}(),
			nil,
		},
		{
			"default_resty_with_opts_error",
			resty.New(),
			[]ClientOpt{
				AsService("ololo"),
				func(c *Client) error {
					return errors.New("because I'm bad")
				},
			},
			nil,
			errors.New("because I'm bad"),
		},
		{
			"default_resty_with_opts",
			resty.New(),
			[]ClientOpt{
				AsService("ololo"),
				WithHTTPHost("http://laas-mock.yandex.ru"),
			},
			func() *Client {
				httpc := resty.New().
					SetBaseURL("http://laas-mock.yandex.ru").
					SetHeader("Host", "http://laas-mock.yandex.ru").
					SetDoNotParseResponse(true).
					SetQueryParam("service", "ololo")
				return &Client{httpc: httpc}
			}(),
			nil,
		},
		{
			"custom_resty_with_opts",
			resty.New().
				SetHeader("X-Real-IP", "192.168.0.42"),
			[]ClientOpt{
				AsService("ololo"),
				WithHTTPHost("http://laas-mock.yandex.ru"),
			},
			func() *Client {
				httpc := resty.New().
					SetBaseURL("http://laas-mock.yandex.ru").
					SetHeader("Host", "http://laas-mock.yandex.ru").
					SetDoNotParseResponse(true).
					SetHeader("X-Real-IP", "192.168.0.42").
					SetQueryParam("service", "ololo")
				return &Client{httpc: httpc}
			}(),
			nil,
		},
	}

	// setup comparer
	cmpOpts := cmp.Options{
		cmp.AllowUnexported(Client{}),
		cmpopts.IgnoreUnexported(log.Logger{}, resty.Client{}),
		cmpopts.IgnoreFields(resty.Client{}, "JSONMarshal", "JSONUnmarshal", "XMLMarshal", "XMLUnmarshal"),
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			c, err := NewClientWithResty(tc.restyClient, tc.opts...)

			if tc.expectError == nil {
				assert.NoError(t, err)
			} else {
				assert.EqualError(t, err, tc.expectError.Error())
			}

			assert.True(t, cmp.Equal(tc.expectClient, c, cmpOpts...), cmp.Diff(tc.expectClient, c, cmpOpts...))
		})
	}
}

func TestNewClient(t *testing.T) {
	testCases := []struct {
		name         string
		opts         []ClientOpt
		expectClient *Client
		expectError  error
	}{
		{
			"no_opts",
			nil,
			func() *Client {
				httpc := resty.New().
					SetBaseURL(DefaultHTTPHost).
					SetHeader("Host", DefaultHTTPHost).
					SetDoNotParseResponse(true)
				return &Client{httpc: httpc}
			}(),
			nil,
		},
		{
			"with_opts_error",
			[]ClientOpt{
				AsService("ololo"),
				func(c *Client) error {
					return errors.New("because I'm bad")
				},
			},
			nil,
			errors.New("because I'm bad"),
		},
		{
			"with_opts",
			[]ClientOpt{
				AsService("ololo"),
				WithHTTPHost("http://laas-mock.yandex.ru"),
			},
			func() *Client {
				httpc := resty.New().
					SetBaseURL("http://laas-mock.yandex.ru").
					SetHeader("Host", "http://laas-mock.yandex.ru").
					SetQueryParam("service", "ololo").
					SetDoNotParseResponse(true)
				return &Client{httpc: httpc}
			}(),
			nil,
		},
	}

	// setup comparer
	cmpOpts := cmp.Options{
		cmp.AllowUnexported(Client{}),
		cmpopts.IgnoreUnexported(log.Logger{}, resty.Client{}),
		cmpopts.IgnoreFields(resty.Client{}, "JSONMarshal", "JSONUnmarshal", "XMLMarshal", "XMLUnmarshal"),
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			c, err := NewClient(tc.opts...)

			if tc.expectError == nil {
				assert.NoError(t, err)
			} else {
				assert.EqualError(t, err, tc.expectError.Error())
			}

			assert.True(t, cmp.Equal(tc.expectClient, c, cmpOpts...), cmp.Diff(tc.expectClient, c, cmpOpts...))
		})
	}
}
