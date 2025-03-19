package cache

import (
	lru "github.com/hashicorp/golang-lru"
)

// Cache is a general interface for various cache backends
type Cache interface {
	Get(key interface{}) (value interface{}, ok bool)
	Add(key interface{}, value interface{})
	Remove(key interface{})
	Len() int
}

// NewARC returns an enhanced version of LRU cache - ARCCache
func NewARC(size int) (Cache, error) {
	return lru.NewARC(size)
}
