package performance

import (
	"time"
)

////////////////////////////////////////////////////////////////////////////////

const (
	MiB = uint64(1 << 20)
)

////////////////////////////////////////////////////////////////////////////////

func Estimate(storageSize, bandwidthMiBs uint64) time.Duration {
	bandwidthBytes := bandwidthMiBs * MiB
	return time.Duration(storageSize/bandwidthBytes) * time.Second
}
