package system

type BaseMetric struct {
	Timestamp int64
}

type CPUMetric struct {
	BaseMetric
	Used float64
}

type MemoryMetric struct {
	BaseMetric
	Used  int64
	Total int64
}

type DiskMetric struct {
	BaseMetric
	Used  int64
	Total int64
}

type Metrics struct {
	CPU    *CPUMetric
	Memory *MemoryMetric
	Disk   *DiskMetric
}
