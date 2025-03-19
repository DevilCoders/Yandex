package hashring

import (
	"crypto/md5"
	"encoding/binary"
	"fmt"
)

type HashKey interface {
	Less(other HashKey) bool
}
type HashKeyOrder []HashKey

func (h HashKeyOrder) Len() int      { return len(h) }
func (h HashKeyOrder) Swap(i, j int) { h[i], h[j] = h[j], h[i] }
func (h HashKeyOrder) Less(i, j int) bool {
	return h[i].Less(h[j])
}

type hashFunc func([]byte) HashKey

type uint32HashKey uint32

func (k uint32HashKey) Less(other HashKey) bool {
	return k < other.(uint32HashKey)
}

var compatibleHashFunc = func() hashFunc {
	hashFunc, err := NewHash(md5.New).Use(newUInt32HashKey)
	if err != nil {
		panic(fmt.Sprintf("failed to create compatibleHashFunc: %s", err.Error()))
	}
	return hashFunc
}()

var compatibleReplicaHashFuncs = []hashFunc{
	compatibleHashForReplica(0),
	compatibleHashForReplica(1),
	compatibleHashForReplica(2),
	compatibleHashForReplica(3),
}

func newUInt32HashKey(bytes []byte) (HashKey, error) {
	return uint32HashKey(binary.LittleEndian.Uint32(bytes[:4])), nil
}

func newUInt32HashKeyForReplica(replica int) func(bytes []byte) (HashKey, error) {
	return func(bytes []byte) (HashKey, error) {
		return uint32HashKey(binary.LittleEndian.Uint32(bytes[replica*4 : (replica+1)*4])), nil
	}
}

func compatibleHashForReplica(replica int) hashFunc {
	hashFunc, err := NewHash(md5.New).Use(newUInt32HashKeyForReplica(replica))
	if err != nil {
		panic(fmt.Sprintf("failed to create compatibleHashFunc for replica %d: %s", replica, err.Error()))
	}
	return hashFunc
}
