package main

import (
	"encoding/json"
	"fmt"
	"io"
	"sync/atomic"
)

const InvalidAppID string = "invalid"

type Stats struct {
	ServerErrors  StatCounter
	LoggingErrors StatCounter
	ByAppID       map[string]*AppIDStats
}

type AppIDStats struct {
	IOSPasses              StatCounter
	IOSRejections          StatCounter
	IOSFailedAppleRequests StatCounter
	AndroidNonceRequests   StatCounter
	AndroidPasses          StatCounter
	AndroidRejections      StatCounter
}

type StatCounter struct {
	Value uint64
}

type statKeyCounter struct {
	key     string
	counter *StatCounter
}

func NewStats(appIDs []string) Stats {
	var stats Stats
	stats.ByAppID = make(map[string]*AppIDStats)

	for _, appID := range appIDs {
		stats.ByAppID[appID] = &AppIDStats{}
	}

	stats.ByAppID[InvalidAppID] = &AppIDStats{}

	return stats
}

func (stats *Stats) FindAppIDStats(appID string) *AppIDStats {
	appIDStats := stats.ByAppID[appID]

	if appIDStats == nil {
		appIDStats = stats.GetInvalidAppIDStats()
	}

	return appIDStats
}

func (stats *Stats) GetInvalidAppIDStats() *AppIDStats {
	return stats.ByAppID[InvalidAppID]
}

func (stats *Stats) Dump(writer io.Writer) error {
	rows := appendStatKeyCounters("", [][]interface{}{}, []statKeyCounter{
		statKeyCounter{"server_errors_deee", &stats.ServerErrors},
		statKeyCounter{"logging_errors_deee", &stats.LoggingErrors},
	})

	for appID, appIDStats := range stats.ByAppID {
		rows = appIDStats.Append(appID, rows)
	}

	return json.NewEncoder(writer).Encode(rows)
}

func (stats *AppIDStats) Append(appID string, rows [][]interface{}) [][]interface{} {
	prefix := fmt.Sprintf("app_id=%s;", appID)
	return appendStatKeyCounters(prefix, rows, []statKeyCounter{
		statKeyCounter{"ios_passes_deee", &stats.IOSPasses},
		statKeyCounter{"ios_rejections_deee", &stats.IOSRejections},
		statKeyCounter{"ios_failed_apple_requests_deee", &stats.IOSFailedAppleRequests},
		statKeyCounter{"android_nonce_requests_deee", &stats.AndroidNonceRequests},
		statKeyCounter{"android_passes_deee", &stats.AndroidPasses},
		statKeyCounter{"android_rejections_deee", &stats.AndroidRejections},
	})
}

func (counter *StatCounter) Inc() {
	atomic.AddUint64(&counter.Value, 1)
}

func (counter *StatCounter) Load() uint64 {
	return atomic.LoadUint64(&counter.Value)
}

func appendStatKeyCounters(prefix string, rows [][]interface{}, keyCounters []statKeyCounter) [][]interface{} {
	for _, keyCounter := range keyCounters {
		rows = append(rows, []interface{}{
			prefix + keyCounter.key,
			keyCounter.counter.Load(),
		})
	}

	return rows
}
