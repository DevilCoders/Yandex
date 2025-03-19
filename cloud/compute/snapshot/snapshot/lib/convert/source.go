package convert

import (
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/directreader"
	neturl "net/url"

	"go.uber.org/zap"
	"golang.org/x/net/context"

	log "a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/common"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/misc"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/nbd"
)

const (
	schemeHTTP  = "http"
	schemeHTTPS = "https"
)

// ImageSource represents conversion image source.
type ImageSource interface {
	// Get must return a valid image or an error.
	// misc.ErrUnknownSource means this request is for different source.
	Get(ctx context.Context, r *common.ConvertRequest, blockMap directreader.BlocksMap) (nbd.Image, error)
}

// ImageSourceRegistry is a chain-of-responsibility-based list of sources.
type ImageSourceRegistry []ImageSource

// Get returns an Image if any of registered sources knows was able to process the request.
func (reg ImageSourceRegistry) Get(ctx context.Context, r *common.ConvertRequest, blocksMap directreader.BlocksMap) (nbd.Image, error) {
	for _, src := range reg {
		img, err := src.Get(ctx, r, blocksMap)
		switch err {
		case nil:
			return img, err
		case misc.ErrUnknownSource:
			continue
		default:
			return img, err
		}
	}
	return nil, misc.ErrUnknownSource
}

// HTTPSource is an HTTP-based image source.
type HTTPSource struct {
	EnableRedirects bool
	ProxySock       string
}

// Get tries to process the request.
func (src HTTPSource) Get(ctx context.Context, r *common.ConvertRequest, blockMap directreader.BlocksMap) (nbd.Image, error) {
	if r.URL == "" {
		return nil, misc.ErrUnknownSource
	}

	log.G(ctx).Info("Creating image from http source", zap.Any("request", r))

	// Fix url
	u, err := neturl.Parse(r.URL)
	if err != nil {
		log.G(ctx).Warn("neturl.Parse failed", zap.Error(err))
		return nil, misc.ErrUnknownSource
	}

	switch u.Scheme {
	case "":
		u.Scheme = schemeHTTP
		fallthrough
	case schemeHTTP, schemeHTTPS:
		return newHTTPImage(ctx, u.String(), r.Format, src.EnableRedirects, src.ProxySock, blockMap)
	default:
		return nil, misc.ErrUnknownSource
	}
}
