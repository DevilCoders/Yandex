package ydblock

import "github.com/prometheus/client_golang/prometheus"

var (
	createLockTimings = prometheus.NewHistogram(prometheus.HistogramOpts{
		Name: "ydblock_create_lock_duration_seconds",
		Help: "CreateLock() method timings",
	})
	checkHostLockTimings = prometheus.NewHistogram(prometheus.HistogramOpts{
		Name: "ydblock_check_host_lock_duration_seconds",
		Help: "CheckHostLock() method timings",
	})
	extendLockTimings = prometheus.NewHistogram(prometheus.HistogramOpts{
		Name: "ydblock_extend_lock_duration_seconds",
		Help: "ExtendLock() method timings",
	})
	releaseLockTimings = prometheus.NewHistogram(prometheus.HistogramOpts{
		Name: "ydblock_release_lock_duration_seconds",
		Help: "ReleaseLock() method timings",
	})
	internalGetLockedTimings = prometheus.NewHistogram(prometheus.HistogramOpts{
		Name: "ydblock_internal_get_locked_duration_seconds",
		Help: "getLocked() method timings",
	})
	internalCreateLockTimings = prometheus.NewHistogram(prometheus.HistogramOpts{
		Name: "ydblock_internal_create_lock_duration_seconds",
		Help: "createLock() method timings",
	})
	internalGetPreviouslySetLocksCountTimings = prometheus.NewHistogram(prometheus.HistogramOpts{
		Name: "ydblock_internal_get_previously_set_locks_count_duration_seconds",
		Help: "getPreviouslySetLocksCount() method timings",
	})
)

func registerMetrics(registry prometheus.Registerer) {
	registry.MustRegister(createLockTimings)
	registry.MustRegister(checkHostLockTimings)
	registry.MustRegister(extendLockTimings)
	registry.MustRegister(releaseLockTimings)
	registry.MustRegister(internalGetLockedTimings)
	registry.MustRegister(internalCreateLockTimings)
	registry.MustRegister(internalGetPreviouslySetLocksCountTimings)
}
