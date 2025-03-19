package blackboxauth

import (
	"time"

	"github.com/karlseguin/ccache/v2"

	"a.yandex-team.ru/cloud/mdb/internal/auth/blackbox"
)

// CCache is a simple cache with ttl
type CCache struct {
	cache *ccache.Cache
	ttl   time.Duration
}

// interface satisfaction check
var _ Cache = &CCache{}

// Get item from cache
func (c *CCache) Get(key string) (blackbox.UserInfo, error) {
	item := c.cache.Get(key)
	if item == nil || item.Expired() {
		return blackbox.UserInfo{}, ErrNotInCache
	}
	return item.Value().(blackbox.UserInfo), nil
}

// Put item into cache
func (c *CCache) Put(key string, value blackbox.UserInfo) {
	c.cache.Set(key, value, c.ttl)
}

// NewCCache is a simple BlackBoxCCache constructor
func NewCCache(ttl time.Duration, size int64) *CCache {
	return &CCache{
		cache: ccache.New(ccache.Configure().MaxSize(size)),
		ttl:   ttl,
	}
}
