package metrics

import (
	"fmt"
	"time"
)

var (
	rpcMillisecondBuckets = joinBuckets(
		millisecondBuckets(time.Millisecond, 10, 25, 50, 75, 100, 250, 500, 750),
		millisecondBuckets(time.Second, 1, 2.5, 5, 7.5, 10, 15, 20, 25, 30, 45),
		millisecondBuckets(time.Minute, 1),
	)
)

func joinBuckets(bkts ...[]float64) (result []float64) {
	for _, b := range bkts {
		result = append(result, b...)
	}
	for i := range result {
		if i == 0 {
			continue
		}
		if result[i] <= result[i-1] {
			panic(fmt.Sprintf("buckets has incorrect order at index %d (%f <= %f)", i, result[i], result[i-1]))
		}
	}
	return result
}

func millisecondBuckets(d time.Duration, values ...float64) []float64 {
	ms := d.Milliseconds()
	result := make([]float64, len(values))
	for i, v := range values {
		result[i] = v * float64(ms)
	}
	return result
}
