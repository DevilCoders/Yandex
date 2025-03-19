package main

import (
	"fmt"
	"io/ioutil"
	"math"
	"path/filepath"
	"strconv"
	"strings"
)

// PidStat - структура для хранения данных из /proc/<pid>/stat
type PidStat struct {
	Pid   int
	Comm  string
	Utime float64
	Stime float64
}

func (pid *PidStat) parseLine(line string) {
	values := strings.Fields(line)
	if len(values) > 0 {
		*pid = PidStat{
			Pid:   strToInt(values[0]),
			Comm:  values[1],
			Utime: strToFloat64(values[13]),
			Stime: strToFloat64(values[14]),
		}
	}
}

func (pid *PidStat) updateFromFile(path string) {
	data, err := ioutil.ReadFile(path)
	if err != nil {
		return
	}
	line := strings.Split(string(data), "\n")[0]
	if len(data) > 0 {
		pid.parseLine(line)
	}
}

// PidStatMap - структура для хранения словаря из структур PidStat
type PidStatMap map[int]PidStat

// NewPidStatMap ...
func NewPidStatMap() PidStatMap {
	return make(PidStatMap)
}

func (pmap PidStatMap) update(item PidStat) {
	pmap[item.Pid] = item
}

func (pmap *PidStatMap) updateFromRuntime() {
	fileList := procfsPidStatFileList()
	for i := 0; i < len(fileList); i++ {
		pidStat := PidStat{}
		pidStat.updateFromFile(fileList[i])
		pmap.update(pidStat)
	}
}

func (pmap PidStatMap) diff(y PidStatMap) PidStatMap {
	/*
		Функция для вычисления разниц между текущими счетчиками PidStat и прошлыми
	*/
	ret := NewPidStatMap()
	for k := range pmap {
		// в разное время может не быть того или иного pid-а, это надо проверять
		if yPidStat, ok := y[k]; ok {
			ret[k] = PidStat{
				Pid:   k,
				Comm:  pmap[k].Comm,
				Utime: math.Abs(pmap[k].Utime - yPidStat.Utime),
				Stime: math.Abs(pmap[k].Stime - yPidStat.Stime),
			}
		}
	}
	return ret
}

func (pmap PidStatMap) topPidStat() PidStat {
	// Вычисляем pid с максимальным дифом между двумя запусками нашей утилиты,
	// это и есть самый жрущий процесс
	curMax := 0.0
	var ret PidStat
	for k := range pmap {
		if pmap[k].Utime+pmap[k].Stime > curMax {
			ret = pmap[k]
			curMax = pmap[k].Utime + pmap[k].Stime
		}
	}
	return ret

}

// ProcStat - Структура хранит данные из /proc/stat
// в структуре сознательно используется float, так как производится расчет процентов,
// в случае с int вычисления получаются неточными (разница в 2-3%)
type ProcStat struct {
	User    float64
	Nice    float64
	System  float64
	Idle    float64
	Iowait  float64
	Irq     float64
	Softirq float64
	Steal   float64
}

func (x *ProcStat) total() float64 {
	return x.User + x.Nice + x.System + x.Idle + x.Iowait + x.Irq + x.Softirq + x.Steal
}
func (x *ProcStat) nonIdle() float64 {
	return x.total() - x.Idle
}

func (x *ProcStat) parseLine(line string) {
	values := strings.Fields(line)
	*x = ProcStat{
		User:    strToFloat64(values[1]),
		Nice:    strToFloat64(values[2]),
		System:  strToFloat64(values[3]),
		Idle:    strToFloat64(values[4]),
		Iowait:  strToFloat64(values[5]),
		Irq:     strToFloat64(values[6]),
		Softirq: strToFloat64(values[7]),
		Steal:   strToFloat64(values[8]),
	}
}

func (x *ProcStat) updateFromFile(path string) {
	data, _ := ioutil.ReadFile(path)
	line := strings.Split(string(data), "\n")[0]
	x.parseLine(line)
}

func (x *ProcStat) updateFromRuntime() {
	x.updateFromFile("/proc/stat")
}

func (x *ProcStat) diff(y ProcStat) ProcStat {
	return ProcStat{
		User:    math.Abs(x.User - y.User),
		Nice:    math.Abs(x.Nice - y.Nice),
		System:  math.Abs(x.System - y.System),
		Idle:    math.Abs(x.Idle - y.Idle),
		Iowait:  math.Abs(x.Iowait - y.Iowait),
		Irq:     math.Abs(x.Irq - y.Irq),
		Softirq: math.Abs(x.Softirq - y.Softirq),
		Steal:   math.Abs(x.Steal - y.Steal),
	}
}

func (x *ProcStat) toPercent() ProcStat {
	total := x.total()
	return ProcStat{
		User:    x.User / total * 100.0,
		Nice:    x.Nice / total * 100.0,
		System:  x.System / total * 100.0,
		Idle:    x.Idle / total * 100.0,
		Iowait:  x.Iowait / total * 100.0,
		Irq:     x.Irq / total * 100.0,
		Softirq: x.Softirq / total * 100.0,
		Steal:   x.Steal / total * 100.0,
	}
}

func procfsPidStatFileList() []string {
	files, _ := filepath.Glob("/proc/[0-9]*/stat")
	return files
}

func cmdline(pid int, count int) string {
	out, err := ioutil.ReadFile(fmt.Sprintf("/proc/%d/cmdline", pid))
	// может  так получиться что pid-а уже нет, поэтому прочитать файл не получится
	if err != nil {
		return "<unknown>"
	}
	//если мы попали сюда, значит pid есть, и мы прочитали его cmdline, но он содержит зеробайты, их надо убрать
	ret := strings.Replace(string(out), "\x00", " ", -1)
	if len(ret) > count {
		return ret[:count]
	}
	return ret
}

func strToInt(s string) int {
	val, _ := strconv.Atoi(s)
	return val
}

func strToFloat64(s string) float64 {
	val, _ := strconv.ParseFloat(s, 64)
	return val
}
