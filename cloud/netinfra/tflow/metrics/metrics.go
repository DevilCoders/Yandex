package metrics

import (
	"fmt"
	"sync/atomic"
	"time"

	"github.com/prometheus/client_golang/prometheus"
)

// Metrica is the struct definitioan for application Metrics
type Metrica struct {
	Launched         int64
	UDPPacketsServed uint64
	UDPBytesServed   uint64
	SFlowPackets     uint64
	SFlowBytes       uint64
	RawHeaders       uint64
}

// Metrics some internal statistics for monitoring purpouse
var (
	Metrics            Metrica
	MetricTrafficBytes = prometheus.NewCounterVec(
		prometheus.CounterOpts{
			Name: "flow_traffic_bytes",
			Help: "Bytes received by the application.",
		},
		[]string{"remote_ip"},
	)
	MetricTrafficPackets = prometheus.NewCounterVec(
		prometheus.CounterOpts{
			Name: "flow_traffic_packets",
			Help: "Packets received by the application.",
		},
		[]string{"remote_ip"},
	)
	MetricPacketSizeSum = prometheus.NewSummaryVec(
		prometheus.SummaryOpts{
			Name:       "flow_traffic_summary_size_bytes",
			Help:       "Summary of packet size.",
			Objectives: map[float64]float64{0.5: 0.05, 0.9: 0.01, 0.99: 0.001},
		},
		[]string{"remote_ip"},
	)
)

func init() {
	Metrics.Launched = time.Now().Unix()
	prometheus.MustRegister(MetricTrafficBytes)
	prometheus.MustRegister(MetricTrafficPackets)
	prometheus.MustRegister(MetricPacketSizeSum)
}

func (*Metrica) Dump() {
	now := time.Now().Unix()
	fmt.Printf("collector_uptime %d\n", now-Metrics.Launched)
	fmt.Printf("sflow_packets %d\n", atomic.LoadUint64(&Metrics.SFlowPackets))
	fmt.Printf("sflow_bytes %d\n", atomic.LoadUint64(&Metrics.SFlowBytes))
	fmt.Printf("raw_headers %d\n", atomic.LoadUint64(&Metrics.RawHeaders))
}
