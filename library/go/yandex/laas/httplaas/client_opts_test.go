package httplaas

import (
	"context"
	"errors"
	stdlog "log"
	"net/http"
	"net/http/httptest"
	"testing"

	"github.com/go-resty/resty/v2"
	"github.com/gofrs/uuid"
	"github.com/google/go-cmp/cmp"
	"github.com/google/go-cmp/cmp/cmpopts"
	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
	"a.yandex-team.ru/library/go/test/output"
	"a.yandex-team.ru/library/go/yandex/laas"
)

func TestAsService(t *testing.T) {
	testCases := []struct {
		name         string
		value        string
		expectClient *Client
		expectError  error
	}{
		{
			"empty_name",
			"",
			&Client{httpc: resty.New()},
			errors.New("laas: service name cannot be empty"),
		},
		{
			"nonempty_name",

			"ololo",
			&Client{
				httpc: resty.New().
					SetQueryParam("service", "ololo"),
			},
			nil,
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			c := &Client{httpc: resty.New()}
			opt := AsService(tc.value)
			err := opt(c)

			if tc.expectError == nil {
				assert.NoError(t, err)
			} else {
				assert.EqualError(t, err, tc.expectError.Error())
			}

			cmpOpts := cmp.Options{
				cmp.AllowUnexported(Client{}),
				cmpopts.IgnoreUnexported(stdlog.Logger{}, resty.Client{}),
				cmpopts.IgnoreFields(resty.Client{}, "JSONMarshal", "JSONUnmarshal", "XMLMarshal", "XMLUnmarshal"),
			}
			assert.True(t, cmp.Equal(tc.expectClient, c, cmpOpts...), cmp.Diff(tc.expectClient, c, cmpOpts...))
		})
	}
}

func TestWithAppUUID(t *testing.T) {
	testCases := []struct {
		name         string
		value        uuid.UUID
		expectClient *Client
	}{
		{
			"empty_uuid",
			uuid.Nil,
			&Client{
				httpc: resty.New().
					SetQueryParam("uuid", "00000000-0000-0000-0000-000000000000"),
			},
		},
		{
			"nonempty_uuid",
			uuid.Must(uuid.FromString("8b217717-f4aa-4417-b872-59780d8bb577")),
			&Client{
				httpc: resty.New().
					SetQueryParam("uuid", "8b217717-f4aa-4417-b872-59780d8bb577"),
			},
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			c := &Client{httpc: resty.New()}
			opt := WithAppUUID(tc.value)
			_ = opt(c)

			cmpOpts := cmp.Options{
				cmp.AllowUnexported(Client{}),
				cmpopts.IgnoreUnexported(stdlog.Logger{}, resty.Client{}),
				cmpopts.IgnoreFields(resty.Client{}, "JSONMarshal", "JSONUnmarshal", "XMLMarshal", "XMLUnmarshal"),
			}
			assert.True(t, cmp.Equal(tc.expectClient, c, cmpOpts...), cmp.Diff(tc.expectClient, c, cmpOpts...))
		})
	}
}

func TestWithHTTPHost(t *testing.T) {
	testCases := []struct {
		name        string
		client      *Client
		value       string
		expectValue string
	}{
		{
			"empty_url",
			&Client{httpc: resty.New()},
			"",
			DefaultHTTPHost,
		},
		{
			"nonempty_url",
			&Client{httpc: resty.New()},
			"http://laas-mock.yandex-team.ru",
			"http://laas-mock.yandex-team.ru",
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			opt := WithHTTPHost(tc.value)
			_ = opt(tc.client)
			assert.Equal(t, tc.value, tc.client.httpc.HostURL)
		})
	}
}

func TestWithLogger(t *testing.T) {
	err := output.Replace(output.Stdout)
	require.NoError(t, err)
	defer output.Reset(output.Stdout)

	srv := httptest.NewServer(http.HandlerFunc(func(_ http.ResponseWriter, _ *http.Request) {
	}))
	defer srv.Close()

	httpc := resty.New().
		SetBaseURL(srv.URL).
		SetDebug(true)

	c := &Client{httpc: httpc}

	l, err := zap.NewQloudLogger(log.DebugLevel)
	assert.NoError(t, err)

	opt := WithLogger(l)
	err = opt(c)
	assert.NoError(t, err)

	ctx := context.Background()
	_, _ = c.DetectRegion(ctx, laas.Params{})

	result := output.Catch(output.Stdout)
	assert.NotEmpty(t, result)
}

func TestWithDebug(t *testing.T) {
	testCases := []struct {
		name         string
		value        bool
		expectClient *Client
	}{
		{
			"with_debug",
			true,
			&Client{
				httpc: resty.New().
					SetDebug(true),
			},
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			c := &Client{httpc: resty.New()}
			opt := WithDebug(tc.value)
			_ = opt(c)

			cmpOpts := cmp.Options{
				cmp.AllowUnexported(Client{}),
				cmpopts.IgnoreUnexported(stdlog.Logger{}, resty.Client{}),
				cmpopts.IgnoreFields(resty.Client{}, "JSONMarshal", "JSONUnmarshal", "XMLMarshal", "XMLUnmarshal"),
			}
			assert.True(t, cmp.Equal(tc.expectClient, c, cmpOpts...), cmp.Diff(tc.expectClient, c, cmpOpts...))
		})
	}
}
