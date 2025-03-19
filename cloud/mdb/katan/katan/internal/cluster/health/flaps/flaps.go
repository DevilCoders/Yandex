package flaps

import (
	"fmt"
	"sort"
	"strings"
	"time"

	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/client"
	"a.yandex-team.ru/library/go/x/math"
)

type Verdict struct {
	IsFlapping     bool
	Status         client.ClusterStatus
	Interpretation string
}

type FlapAwareHealthObserver struct {
	healthTSMap            map[time.Time]bool
	history                []client.ClusterHealth
	numSignificantStatuses int
}

func NewFlapAwareHealthObserver(numSignicantStatuses int) FlapAwareHealthObserver {
	return FlapAwareHealthObserver{
		healthTSMap:            map[time.Time]bool{},
		history:                []client.ClusterHealth{},
		numSignificantStatuses: numSignicantStatuses,
	}
}

// Same health values can be read more than once. So we put them into a hash map to sort afterwards.
func (f *FlapAwareHealthObserver) LearnHealth(health client.ClusterHealth) {
	if _, ok := f.healthTSMap[health.Timestamp]; !ok {
		f.healthTSMap[health.Timestamp] = true
		f.history = append(f.history, health)
	}
}

func (f *FlapAwareHealthObserver) latestStatusChanges(limit int) []string {
	var statusChanges []string
	var prevTime time.Time
	for j := math.MaxInt(len(f.history)-limit, 0); j < len(f.history); j++ {
		health := f.history[j]
		var timeMark string
		if prevTime.IsZero() {
			timeMark = health.Timestamp.Format("15:04:05")
		} else {
			timeMark = fmt.Sprintf("+%s", health.Timestamp.Sub(prevTime).Round(time.Second))
		}
		prevTime = health.Timestamp
		statusChanges = append(statusChanges, fmt.Sprintf("%s %s", timeMark, health.Status))
	}
	return statusChanges
}

func (f *FlapAwareHealthObserver) Verdict() Verdict {
	sort.Slice(f.history, func(i, j int) bool {
		return f.history[i].Timestamp.Before(f.history[j].Timestamp)
	})
	resultStatus := client.ClusterStatusUnknown
	lenHistory := len(f.history)
	if lenHistory < f.numSignificantStatuses {
		return Verdict{
			IsFlapping:     true,
			Status:         client.ClusterStatusUnknown,
			Interpretation: fmt.Sprintf("only %d measurements were made, %d required", lenHistory, f.numSignificantStatuses),
		}
	}
	for i := 0; i < f.numSignificantStatuses; i++ {
		current := f.history[lenHistory-1-i].Status
		if i == 0 {
			resultStatus = current
		} else {
			if resultStatus != current {
				return Verdict{
					IsFlapping:     true,
					Status:         resultStatus,
					Interpretation: strings.Join(f.latestStatusChanges(f.numSignificantStatuses*2+1), " -> "),
				}
			}
		}
	}
	return Verdict{
		IsFlapping: false,
		Status:     resultStatus,
	}
}
