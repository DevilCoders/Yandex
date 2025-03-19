package conductorcache

import (
	"sync"
	"time"
)

type Cache struct {
	hosts      map[string][]string
	updateTime time.Time
	lock       sync.Locker
}

func (c *Cache) HostGroups(fqdn string) ([]string, bool) {
	c.lock.Lock()
	defer c.lock.Unlock()
	groups, ok := c.hosts[fqdn]
	return groups, ok
}

func (c *Cache) UpdateHosts(hosts map[string][]string) {
	c.lock.Lock()
	defer c.lock.Unlock()
	c.hosts = hosts
	c.updateTime = time.Now()
}

func (c *Cache) UpdatedAt() time.Time {
	c.lock.Lock()
	defer c.lock.Unlock()
	return c.updateTime
}

func NewCache() *Cache {
	return &Cache{lock: &sync.Mutex{}}
}
