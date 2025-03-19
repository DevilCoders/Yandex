package conductorcache_test

import (
	"sync"
	"testing"
	"time"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/dom0discovery/conductorcache"
)

func TestSync(t *testing.T) {
	cache := conductorcache.NewCache()
	fqdn := "fqdn"
	wg := sync.WaitGroup{}
	wg.Add(3)
	go func() {
		for i := 0; i < 100; i++ {
			cache.UpdateHosts(map[string][]string{fqdn: make([]string, i)})
			time.Sleep(10 * time.Millisecond)
		}
		wg.Done()
	}()
	go func() {
		for i := 0; i < 100; i++ {
			cache.HostGroups(fqdn)
			time.Sleep(10 * time.Millisecond)
		}
		wg.Done()
	}()
	go func() {
		for i := 0; i < 100; i++ {
			cache.UpdatedAt()
			time.Sleep(10 * time.Millisecond)
		}
		wg.Done()
	}()
	wg.Wait()
}
