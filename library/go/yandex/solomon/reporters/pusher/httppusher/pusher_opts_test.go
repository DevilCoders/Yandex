package httppusher

import (
	"testing"

	"github.com/go-resty/resty/v2"
	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
	"a.yandex-team.ru/library/go/core/metrics/solomon"
	"a.yandex-team.ru/library/go/httputil/headers"
)

func TestSetCluster(t *testing.T) {
	p := &Pusher{httpc: resty.New()}
	expectValue := "ololo"

	opt := SetCluster(expectValue)

	err := opt(p)
	assert.NoError(t, err)

	assert.Equal(t, expectValue, p.httpc.QueryParam.Get("cluster"))
}

func TestSetProject(t *testing.T) {
	p := &Pusher{httpc: resty.New()}
	expectValue := "ololo"

	opt := SetProject(expectValue)

	err := opt(p)
	assert.NoError(t, err)

	assert.Equal(t, expectValue, p.httpc.QueryParam.Get("project"))
}

func TestSetService(t *testing.T) {
	p := &Pusher{httpc: resty.New()}
	expectValue := "ololo"

	opt := SetService(expectValue)

	err := opt(p)
	assert.NoError(t, err)

	assert.Equal(t, expectValue, p.httpc.QueryParam.Get("service"))
}

func TestWithHTTPHost(t *testing.T) {
	p := &Pusher{httpc: resty.New()}
	expectValue := "https://solomon.yandex.net"

	opt := WithHTTPHost(expectValue)

	err := opt(p)
	assert.NoError(t, err)

	assert.Equal(t, expectValue, p.httpc.HostURL)
}

func TestWithLogger(t *testing.T) {
	p := &Pusher{httpc: resty.New()}
	expectValue, err := zap.NewQloudLogger(log.DebugLevel)
	assert.NoError(t, err)

	opt := WithLogger(expectValue)

	err = opt(p)
	assert.NoError(t, err)

	assert.Equal(t, expectValue, p.logger)
}

func TestWithOAuthToken(t *testing.T) {
	p := &Pusher{httpc: resty.New()}
	expectValue := "token-shmoken"

	opt := WithOAuthToken(expectValue)

	err := opt(p)
	assert.NoError(t, err)

	assert.Equal(t, "OAuth "+expectValue, p.httpc.Header.Get("Authorization"))
}

func TestWithMetricsChunkSize(t *testing.T) {
	p := &Pusher{httpc: resty.New()}
	opt := WithMetricsChunkSize(100)

	err := opt(p)
	assert.NoError(t, err)
	assert.Equal(t, 100, p.metricsChunkSize)
}

func TestWithSpack(t *testing.T) {
	compNone := solomon.CompressionNone
	compLz4 := solomon.CompressionLz4

	cases := []struct {
		name                string
		compType            *solomon.CompressionType
		expectedContentType string
		expectedEncoding    string
	}{
		{"do not use spack", nil, headers.TypeApplicationJSON.String(), ""},
		{"use spack without compresstion", &compNone, headers.TypeApplicationXSolomonSpack.String(), ""},
		{"use spack with compression", &compLz4, headers.TypeApplicationXSolomonSpack.String(), headers.EncodingLZ4.String()},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			opts := []PusherOpt{SetProject("p"), SetService("s"), SetCluster("c")}
			if tc.compType != nil {
				opts = append(opts, WithSpack(*tc.compType))
			}
			p, err := NewPusher(opts...)
			assert.NoError(t, err)
			assert.Equal(t, tc.compType, p.useSpack)

			cType := p.httpc.Header.Get(headers.ContentTypeKey)
			assert.Equal(t, tc.expectedContentType, cType, "unexpected content type")

			cEnc := p.httpc.Header.Get(headers.ContentEncodingKey)
			assert.Equal(t, tc.expectedEncoding, cEnc, "unexpected content encoding")
		})
	}
}
