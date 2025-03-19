package misc

import (
	"fmt"
	"github.com/rcrowley/go-metrics"
	"golang.org/x/xerrors"
	"io/ioutil"
	"runtime/pprof"
	"syscall"
)

// Current amount of opened files must be less the ulimit -n
// otherwise we can not accept connections
func fdHealthCheck(h metrics.Healthcheck) {
	var l syscall.Rlimit
	if err := syscall.Getrlimit(syscall.RLIMIT_NOFILE, &l); err != nil {
		h.Unhealthy(err)
		return
	}
	dirs, err := ioutil.ReadDir("/proc/self/fd")
	if err != nil {
		h.Unhealthy(xerrors.Errorf("problem during get open fd metric: %w", err))
		return
	}
	openFDs := len(dirs)
	FileDescriptors.Set(float64(openFDs))

	// use 80% as safe gap
	if float64(openFDs) >= float64(l.Cur)*0.8 {
		h.Unhealthy(fmt.Errorf("too many open files %d (max %d)", openFDs, l.Cur))
		return
	}

	h.Healthy()
}

// Spawned threads never exit, for example to support pdeathsig mechanism,
// so we need to control the amount to prevent panic when default limit is reached (10k)
// https://golang.org/pkg/runtime/debug/#SetMaxThreads
func threadHealthCheck(h metrics.Healthcheck) {
	threads := pprof.Lookup("threadcreate").Count()
	ThreadCounter.Set(float64(threads))
	if threads >= 5000 {
		h.Unhealthy(fmt.Errorf("too many OS threads %d (max 10000)", threads))
		return
	}

	h.Healthy()
}
