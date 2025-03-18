package inspection

import (
	"reflect"
	"sync"
)

var cache = inspectedCache{
	store: make(map[reflect.Type]Inspected),
}

// inspectedCache represents set of inspected objects grouped by their type
type inspectedCache struct {
	mu    sync.RWMutex
	store map[reflect.Type]Inspected
}

// Inspect returns Inspected data for given target using cache.
// It always returns non-nil object, but it can be empty if invalid interface given
func Inspect(target interface{}) *Inspected {
	var orig reflect.Value
	if vt, ok := target.(reflect.Value); ok {
		orig = vt
	} else {
		orig = reflect.ValueOf(target)
	}

	if !orig.IsValid() {
		return &Inspected{}
	}

	indir := indirectValue(orig)
	indirType := indir.Type()

	// check store with read lock
	if r := cache.load(indirType); r != nil {
		r.setValue(orig, indir)
		return r
	}

	// acquire lock to update store
	cache.mu.Lock()
	defer cache.mu.Unlock()

	// store Inspected struct to store
	rt := inspect(orig, indir, indirType)
	cache.store[indirType] = rt

	rt.setValue(orig, indir)
	return &rt
}

func (c *inspectedCache) load(key reflect.Type) *Inspected {
	c.mu.RLock()
	defer c.mu.RUnlock()
	if r, ok := c.store[key]; ok {
		return &r
	}
	return nil
}
