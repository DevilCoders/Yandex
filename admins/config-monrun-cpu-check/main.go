package main

import (
	"fmt"
	"os"
)

// Функции для вывода результата мониторинга

func (x *ProcStat) monStatus(config *Config) int {
	switch {
	case x.nonIdle() >= float64(config.Critical):
		return 2
	case x.nonIdle() >= float64(config.Warning):
		return 1
	default:
		return 0
	}
}

func monDescription(status int) string {
	return map[int]string{
		2: "CRIT",
		1: "WARN",
		0: "OK",
	}[status]
}

func (x *ProcStat) monrunOutput(config *Config) string {
	status := x.monStatus(config)
	return fmt.Sprintf(
		"%d; %s CPU USAGE %.1f%% (sys %.1f%%, user %.1f%%, io "+
			"%.1f%%, nice %.1f%%, idle %.1f%%, hi %.1f%%, si %.1f%%, st %.1f%%)",
		status,
		monDescription(status),
		x.nonIdle(),
		x.System,
		x.User,
		x.Iowait,
		x.Nice,
		x.Idle,
		x.Irq,
		x.Softirq,
		x.Steal,
	)
}

func main() {
	config := NewConfig()

	if config.Disabled {
		fmt.Println("0;Check disabled by `disabled` parameter in config.")
		os.Exit(0)
	}

	cluster := NewCluster()

	runtime := &cluster.Runtime
	cache := &cluster.Cache

	runtime.ProcStat.updateFromRuntime()
	runtime.PidStatMap.updateFromRuntime()

	err := cache.loadFromDisk()
	if err != nil {
		runtime.saveToDisk()
		_ = cache.loadFromDisk()
	}

	cpuLoad := cluster.cpuLoad()
	pidStatDiff := runtime.PidStatMap.diff(cache.PidStatMap)
	topPidStat := pidStatDiff.topPidStat()

	fmt.Printf("%s (top pid=%d, desc=%s)", cpuLoad.monrunOutput(config), topPidStat.Pid, cmdline(topPidStat.Pid, 30))

	runtime.saveToDisk()
}
