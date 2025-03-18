package httppusher

import (
	"bytes"
	"context"
	"encoding/json"
	"fmt"
	"net/http"
	"time"

	"github.com/go-resty/resty/v2"
	"github.com/pkg/errors"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/metrics/solomon"
	"a.yandex-team.ru/library/go/httputil/headers"
	"a.yandex-team.ru/library/go/yandex/solomon/reporters/pusher"
)

type Pusher struct {
	logger           log.Structured
	authProvider     func(ctx context.Context) (string, string, error)
	metricsChunkSize int
	useSpack         *solomon.CompressionType

	httpc *resty.Client
}

// NewPusher returns new Solomon pusher instance
func NewPusher(opts ...PusherOpt) (*Pusher, error) {
	return NewPusherWithResty(resty.New(), opts...)
}

// NewPusher returns new Solomon pusher instance using custom resty Client
func NewPusherWithResty(httpc *resty.Client, opts ...PusherOpt) (*Pusher, error) {
	p := &Pusher{
		httpc: httpc,
	}

	for _, opt := range opts {
		if err := opt(p); err != nil {
			return nil, err
		}
	}

	if p.httpc.QueryParam.Get("project") == "" {
		return nil, ErrEmptyProject
	}
	if p.httpc.QueryParam.Get("service") == "" {
		return nil, ErrEmptyService
	}
	if p.httpc.QueryParam.Get("cluster") == "" {
		return nil, ErrEmptyCluster
	}

	if p.httpc.HostURL == "" {
		p.httpc.SetBaseURL(pusher.HostProduction)
	}

	if p.useSpack == nil {
		p.httpc.SetHeader(headers.ContentTypeKey, headers.TypeApplicationJSON.String())
	} else {
		p.httpc.SetHeader(headers.ContentTypeKey, headers.TypeApplicationXSolomonSpack.String())
		switch *p.useSpack {
		case solomon.CompressionNone:
		case solomon.CompressionLz4:
			p.httpc.SetHeader(headers.ContentEncodingKey, headers.EncodingLZ4.String())
		default:
			return nil, fmt.Errorf("unsupported spack compression type: %v", *p.useSpack)
		}
	}

	return p, nil
}

func (p Pusher) encodeMetrics(ctx context.Context, metrics *solomon.Metrics) ([]byte, error) {
	if p.useSpack == nil {
		return json.Marshal(metrics)
	}
	var buf bytes.Buffer
	_, err := solomon.NewSpackEncoder(ctx, *p.useSpack, metrics).Encode(&buf)
	return buf.Bytes(), err

}

// Push sends gathered metrics to Solomon via HTTP in JSON format.
// This method expects that metrics encoding is metrics.FmtSolomon
func (p Pusher) Push(ctx context.Context, metrics *solomon.Metrics) error {
	_ = log.WriteAt(p.logger, log.DebugLevel, "pushing metrics", log.Any("metrics", metrics))

	b, err := p.encodeMetrics(ctx, metrics)
	if err != nil {
		return err
	}
	req := p.httpc.R()

	if p.authProvider != nil {
		header, value, err := p.authProvider(ctx)
		if err != nil {
			return err
		}
		req.SetHeader(header, value)
	}

	resp, err := req.
		SetBody(b).
		SetContext(ctx).
		Post("/api/v2/push")

	if err != nil {
		return err
	}

	if resp.StatusCode() == http.StatusGatewayTimeout {
		return ErrSendGatewayTimeout
	}

	if resp.IsError() {
		return fmt.Errorf("bad status code %d: %s", resp.StatusCode(), string(resp.Body()))
	}

	return nil
}

// Start starts new detached repeating push process at every given interval
func (p Pusher) Start(ctx context.Context, r *solomon.Registry, interval time.Duration) error {
	if r == nil {
		return errors.New("cannot start pusher: nil registry given")
	}

	tick := time.NewTicker(interval)
	defer tick.Stop()

	for {
		select {
		case <-ctx.Done():
			// end pusher process on behalf of context
			return nil
		case <-tick.C:
			metrics, err := r.Gather()
			if err != nil {
				_ = log.WriteAt(p.logger, log.ErrorLevel, "cannot gather metrics from registry", log.Error(err))
				continue
			}

			for _, metricsChunk := range metrics.SplitToChunks(p.metricsChunkSize) {
				if err := p.Push(ctx, &metricsChunk); err != nil {
					_ = log.WriteAt(p.logger, log.ErrorLevel, "cannot push metrics", log.Error(err))
				}
			}
		}
	}
}
