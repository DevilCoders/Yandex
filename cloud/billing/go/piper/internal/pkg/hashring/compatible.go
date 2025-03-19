package hashring

import (
	"fmt"
	"sort"
)

type CompatibleHashring struct {
	keyPrefix  string
	partitions int
	vnodes     int
	replicas   int

	ring       map[HashKey]int
	sortedKeys []HashKey
}

func NewCompatible(keyPrefix string, partitions int) *CompatibleHashring {
	ring := &CompatibleHashring{
		keyPrefix:  keyPrefix,
		partitions: partitions,
		vnodes:     40,
		replicas:   4,
		ring:       make(map[HashKey]int),
	}
	ring.generateCircle()
	return ring
}

func (h *CompatibleHashring) Size() int {
	return h.partitions
}

func (h *CompatibleHashring) GenKey(key string) HashKey {
	return compatibleHashFunc([]byte(key))
}

func (h *CompatibleHashring) GetPartition(stringKey string) (partition int, ok bool) {
	pos, ok := h.GetNodePos(stringKey)
	if !ok {
		return -1, false
	}
	return h.ring[h.sortedKeys[pos]], true
}

func (h *CompatibleHashring) GetNodePos(stringKey string) (pos int, ok bool) {
	if len(h.ring) == 0 {
		return 0, false
	}

	key := h.GenKey(stringKey)

	nodes := h.sortedKeys
	pos = sort.Search(len(nodes), func(i int) bool { return key.Less(nodes[i]) })

	if pos == len(nodes) {
		// Wrap the search, should return First node
		return 0, true
	} else {
		return pos, true
	}
}

func (h *CompatibleHashring) generateCircle() {
	for nn := 0; nn < h.partitions; nn++ {
		nodeName := fmt.Sprintf("%s.%d", h.keyPrefix, nn)
		for vnn := 0; vnn < h.vnodes; vnn++ {
			wNodeName := fmt.Sprintf("%s-%d", nodeName, vnn)
			for r := 0; r < h.replicas; r++ {
				key := h.replicaHashKey(wNodeName, r)
				h.ring[key] = nn
				h.sortedKeys = append(h.sortedKeys, key)
			}
		}
		sort.Sort(HashKeyOrder(h.sortedKeys))
	}
}

func (h *CompatibleHashring) replicaHashKey(key string, replica int) HashKey {
	hashFnc := compatibleReplicaHashFuncs[replica]
	return hashFnc([]byte(key))
}
