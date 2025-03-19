package main

import (
	"encoding/json"
	"io/ioutil"
	"os"
)

const lastFilePath = "/tmp/config-monrun-cpu-check.go.last"

// Container содержит данные из /proc/stat и /proc/<pid>/stat
type Container struct {
	ProcStat   ProcStat
	PidStatMap PidStatMap
}

func newContainer() Container {
	return Container{
		PidStatMap: NewPidStatMap(),
		ProcStat:   ProcStat{},
	}
}

func (x *Container) parseJSON(data []byte) Container {
	var ret Container
	_ = json.Unmarshal(data, &ret)
	return ret
}

func (x *Container) loadFromDisk() error {
	data, err := ioutil.ReadFile(lastFilePath)
	if err != nil {
		return err
	}
	*x = x.parseJSON(data)
	return err
}

func (x *Container) saveToDisk() {
	data, _ := json.MarshalIndent(x, "", "    ")
	fh, _ := os.Create(lastFilePath)
	defer func() { _ = fh.Close() }()
	_, _ = fh.Write(data)
}

// Cluster - это структура, содержая два контейнера (один с текущими данными, второй - с прошлыми)
type Cluster struct {
	Runtime Container
	Cache   Container
}

// NewCluster ...
func NewCluster() Cluster {
	return Cluster{
		Runtime: newContainer(),
		Cache:   newContainer(),
	}
}

func (x *Cluster) cpuLoad() ProcStat {
	// Функция возвращает структуру типа ProcStat, в которой содержатся
	// разницы процессорных метрик /proc/stat, выраженных в процентах
	diff := x.Runtime.ProcStat.diff(x.Cache.ProcStat)
	return diff.toPercent()
}
