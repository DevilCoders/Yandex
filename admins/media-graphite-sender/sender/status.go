package sender

import (
	"encoding/json"
	"fmt"
	"net/http"
	"strconv"
	"strings"
	"time"

	"gopkg.in/yaml.v2"

	"github.com/c2h5oh/datasize"
	"github.com/prometheus/client_golang/prometheus"
	"github.com/prometheus/client_golang/prometheus/promauto"
	"github.com/prometheus/client_golang/prometheus/promhttp"
	dto "github.com/prometheus/client_model/go"
)

var startupTime = time.Now()

var (
	metricsReceive = promauto.NewCounter(prometheus.CounterOpts{
		Name: "sender_receive_metrics_total",
		Help: "The total number of received metrics",
	})
	metricsDelay = promauto.NewCounter(prometheus.CounterOpts{
		Name: "sender_delay_metrics_total",
		Help: "The total number of delayed metrics",
	})
	metricsDrop = promauto.NewCounter(prometheus.CounterOpts{
		Name: "sender_drop_metrics_total",
		Help: "The total number of dropped metrics",
	})
	metricsInvalid = promauto.NewCounter(prometheus.CounterOpts{
		Name: "sender_invalid_metrics_total",
		Help: "The total number of invalid metrics",
	})
	metricsLastReceive = promauto.NewGauge(prometheus.GaugeOpts{
		Name: "sender_last_receive_timestamp",
		Help: "Time of the last receive",
	})
	metricsBufCount = promauto.NewGauge(prometheus.GaugeOpts{
		Name: "sender_bufs_total",
		Help: "Number of buffers alive",
	})
	metricsBufsSize = promauto.NewGauge(prometheus.GaugeOpts{
		Name: "sender_bufs_allocated_size_total",
		Help: "Total allocated size of the alive buffers",
	})
	metricsLastSuccessSend = promauto.NewGauge(prometheus.GaugeOpts{
		Name: "sender_last_success_send_timestamp",
		Help: "Time of the last successful send",
	})
	metricsSendToServer = promauto.NewCounterVec(
		prometheus.CounterOpts{
			Name: "sender_send_to_server_total",
			Help: "The total number of sended metrics",
		},
		[]string{"server"},
	)
	metricsInvalidRate = promauto.NewSummary(prometheus.SummaryOpts{
		Name: "sender_invalid_metric_summary",
		Help: "The summary invalid metrics for rate calculating",
	})
	metricsDropRate = promauto.NewSummary(prometheus.SummaryOpts{
		Name: "sender_drop_metric_summary",
		Help: "The summary drop metrics for rate calculating",
	})

	metricsOperationDurations = promauto.NewHistogramVec(
		prometheus.HistogramOpts{
			Name:    "sender_operation_duration_seconds",
			Help:    "Operation duration distribution",
			Buckets: prometheus.LinearBuckets(10, 20, 8),
		},
		[]string{"op_type"},
	)
)

func metricsDuration(label string, from time.Time) {
	metricsOperationDurations.
		WithLabelValues(label).
		Observe(float64(time.Since(from).Milliseconds()))
}

// startMonitoringExporter exports sender status over http
func (s *Sender) startMonitoringExporter() {
	port := s.config.Status.Port
	http.Handle("/status", promhttp.Handler())
	http.HandleFunc("/monrun", monrunHandler(s.config))
	http.HandleFunc("/settings", settingsHandler(s.config))
	logger.Infof("Launching status thread on port=%d", port)
	err := http.ListenAndServe(":"+strconv.Itoa(port), nil)
	if err != nil {
		logger.Fatalf("Can not start status thread: %v", err)
	}
}

// settingsHandler export config as json or yaml
func settingsHandler(config *Config) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		q := r.URL.Query()
		format := q.Get("format")

		if format == "yaml" {
			resp, err := yaml.Marshal(config)
			if err != nil {
				msg := fmt.Sprintf("Failed to dump yaml: %s", err)
				logger.Errorf("settings handler: %s", msg)
				w.WriteHeader(http.StatusInternalServerError)
				_, _ = w.Write([]byte(msg))
			}
			_, _ = w.Write(resp)
		} else {
			_ = json.NewEncoder(w).Encode(config)
		}
	}
}

func monrunHandler(config *Config) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {

		var level int
		var msg []string

		m := dto.Metric{}
		_ = metricsBufsSize.Write(&m)
		usageMemoryBytes := datasize.ByteSize(uint64(m.GetGauge().GetValue()))
		logger.Debugf("Current buffers total allocated size=%s", usageMemoryBytes.HR())
		if usageMemoryBytes > config.Status.MaxMemoryUsage {
			level = 2
			msg = append(msg, fmt.Sprintf("Memory usage by buffers %s", usageMemoryBytes.HR()))
		}

		m.Reset()
		_ = metricsLastReceive.Write(&m)
		lastReceiveTime := int64(m.GetGauge().GetValue())
		if lastReceiveTime != 0 {
			m.Reset()
			_ = metricsLastSuccessSend.Write(&m)
			lastSendTS := int64(m.GetGauge().GetValue())
			lastSend := time.Unix(lastSendTS, 0)
			if lastSendTS == 0 {
				lastSend = startupTime
			}
			offlinePeriod := time.Since(lastSend)
			logger.Debugf("Last sent %s ago", offlinePeriod)
			if time.Unix(lastReceiveTime, 0).After(lastSend) {
				if offlinePeriod > config.Status.MaxOfflineTime {
					level = 2
					msg = append(msg, fmt.Sprintf("Offline period %s", offlinePeriod))
				}
			}
		}

		sendIntervalSeconds := config.Sender.SendInterval.Seconds()

		m.Reset()
		_ = metricsDropRate.Write(&m)
		dropPerSample := m.GetSummary().GetSampleSum() / float64(m.GetSummary().GetSampleCount())
		dropRps := dropPerSample / sendIntervalSeconds
		logger.Debugf("Current drop rps=%0.2f", dropRps)
		if dropRps > config.Status.MaxDropRps {
			level = 2
			msg = append(msg, fmt.Sprintf("Drop rps %0.3f", dropRps))
		}

		m.Reset()
		_ = metricsInvalidRate.Write(&m)
		invalidPerSample := m.GetSummary().GetSampleSum() / float64(m.GetSummary().GetSampleCount())
		invalidRps := invalidPerSample / sendIntervalSeconds
		logger.Debugf("Current invalid metric rps=%0.2f", invalidRps)
		if invalidRps > config.Status.MaxInvalidRps {
			level = 2
			msg = append(msg, fmt.Sprintf("Invalid mertics rps %0.3f", invalidRps))
		}
		if len(msg) == 0 {
			msg = append(msg, "OK")
		}

		_, _ = fmt.Fprintf(w, "%d;%s", level, strings.Join(msg, ","))
	}
}
