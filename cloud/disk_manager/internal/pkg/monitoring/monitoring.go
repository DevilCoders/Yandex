package monitoring

import (
	"context"
	"fmt"
	"io/ioutil"
	"net/http"
	"net/http/pprof"
	"os"
	"strconv"
	"time"

	"github.com/golang/protobuf/proto"
	"github.com/shirou/gopsutil/v3/process"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/logging"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/monitoring/config"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/monitoring/metrics"
)

////////////////////////////////////////////////////////////////////////////////

type Monitoring struct {
	mux *http.ServeMux
	cfg *config.MonitoringConfig
}

func (m *Monitoring) Start(ctx context.Context) {
	metaMetricsRegistry := m.CreateSolomonRegistry("meta")
	m.reportVersions(ctx, metaMetricsRegistry)
	m.reportRestartsCount(ctx, metaMetricsRegistry)

	procMetricsRegistry := m.CreateSolomonRegistry("proc")
	go m.reportProcStats(ctx, procMetricsRegistry)

	m.registerProfilingHandlers()

	go func() {
		err := http.ListenAndServe(
			fmt.Sprintf(":%v", m.cfg.GetPort()),
			m.mux,
		)
		if err != nil {
			logging.Warn(
				ctx,
				"Failed to set up monitoring on %v err=%v",
				m.cfg.GetPort(),
				err,
			)
		}
	}()
}

func (m *Monitoring) CreateSolomonRegistry(path string) metrics.Registry {
	return metrics.CreateSolomonRegistry(
		m.mux,
		path,
	)
}

////////////////////////////////////////////////////////////////////////////////

func (m *Monitoring) getConfigVersion() (*config.PackageVersion, error) {
	packageVersion := &config.PackageVersion{}

	if len(m.cfg.GetConfigVersionFile()) == 0 {
		proto.Merge(packageVersion, defaultPackageVersion())
		return packageVersion, nil
	}

	err := parseVersion(m.cfg.GetConfigVersionFile(), packageVersion)
	if err != nil {
		return nil, err
	}

	return packageVersion, nil
}

func (m *Monitoring) getServerVersion() (*config.PackageVersion, error) {
	packageVersion := &config.PackageVersion{}

	if len(m.cfg.GetServerVersionFile()) == 0 {
		proto.Merge(packageVersion, defaultPackageVersion())
		return packageVersion, nil
	}

	err := parseVersion(m.cfg.GetServerVersionFile(), packageVersion)
	if err != nil {
		return nil, err
	}

	return packageVersion, nil
}

func fullVersionString(cfg *config.PackageVersion) string {
	return fmt.Sprintf("%d.%s", cfg.GetRevision(), cfg.GetVersion())
}

func (m *Monitoring) reportVersions(
	ctx context.Context,
	registry metrics.Registry,
) {

	configVersion, err := m.getConfigVersion()
	if err != nil {
		logging.Warn(ctx, "Failed to get config version: err=%v", err)
	} else {
		logging.Info(ctx, "Disk Manager config version: %v", configVersion)
		registry.WithTags(map[string]string{
			"revision": fullVersionString(configVersion),
		}).Gauge("configVersion").Set(1)
	}

	serverVersion, err := m.getServerVersion()
	if err != nil {
		logging.Warn(ctx, "Failed to get server version: err=%v", err)
	} else {
		logging.Info(ctx, "Disk Manager server version: %v", serverVersion)
		registry.WithTags(map[string]string{
			"revision": fullVersionString(serverVersion),
		}).Gauge("serverVersion").Set(1)
	}
}

func (m *Monitoring) reportRestartsCount(
	ctx context.Context,
	registry metrics.Registry,
) {

	restartsCount, err := parseAndIncrementRestartsCount(
		m.cfg.GetRestartsCountFile(),
	)
	if err != nil {
		logging.Warn(
			ctx,
			"Failed to parse and increment restarts count: err=%v",
			err,
		)
		return
	}

	registry.Gauge("restartsCount").Set(float64(restartsCount))
}

