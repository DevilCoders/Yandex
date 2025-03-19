package actions

import (
	"sync"

	"github.com/valyala/fastjson"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/pkg/hashring"
)

var (
	parsers   = fastjson.ParserPool{}
	arenas    = fastjson.ArenaPool{}
	hashRings = hashPool{}
)

type hashPool struct {
	mu    sync.Mutex
	rings map[hrKey]*hashRing
}

type hrKey struct {
	prefix string
	parts  int
}

func (p *hashPool) getRing(nodePrefix string, partitions int) *hashRing {
	p.mu.Lock()
	defer p.mu.Unlock()
	if p.rings == nil {
		p.rings = make(map[hrKey]*hashRing)
	}
	key := hrKey{nodePrefix, partitions}
	r, ok := p.rings[key]
	if !ok {
		r = &hashRing{
			ring: hashring.NewCompatible(nodePrefix, partitions),
		}
		p.rings[key] = r
	}
	return r
}
