package prometheus

import (
	"encoding/json"
	"fmt"
	"math"
	"net/http"
	"strings"

	"github.com/prometheus/client_golang/prometheus"
	dto "github.com/prometheus/client_model/go"
	"github.com/prometheus/common/model"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const (
	counterTotalSuffix = "_ammv"
	counterDeltaSuffix = "_dmmm"
	gaugeSuffix        = "_ammv"
	quantileSuffix     = counterTotalSuffix
	sumSuffix          = counterTotalSuffix
	histogramSuffix    = quantileSuffix
)

// Errors that can be returned
var (
	ErrLabelFormat          = xerrors.NewSentinel("failed to format labels")
	ErrInvalidMetricType    = xerrors.NewSentinel("invalid metric type")
	ErrUnexpectedMetricType = xerrors.NewSentinel("unexpected metric type")
)

// YasmHandler converts prometheus metrics and writes a response in a yasm-compatible format
func YasmHandler(l log.Logger) http.Handler {
	return http.HandlerFunc(func(rw http.ResponseWriter, r *http.Request) {
		rw.Header().Add("Content-Type", "application/json; charset=utf-8")

		mfs, err := prometheus.DefaultGatherer.Gather()
		if err != nil {
			ctxlog.Error(r.Context(), l, "prometheus.Gather failed", log.Error(err))
			http.Error(rw, fmt.Sprintf("prometheus.Gather failed: %s", err), http.StatusInternalServerError)
			return
		}

		yasmStats, err := FormatYasmStats(mfs)
		if err != nil {
			ctxlog.Error(r.Context(), l, "yasm metric formatting failed", log.Error(err))
			http.Error(rw, fmt.Sprintf("yasm metric formatting failed: %s", err), http.StatusInternalServerError)
			return
		}

		enc := json.NewEncoder(rw)
		if err = enc.Encode(yasmStats); err != nil {
			ctxlog.Error(r.Context(), l, "yasm metric encoding failed", log.Error(err))
			http.Error(rw, fmt.Sprintf("yasm metric encoding failed: %s", err), http.StatusInternalServerError)
			return
		}
	})
}

// FormatYasmStats converts prometheus stats to yasm format
func FormatYasmStats(mfs []*dto.MetricFamily) ([][]interface{}, error) {
	var stats [][]interface{}
	for _, mf := range mfs {
		// Special handling for gRPC server and client metrics
		if strings.HasPrefix(mf.GetName(), "grpc_server") {
			formatted, err := formatGRPCServerMetrics(mf)
			if err != nil {
				return nil, err
			}

			stats = append(stats, formatted...)
			continue
		} else if strings.HasPrefix(mf.GetName(), "grpc_client") {
			formatted, err := formatGRPCClientMetrics(mf)
			if err != nil {
				return nil, err
			}

			stats = append(stats, formatted...)
			continue
		}

		for _, metric := range mf.GetMetric() {
			formatted, err := formatMetric(mf, metric)
			if err != nil {
				return nil, err
			}

			stats = append(stats, formatted...)
		}
	}

	return stats, nil
}

func formatGRPCServerMetrics(mf *dto.MetricFamily) ([][]interface{}, error) {
	var res [][]interface{}
	codeAggregates := make(map[string]float64)
	var codelessAggregate optional.Float64
	histogramAggregates := make(map[metricsMethodType]map[string]uint64)
	for _, metric := range mf.GetMetric() {
		labels := grpcLabelsFromMetric(metric)

		var value float64
		switch mf.GetType() {
		case dto.MetricType_COUNTER:
			value = metric.GetCounter().GetValue()
		case dto.MetricType_GAUGE:
			value = metric.GetGauge().GetValue()
		case dto.MetricType_HISTOGRAM:
			histogram := grpcHistogramFromMetric(metric)
			v, ok := histogramAggregates[labels.MetricsMethodType]
			if !ok {
				v = make(map[string]uint64)
				histogramAggregates[labels.MetricsMethodType] = v
			}

			for bound, histValue := range histogram {
				v[bound] += histValue
			}

			continue
		default:
			continue
		}

		if !labels.Code.Valid {
			codelessAggregate.Float64 += value
			codelessAggregate.Valid = true
		} else {
			codeAggregates[labels.Code.String] += value
		}
	}

	for code, value := range codeAggregates {
		switch mf.GetType() {
		case dto.MetricType_COUNTER:
			res = append(
				res,
				[]interface{}{mf.GetName() + "_grpc_code_" + code + counterTotalSuffix, value},
				[]interface{}{mf.GetName() + "_grpc_code_" + code + counterDeltaSuffix, value},
			)
		case dto.MetricType_GAUGE:
			res = append(
				res,
				[]interface{}{mf.GetName() + "_grpc_code_" + code + gaugeSuffix, value},
			)
		}
	}

	if codelessAggregate.Valid {
		switch mf.GetType() {
		case dto.MetricType_COUNTER:
			res = append(
				res,
				[]interface{}{mf.GetName() + counterTotalSuffix, codelessAggregate.Float64},
				[]interface{}{mf.GetName() + counterDeltaSuffix, codelessAggregate.Float64},
			)
		case dto.MetricType_GAUGE:
			res = append(
				res,
				[]interface{}{mf.GetName() + gaugeSuffix, codelessAggregate.Float64},
			)
		}
	}

	for methodType, aggregate := range histogramAggregates {
		for bound, value := range aggregate {
			res = append(
				res,
				[]interface{}{mf.GetName() + "_method_type_" + string(methodType) + "_" + model.BucketLabel + "_" + bound + histogramSuffix, value},
			)
		}
	}

	return res, nil
}

func formatGRPCClientMetrics(mf *dto.MetricFamily) ([][]interface{}, error) {
	var res [][]interface{}
	codeAggregates := make(map[string]map[string]float64)
	codelessAggregate := make(map[string]float64)
	for _, metric := range mf.GetMetric() {
		labels := grpcLabelsFromMetric(metric)

		var value float64
		switch mf.GetType() {
		case dto.MetricType_COUNTER:
			value = metric.GetCounter().GetValue()
		case dto.MetricType_GAUGE:
			value = metric.GetGauge().GetValue()
		default:
			continue
		}

		if !labels.Code.Valid {
			codelessAggregate[labels.Service] += value
		} else {
			v, ok := codeAggregates[labels.Service]
			if !ok {
				v = make(map[string]float64)
				codeAggregates[labels.Service] = v
			}
			v[labels.Code.String] += value
		}
	}

	for service, aggregate := range codeAggregates {
		for code, value := range aggregate {
			switch mf.GetType() {
			case dto.MetricType_COUNTER:
				res = append(
					res,
					[]interface{}{mf.GetName() + "_service_" + service + "_grpc_code_" + code + counterTotalSuffix, value},
					[]interface{}{mf.GetName() + "_service_" + service + "_grpc_code_" + code + counterDeltaSuffix, value},
				)
			case dto.MetricType_GAUGE:
				res = append(
					res,
					[]interface{}{mf.GetName() + "_service_" + service + "_grpc_code_" + code + gaugeSuffix, value},
				)
			}
		}
	}

	for service, value := range codelessAggregate {
		switch mf.GetType() {
		case dto.MetricType_COUNTER:
			res = append(
				res,
				[]interface{}{mf.GetName() + "_service_" + service + counterTotalSuffix, value},
				[]interface{}{mf.GetName() + "_service_" + service + counterDeltaSuffix, value},
			)
		case dto.MetricType_GAUGE:
			res = append(
				res,
				[]interface{}{mf.GetName() + "_service_" + service + gaugeSuffix, value},
			)
		}
	}

	return res, nil
}

type grpcLabels struct {
	Service           string
	Method            string
	MetricsMethodType metricsMethodType
	Type              string
	Code              optional.String
}

func grpcLabelsFromMetric(metric *dto.Metric) grpcLabels {
	labels := grpcLabels{MetricsMethodType: metricsMethodTypeInvalid}
	for _, l := range metric.GetLabel() {
		v := l.GetValue()
		switch l.GetName() {
		case "grpc_service":
			labels.Service = v
		case "grpc_method":
			labels.Method = v
		case "grpc_type":
			labels.Type = v
		case "grpc_code":
			labels.Code = optional.NewString(v)
		}
	}

	if strings.HasSuffix(strings.ToLower(labels.Type), "stream") {
		labels.MetricsMethodType = metricsMethodTypeStream
		return labels
	}

	methodLower := strings.ToLower(labels.Method)
	switch {
	case strings.HasPrefix(methodLower, "get") || strings.HasPrefix(methodLower, "check"):
		labels.MetricsMethodType = metricsMethodTypeGet
	case strings.HasPrefix(methodLower, "list"):
		labels.MetricsMethodType = metricsMethodTypeList
	default:
		labels.MetricsMethodType = metricsMethodTypeModify
	}

	return labels
}

type metricsMethodType string

const (
	metricsMethodTypeInvalid metricsMethodType = "invalid"
	metricsMethodTypeGet     metricsMethodType = "get"
	metricsMethodTypeList    metricsMethodType = "list"
	metricsMethodTypeModify  metricsMethodType = "modify"
	metricsMethodTypeStream  metricsMethodType = "stream"
)

var allowedGRPCHistogramBounds = map[string]struct{}{
	"0.01": {},
	"0.1":  {},
	"0.5":  {},
	"1":    {},
	"5":    {},
	"10":   {},
}

func grpcHistogramFromMetric(metric *dto.Metric) map[string]uint64 {
	infSeen := false
	res := make(map[string]uint64)
	for _, q := range metric.Histogram.Bucket {
		bound := fmt.Sprint(q.GetUpperBound())
		if _, ok := allowedGRPCHistogramBounds[bound]; !ok {
			continue
		}

		res[bound] = q.GetCumulativeCount()

		if math.IsInf(q.GetUpperBound(), +1) {
			infSeen = true
		}
	}

	if !infSeen {
		res["Inf"] = metric.Histogram.GetSampleCount()
	}

	return res
}

func formatMetric(mf *dto.MetricFamily, metric *dto.Metric) ([][]interface{}, error) {
	switch mf.GetType() {
	case dto.MetricType_COUNTER:
		if metric.Counter == nil {
			return nil, xerrors.Errorf("%s: %w", mf.GetName(), ErrInvalidMetricType)
		}
		return formatCounter(mf.GetName(), metric.GetLabel(), metric.Counter.GetValue())
	case dto.MetricType_GAUGE:
		if metric.Gauge == nil {
			return nil, xerrors.Errorf("%s: %w", mf.GetName(), ErrInvalidMetricType)
		}
		return formatGauge(mf.GetName(), metric.GetLabel(), metric.Gauge.GetValue())
	case dto.MetricType_UNTYPED:
		if metric.Untyped == nil {
			return nil, xerrors.Errorf("%s: %w", mf.GetName(), ErrInvalidMetricType)
		}
		return formatCounter(mf.GetName(), metric.GetLabel(), metric.Untyped.GetValue())
	case dto.MetricType_SUMMARY:
		formatted, err := formatSummaryMetric(mf.GetName(), metric)
		return formatted, err
	case dto.MetricType_HISTOGRAM:
		formatted, err := formatHistogramMetric(mf.GetName(), metric)
		return formatted, err
	default:
		return nil, xerrors.Errorf("%s: %w", mf.GetName(), ErrUnexpectedMetricType)
	}
}

func formatCounter(name string, lp []*dto.LabelPair, value interface{}) ([][]interface{}, error) {
	labels, err := LabelPairsToText(lp, "", "")
	if err != nil {
		return nil, xerrors.Errorf("%s: %w", name, ErrLabelFormat)
	}

	return [][]interface{}{
		{name + labels + counterTotalSuffix, value},
		{name + labels + counterDeltaSuffix, value},
	}, nil
}

func formatGauge(name string, lp []*dto.LabelPair, value interface{}) ([][]interface{}, error) {
	labels, err := LabelPairsToText(lp, "", "")
	if err != nil {
		return nil, xerrors.Errorf("%s: %w", name, ErrLabelFormat)
	}

	return [][]interface{}{{name + labels + gaugeSuffix, value}}, nil
}

func formatSummaryMetric(name string, metric *dto.Metric) ([][]interface{}, error) {
	var stats [][]interface{}
	if metric.Summary == nil {
		return nil, xerrors.Errorf("%s: %w", name, ErrInvalidMetricType)
	}

	for _, q := range metric.Summary.Quantile {
		labels, err := LabelPairsToText(metric.GetLabel(), model.QuantileLabel, fmt.Sprint(q.GetQuantile()))
		if err != nil {
			return nil, xerrors.Errorf("%s: %w", name, ErrLabelFormat)
		}
		stats = append(stats, []interface{}{name + labels + quantileSuffix, q.GetValue()})
	}

	stat, err := formatSum(name+"_sum", metric.GetLabel(), metric.Summary.GetSampleSum())
	if err != nil {
		return nil, err
	}
	stats = append(stats, stat...)

	stat, err = formatCounter(name+"_count", metric.GetLabel(), metric.Summary.GetSampleCount())
	if err != nil {
		return nil, err
	}
	stats = append(stats, stat...)
	return stats, nil
}

func formatHistogramMetric(name string, metric *dto.Metric) ([][]interface{}, error) {
	var stats [][]interface{}
	if metric.Histogram == nil {
		return nil, xerrors.Errorf("%s: %w", name, ErrInvalidMetricType)
	}

	infSeen := false

	for _, q := range metric.Histogram.Bucket {
		bucketname := name + "_bucket"
		labels, err := LabelPairsToText(metric.GetLabel(), model.BucketLabel, fmt.Sprint(q.GetUpperBound()))
		if err != nil {
			return nil, xerrors.Errorf("%s: %w", bucketname, ErrLabelFormat)
		}
		stats = append(stats, []interface{}{bucketname + labels + histogramSuffix, q.GetCumulativeCount()})

		if math.IsInf(q.GetUpperBound(), +1) {
			infSeen = true
		}
	}

	if !infSeen {
		bucketname := name + "_bucket"
		labels, err := LabelPairsToText(metric.GetLabel(), model.BucketLabel, "Inf")
		if err != nil {
			return nil, xerrors.Errorf("%s: %w", bucketname, ErrLabelFormat)
		}
		stats = append(stats, []interface{}{bucketname + labels + histogramSuffix, metric.Histogram.GetSampleCount()})
	}

	stat, err := formatSum(name+"_sum", metric.GetLabel(), metric.Histogram.GetSampleSum())
	if err != nil {
		return nil, err
	}
	stats = append(stats, stat...)

	stat, err = formatCounter(name+"_count", metric.GetLabel(), metric.Histogram.GetSampleCount())
	if err != nil {
		return nil, err
	}
	stats = append(stats, stat...)
	return stats, nil
}

func formatSum(name string, lp []*dto.LabelPair, value interface{}) ([][]interface{}, error) {
	labels, err := LabelPairsToText(lp, "", "")
	if err != nil {
		return nil, xerrors.Errorf("%s: %w", name, ErrLabelFormat)
	}
	return [][]interface{}{{name + labels + sumSuffix, value}}, nil
}