func (m *Monitoring) reportProcStats(
	ctx context.Context,
	registry metrics.Registry,
) {

	pid := int32(os.Getpid())
	proc, err := process.NewProcess(pid)
	if err != nil {
		logging.Warn(ctx, "Failed to find current process with pid=%v", pid)
		return
	}

	for {
		cpuTimesStat, err := proc.TimesWithContext(ctx)
		if err == nil {
			registry.Gauge("cpu/User").Set(float64(cpuTimesStat.User))
			registry.Gauge("cpu/System").Set(float64(cpuTimesStat.System))
			registry.Gauge("cpu/Idle").Set(float64(cpuTimesStat.Idle))
			registry.Gauge("cpu/Iowait").Set(float64(cpuTimesStat.Iowait))
			registry.Gauge("cpu/Irq").Set(float64(cpuTimesStat.Irq))
			registry.Gauge("cpu/Softirq").Set(float64(cpuTimesStat.Softirq))
		}

		memInfoStat, err := proc.MemoryInfoWithContext(ctx)
		if err == nil {
			registry.Gauge("mem/RSS").Set(float64(memInfoStat.RSS))
			registry.Gauge("mem/VMS").Set(float64(memInfoStat.VMS))
			registry.Gauge("mem/Locked").Set(float64(memInfoStat.Locked))
			registry.Gauge("mem/Swap").Set(float64(memInfoStat.Swap))
		}

		ioCountersStat, err := proc.IOCountersWithContext(ctx)
		if err == nil {
			registry.Gauge("io/ReadCount").Set(float64(ioCountersStat.ReadCount))
			registry.Gauge("io/WriteCount").Set(float64(ioCountersStat.WriteCount))
			registry.Gauge("io/ReadBytes").Set(float64(ioCountersStat.ReadBytes))
			registry.Gauge("io/WriteBytes").Set(float64(ioCountersStat.WriteBytes))
		}

		numFDs, err := proc.NumFDsWithContext(ctx)
		if err == nil {
			registry.Gauge("numFDs").Set(float64(numFDs))
		}

		netConnectionsStat, err := proc.ConnectionsWithContext(ctx)
		if err == nil {
			registry.Gauge("numConnections").Set(float64(len(netConnectionsStat)))
		}

		numCtxSwitchesStat, err := proc.NumCtxSwitchesWithContext(ctx)
		if err == nil {
			registry.Gauge("involuntaryCtxSwitches").Set(float64(numCtxSwitchesStat.Involuntary))
			registry.Gauge("voluntaryCtxSwitches").Set(float64(numCtxSwitchesStat.Voluntary))
		}

		numThreads, err := proc.NumThreadsWithContext(ctx)
		if err == nil {
			registry.Gauge("numThreads").Set(float64(numThreads))
		}

		<-time.After(time.Second)
	}
}

func (m *Monitoring) registerProfilingHandlers() {
	m.mux.HandleFunc("/debug/pprof/", pprof.Index)
	m.mux.HandleFunc("/debug/pprof/cmdline", pprof.Cmdline)
	m.mux.HandleFunc("/debug/pprof/profile", pprof.Profile)
	m.mux.HandleFunc("/debug/pprof/symbol", pprof.Symbol)
	m.mux.HandleFunc("/debug/pprof/trace", pprof.Trace)
}

////////////////////////////////////////////////////////////////////////////////

func defaultPackageVersion() *config.PackageVersion {
	revision := uint64(0)
	version := "undefined"

	return &config.PackageVersion{
		Revision: &revision,
		Version:  &version,
	}
}

func parseVersion(
	versionFileName string,
	version *config.PackageVersion,
) error {

	versionBytes, err := ioutil.ReadFile(versionFileName)
	if err != nil {
		return fmt.Errorf(
			"failed to read version file %v: %w",
			versionFileName,
			err,
		)
	}

	err = proto.UnmarshalText(string(versionBytes), version)
	if err != nil {
		return fmt.Errorf(
			"failed to parse version file %v as protobuf: %w",
			versionFileName,
			err,
		)
	}

	return nil
}

////////////////////////////////////////////////////////////////////////////////

func parseAndIncrementRestartsCount(
	restartsCountFilename string,
) (uint64, error) {

	if len(restartsCountFilename) == 0 {
		return 0, nil
	}

	restartsCountBytes, err := ioutil.ReadFile(restartsCountFilename)
	if err != nil {
		return 0, fmt.Errorf(
			"failed to read restarts count file %v: %w",
			restartsCountFilename,
			err,
		)
	}

	var restartsCount uint64 = 0

	if len(restartsCountBytes) != 0 {
		restartsCount, err = strconv.ParseUint(string(restartsCountBytes), 10, 64)
		if err != nil {
			return 0, fmt.Errorf(
				"failed to convert restartsCount from byte[] to uint64: %w",
				err,
			)
		}
	}

	restartsCount += 1

	err = ioutil.WriteFile(
		restartsCountFilename,
		[]byte(strconv.FormatUint(restartsCount, 10)),
		0644,
	)
	if err != nil {
		return 0, fmt.Errorf(
			"failed to write to restarts count file %v: %w",
			restartsCountFilename,
			err,
		)
	}

	return restartsCount, nil
}

////////////////////////////////////////////////////////////////////////////////

func CreateMonitoring(
	cfg *config.MonitoringConfig,
) *Monitoring {

	mux := http.NewServeMux()

	return &Monitoring{
		mux,
		cfg,
	}
}
