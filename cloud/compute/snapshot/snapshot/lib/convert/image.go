package convert

import (
	stdctx "context"
	"fmt"
	"net"
	"net/http"
	"time"

	"github.com/opentracing/opentracing-go"
	"go.uber.org/zap"
	"golang.org/x/net/context"

	log "a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/directreader"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/misc"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/nbd"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/storage"
)

const (
	qemuDefaultReadahead = 256 * 1024
	httpDefaultTimeout   = 10 * time.Second
	qemuDefaultTimeout   = 30 * time.Second
	settingsTimeout      = 5 * time.Minute
)

var (
	nonZeroChunk           = []byte("\"data\": true")
	_            nbd.Image = &httpImage{}
)

func headImage(ctx context.Context, url string, enableRedirects bool, proxySock string) (*http.Response, error) {
	span := opentracing.SpanFromContext(ctx)
	if span != nil {
		span = opentracing.StartSpan("headImage", opentracing.ChildOf(span.Context()))
		defer span.Finish()
		ctx = opentracing.ContextWithSpan(ctx, span)
	}

	connSocket := func(ctx stdctx.Context) (net.Conn, error) {
		conn := (&net.Dialer{
			Timeout:   30 * time.Second,
			KeepAlive: 30 * time.Second,
			DualStack: true,
		}).DialContext
		return conn(ctx, "unix", proxySock)
	}
	transport := http.Transport{
		DialContext: func(ctx stdctx.Context, network, addr string) (conn net.Conn, err error) {
			return connSocket(ctx)
		},
		Dial: func(network, addr string) (conn net.Conn, err error) {
			return connSocket(context.Background())
		},
	}

	c := http.Client{
		Transport: &transport,
		Timeout:   httpDefaultTimeout,
	}
	if !enableRedirects {
		c.CheckRedirect = func(req *http.Request, via []*http.Request) error {
			return fmt.Errorf("redirects forbidden: %s", req.URL.String())
		}
	}

	req, err := http.NewRequest("HEAD", url, nil)
	if err != nil {
		log.G(ctx).Error("NewRequest failed", zap.Error(err))
		return nil, misc.ErrUnreachableSource
	}
	req = req.WithContext(ctx)

	r, err := c.Do(req)
	if err != nil {
		log.G(ctx).Error("Head failed", zap.Error(err))
		return nil, misc.CheckContextError(ctx, misc.ErrUnreachableSource, true)
	}
	//nolint:errcheck
	defer r.Body.Close()

	// Second existence check for 'dummy' S3.
	if r.StatusCode != http.StatusOK {
		log.G(ctx).Error("head failed",
			zap.Int("code", r.StatusCode), zap.Error(misc.ErrUnreachableSource))
		return nil, misc.ErrUnreachableSource
	}

	return r, nil
}

type imageInfo struct {
	VirtualSize int64 `json:"virtual-size"`
}

type readerSettings struct {
	ReadAhead int
	ChunkSize int // must be divisor of defaultChunkSize
}

type httpImage struct {
	url       string
	format    string
	rs        readerSettings
	proxySock string
	blockMap  directreader.BlocksMap
}

func newHTTPImage(ctx context.Context, url, format string, enableRedirects bool, proxySock string, blocksMap directreader.BlocksMap) (nbd.Image, error) {
	ctx = log.WithLogger(ctx, log.G(ctx).With(zap.String("url", url)))

	img := &httpImage{
		url:       url,
		format:    format,
		proxySock: proxySock,
		blockMap:  blocksMap,
	}

	r, err := headImage(ctx, url, enableRedirects, proxySock)
	if err != nil {
		log.G(ctx).Error("headImage error", zap.Error(err))
		return img, err
	}

	// qemu-nbd requires byte ranges be supported on server
	v, ok := r.Header["Accept-Ranges"]
	if !ok || len(v) == 0 || v[0] != "bytes" {
		err = misc.ErrSourceNoRange
		log.G(ctx).Error("newHTTPImage failed: server does not support byte ranges.",
			zap.Error(err))
		return img, err
	}

	img.rs, err = img.getSettings(ctx, r.ContentLength)
	if err != nil {
		log.G(ctx).Error("Can't get http image settings.", zap.Error(err))
	}
	return img, err
}

func (img *httpImage) GetQemuBlockdev(ctx context.Context) (string, error) {
	return img.getBlockdev(ctx, img.rs.ReadAhead, qemuDefaultTimeout)
}

func (img *httpImage) GetChunkSize() int {
	return img.rs.ChunkSize
}

func (img *httpImage) GetFormat() string {
	return img.format
}

func (img *httpImage) GetReaderCount(maxCount int) int {
	// Empirical formula: https://st.yandex-team.ru/CLOUD-10949
	var count int
	switch {
	case img.rs.ReadAhead < 2*misc.MB:
		count = 2
	case img.rs.ReadAhead < 7*misc.MB/2:
		count = 4
	case img.rs.ReadAhead < 4*misc.MB:
		count = 8
	}
	if count == 0 || count > maxCount {
		count = maxCount
	}
	return count
}

func (img *httpImage) getBlockdev(ctx context.Context, readahead int, timeout time.Duration) (string, error) {
	blkdev, err := directreader.CreateQemuJSONTarget(img.url, readahead, timeout)
	if err != nil {
		log.G(ctx).Error("getBlockdev failed", zap.String("url", img.url), zap.Error(err))
	}
	return blkdev, err
}

func (img *httpImage) getSettings(ctx context.Context, actualSize int64) (readerSettings, error) {
	log.G(ctx).Debug("Collecting image settings")

	rs := readerSettings{
		ReadAhead: 0,
		ChunkSize: storage.DefaultChunkSize,
	}

	vsize := img.blockMap.VirtualSize()

	clusters := img.blockMap.CountBlocksWithData()
	if clusters == 0 {
		rs.ReadAhead = storage.DefaultChunkSize
		rs.ChunkSize = qemuChunkSize
		return rs, nil
	}

	// Maximum total overhead per stripe
	// https://st.yandex-team.ru/CLOUD-1862
	overhead := actualSize / clusters

	// Round to qemu native
	ra := overhead / qemuChunkSize * qemuChunkSize

	// Large readahead does not make sense
	if ra > storage.DefaultChunkSize {
		ra = storage.DefaultChunkSize
	}

	if ra == 0 {
		// Optimal for non-readaheads
		rs.ChunkSize = storage.DefaultChunkSize
	} else {
		// Optimal for readaheads
		rs.ChunkSize = qemuChunkSize
	}

	rs.ReadAhead = int(ra)
	log.G(ctx).Info("Collected image settings", zap.Int64("ActualSize", actualSize),
		zap.Int64("VirtualSize", vsize), zap.Int64("Clusters", clusters),
		zap.Int("ReadAhead", rs.ReadAhead), zap.Int("ChunkSize", rs.ChunkSize))

	return rs, nil
}

func trimBytesForLog(bytesString []byte) []byte {
	const maxLength = 1000
	if len(bytesString) > maxLength {
		bytesString = bytesString[:maxLength]
	}
	return bytesString
}
