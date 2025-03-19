package raven

import (
	"context"
	"crypto/tls"
	"fmt"
	"net/http"

	"github.com/getsentry/raven-go"

	"a.yandex-team.ru/cloud/mdb/internal/secret"
	"a.yandex-team.ru/cloud/mdb/internal/sentry"
	"a.yandex-team.ru/cloud/mdb/internal/sentry/tags"
	"a.yandex-team.ru/library/go/certifi"
	"a.yandex-team.ru/library/go/core/buildinfo"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Config struct {
	DSN         secret.String
	Environment string
}

func Init(cfg Config) error {
	client, err := raven.New(cfg.DSN.Unmask())
	if err != nil {
		return xerrors.Errorf("client construction: %w", err)
	}

	client.SetEnvironment(cfg.Environment)
	client.SetRelease(buildinfo.Info.SVNRevision)
	client.SetIncludePaths([]string{"a.yandex-team.ru/cloud/mdb"})

	// transport
	certs, err := certifi.NewCertPool()
	if err != nil {
		return xerrors.Errorf("certifi cert pool: %w", err)
	}

	client.Transport = &raven.HTTPTransport{Client: &http.Client{
		Transport: &http.Transport{
			Proxy:           http.ProxyFromEnvironment,
			TLSClientConfig: &tls.Config{RootCAs: certs},
		},
	}}

	sentry.SetGlobalClient(&Client{client})
	return nil
}

type Client struct {
	sentry *raven.Client
}

var _ sentry.Client = &Client{}

func (c *Client) CaptureError(ctx context.Context, err error, callerTags map[string]string) {
	knownTags := tags.WellKnownTags(ctx)
	for k, v := range callerTags {
		knownTags["caller."+k] = v
	}

	c.sentry.CaptureError(
		err,
		knownTags,
		newStackTrace(err, 2, c.sentry.IncludePaths()),
		// Add full error message with backtraces and stuff
		&raven.Message{
			Message: fmt.Sprintf("%+v", err),
		},
	)
}

func (c *Client) CaptureErrorAndWait(ctx context.Context, err error, callerTags map[string]string) {
	knownTags := tags.WellKnownTags(ctx)
	for k, v := range callerTags {
		knownTags["caller."+k] = v
	}
	c.sentry.CaptureErrorAndWait(err, knownTags, newStackTrace(err, 2, c.sentry.IncludePaths()))
}
