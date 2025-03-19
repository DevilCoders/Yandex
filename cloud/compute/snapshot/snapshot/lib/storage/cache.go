package storage

import (
	"a.yandex-team.ru/cloud/compute/snapshot/pkg/cache"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/misc"
)

// SnapshotChunksCache is a wrapper around generic cache to avoid type casts
type SnapshotChunksCache struct {
	cache.Cache
}

// NewSnapshotChunksCache returns a SnapshotChunksCache instance.
func NewSnapshotChunksCache(size int) (SnapshotChunksCache, error) {
	arccache, err := cache.NewARC(size)
	if err != nil {
		return SnapshotChunksCache{}, err
	}

	return SnapshotChunksCache{arccache}, nil
}

// Get returns a cached map.
func (c SnapshotChunksCache) Get(id string) (ChunkMap, bool) {
	chunks, ok := c.Cache.Get(id)
	if !ok {
		misc.SnapshotCacheMiss.Inc()
		// NOTE: newChunkMap makes heap allocation,
		// so I would better return zerovalue of lib.ChunkMap with nil ptr
		return ChunkMap{}, ok
	}

	misc.SnapshotCacheHit.Inc()
	return chunks.(ChunkMap), ok
}

// Add adds a chunk map to cache.
func (c SnapshotChunksCache) Add(id string, chunks ChunkMap) {
	c.Cache.Add(id, chunks)
}

// Remove deletes a chunk map from cache.
func (c SnapshotChunksCache) Remove(id string) {
	c.Cache.Remove(id)
}
