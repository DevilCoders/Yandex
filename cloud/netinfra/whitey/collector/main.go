package collector

import (
	"time"
	"whitey/bgp"
	"whitey/push"
	"whitey/ua"
)

// Collector interface
// Each collector MUST implement this
type Collector interface {
	GetInterval() time.Duration
	SetInterval(time.Duration)
	GetTasks() []Task
	AddTask(Task)

	Collect() error
	Drop()

	GetData() Data
}

// Task interface type
// Each collector task MUST implement this
type Task interface {
	SetPushType(push.Type)
	SetEndpoint(string)
	SetCollector(Collector)

	GetMetrics() error
	Push() error
}

// Data container
// Add new type whenever new data type is added
type Data struct {
	BGPPeers []bgp.Peer
}

// Metrics container
// Add new type whenever new push type is added
type Metrics struct {
	UA []ua.Metric
}
