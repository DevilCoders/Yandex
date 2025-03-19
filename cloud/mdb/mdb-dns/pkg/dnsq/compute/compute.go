package compute

import (
	"context"
	"fmt"
	"sync/atomic"
	"time"

	"github.com/karlseguin/ccache/v2"
	"github.com/opentracing/opentracing-go"

	"a.yandex-team.ru/cloud/mdb/internal/compute/dns"
	"a.yandex-team.ru/cloud/mdb/internal/tracing"
	"a.yandex-team.ru/cloud/mdb/internal/tracing/tags"
	"a.yandex-team.ru/cloud/mdb/mdb-dns/pkg/dnsq"
	"a.yandex-team.ru/library/go/core/log"
)

var _ dnsq.Client = &client{}

const (
	cacheListMinRecs = 2
	cacheStatMask    = 0xfff
)

type client struct {
	logger log.Logger
	dca    dns.Client
	cache  *ccache.Cache
	cTTL   time.Duration

	// cache statistic
	cMiss uint64
	cHit  uint64
}

// New constructs new compute client
func New(logger log.Logger, dca dns.Client, cachettl time.Duration) dnsq.Client {
	return &client{
		logger: logger,
		dca:    dca,
		cache:  ccache.New(ccache.Configure()),
		cTTL:   cachettl,
	}
}

func (c *client) LookupCNAME(ctx context.Context, fqdn, groupid string) (cname string, ok bool) {
	span, ctx := opentracing.StartSpanFromContext(
		ctx,
		"Compute Lookup CNAME",
		tags.NetworkID.Tag(groupid),
		tags.CNAMEFqdn.Tag(fqdn),
	)
	defer span.Finish()

	netid := groupid

	var err error
	var lr dns.MapRec
	ci := c.cache.Get(netid)
	if ci == nil || ci.Expired() {
		lr, err = c.dca.ListRecords(ctx, netid)
		if err != nil {
			tracing.SetErrorOnSpan(span, err)
			c.logger.Errorf("failed to get list records for compute net: '%s', %s", netid, err)
			return "", false
		}

		if len(lr) >= cacheListMinRecs {
			atomic.AddUint64(&c.cMiss, 1)
			c.cache.Set(netid, lr, c.cTTL)
		}
	} else {
		atomic.AddUint64(&c.cHit, 1)
		lr = ci.Value().(dns.MapRec)
	}
	if stat := c.GetCacheStat(); stat != "" {
		c.logger.Info(stat)
	}

	data := lr[fqdn]
	return data.Value, true
}

func (c *client) GetCacheStat() string {
	ch := atomic.LoadUint64(&c.cHit)
	cm := atomic.LoadUint64(&c.cMiss)
	if (ch+cm)&cacheStatMask != cacheStatMask {
		return ""
	}
	cr := 100 * float64(ch) / float64(ch+cm)
	return fmt.Sprintf("cache statistic, ratio: %.1f, hit: %d, miss: %d, cache size: %d", cr, ch, cm, c.cache.ItemCount())
}
