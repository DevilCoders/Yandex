package misc

import (
	"context"
	"fmt"
	"math"
	"time"

	"go.uber.org/zap"

	log "a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"

	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"

	"google.golang.org/grpc"

	"github.com/jonboulle/clockwork"
	"github.com/prometheus/client_golang/prometheus"

	"a.yandex-team.ru/cloud/compute/go-common/pkg/metriclabels"
)

var (
	names = metricsNames{}

	// https://godoc.org/github.com/prometheus/client_golang/prometheus#SummaryOpts
	// in map: key is approx percentile value is interval for real percentile
	DefObjectives  = map[float64]float64{0.75: 0.05, 0.9: 0.01, 0.99: 0.001}
	LongTaskBucket = BoundedBuckets(3600*10, 10)

	SpeedDataBuckets = CompoundBuckets(BoundedStepBuckets(1*MB, 10*MB, 1*MB),
		BoundedStepBuckets(10*MB, 300*MB, 10*MB))

	SpeedZeroBuckets = CompoundBuckets(
		BoundedStepBuckets(1*MB, 10*MB, 1*MB),
		BoundedStepBuckets(10*MB, 100*MB, 10*MB),
		BoundedStepBuckets(100*MB, 1000*MB, 100*MB),
		BoundedStepBuckets(1000*MB, 10000*MB, 1000*MB))

	TimeOperationBuckets = CompoundBuckets(
		BoundedStepBuckets(0, 200*time.Millisecond.Seconds(), 20*time.Millisecond.Seconds()),
		BoundedStepBuckets(200*time.Millisecond.Seconds(), 1000*time.Millisecond.Seconds(), 100*time.Millisecond.Seconds()),
		BoundedStepBuckets(time.Second.Seconds(), 5*time.Second.Seconds(), time.Second.Seconds()),
		BoundedStepBuckets(5*time.Second.Seconds(), 30*time.Second.Seconds(), 5*time.Second.Seconds()),
	)
	// Add your metric here
	// Gauges
	FileDescriptors = MustRegisterGauge("file_descriptors")
	ThreadCounter   = MustRegisterGauge("thread_counter")

	CurrentInManagerTasks = MustRegisterGauge("current_in_manager_tasks")
	CurrentRunningTasks   = MustRegisterGauge("current_running_tasks")
	CurrentWaitingTasks   = MustRegisterGauge("current_waiting_tasks")
	KikimrTransactions    = MustRegisterGauge("kikimr_running_transactions")

	// Counters
	LogErrorMessages        = MustRegisterCounter("log_error_messages")
	AuthorizationOk         = MustRegisterCounter("iam_authorization_ok")
	AuthorizationFailed     = MustRegisterCounter("iam_authorization_failed")
	SnapshotCacheMiss       = MustRegisterCounter("snapshot_cache_miss")
	SnapshotCacheHit        = MustRegisterCounter("snapshot_cache_hit")
	KikimrQueryErrors       = MustRegisterCounter("kikimr_query_errors")
	KikimrTransactionErrors = MustRegisterCounter("kikimr_transaction_errors")

	TaskRunNoError          = MustRegisterCounter("task_run_no_error")
	TaskRunHeartbeatTimeout = MustRegisterCounter("task_run_heartbeat_timeout")
	TaskRunPublicError      = MustRegisterCounter("task_run_public_error")
	TaskRunUnknownError     = MustRegisterCounter("task_run_unknown_error")

	GRPCRunNoError        = MustRegisterCounter("grpc_run_no_error")
	GRPCRunInvalidRequest = MustRegisterCounter("grpc_run_invalid_request")
	GRPCRunInternalError  = MustRegisterCounter("grpc_run_internal_error")

	// Storage speed
	NBSReadAtSpeed      = MustRegisterHistWithSummaryLabels("nbs_read_at_speed", SpeedDataBuckets, DefObjectives)
	NBSReadAtSpeedZero  = MustRegisterHistWithSummaryLabels("nbs_read_at_speed_zero", SpeedZeroBuckets, DefObjectives)
	NBSWriteAtSpeed     = MustRegisterHistWithSummaryLabels("nbs_write_at_speed", SpeedDataBuckets, DefObjectives)
	NBSWriteAtSpeedZero = MustRegisterHistWithSummaryLabels("nbs_write_at_speed_zero", SpeedZeroBuckets, DefObjectives)

	NBSReadZerosTimer     = MustRegisterTimer("nbs_read_zeroes_timer", prometheus.DefBuckets)
	NBSReadDataFullTimer  = MustRegisterTimer("nbs_read_data_full_timer", TimeOperationBuckets)
	NBSWriteDataFullTimer = MustRegisterTimer("nbs_write_data_full_timer", TimeOperationBuckets)
	NBSWriteDataTimer     = MustRegisterTimer("nbs_write_data_timer", TimeOperationBuckets)
	NBSWriteZerosTimer    = MustRegisterTimer("nbs_write_zeroes_timer", TimeOperationBuckets)

	SnapshotReadAtSpeed      = MustRegisterHistWithSummaryLabels("snapshot_read_at_speed", SpeedDataBuckets, DefObjectives)
	SnapshotReadAtSpeedZero  = MustRegisterHistWithSummaryLabels("snapshot_read_at_speed_zero", SpeedZeroBuckets, DefObjectives)
	SnapshotWriteAtSpeed     = MustRegisterHistWithSummaryLabels("snapshot_write_at_speed", SpeedDataBuckets, DefObjectives)
	SnapshotWriteAtSpeedZero = MustRegisterHistWithSummaryLabels("snapshot_write_at_speed_zero", SpeedZeroBuckets, DefObjectives)

	SnapshotReadAtTimer      = MustRegisterTimer("snapshot_read_at_timer", TimeOperationBuckets)
	SnapshotReadZeroAtTimer  = MustRegisterTimer("snapshot_read_at_zero_timer", TimeOperationBuckets)
	SnapshotWriteAtTimer     = MustRegisterTimer("snapshot_write_at_timer", TimeOperationBuckets)
	SnapshotWriteZeroAtTimer = MustRegisterTimer("snapshot_write_at_zero_timer", TimeOperationBuckets)

	ImageReadSpeed = MustRegisterHistWithGaugeLabels("image_read_speed", SpeedZeroBuckets)
	// Timers

	// chunker
	DecodeZeroChunkTimer = MustRegisterTimer("decode_zero_chunk_timer", TimeOperationBuckets)
	DecodeDataChunkTimer = MustRegisterTimer("decode_data_chunk_timer", TimeOperationBuckets)
	StoreDataChunkFull   = MustRegisterTimer("store_data_chunk_full", TimeOperationBuckets)
	StoreZeroChunkFull   = MustRegisterTimer("store_zero_chunk_full", TimeOperationBuckets)
	StoreDataChunkBlob   = MustRegisterTimer("store_data_chunk_blob", TimeOperationBuckets)
	StoreZeroChunkBlob   = MustRegisterTimer("store_zero_chunk_blob", TimeOperationBuckets)
	StoreFromStream      = MustRegisterTimer("store_from_stream", TimeOperationBuckets)
	ReadDataChunkTimer   = MustRegisterTimer("read_data_chunk_timer", TimeOperationBuckets)
	ReadZeroChunkTimer   = MustRegisterTimer("read_zero_chunk_timer", TimeOperationBuckets)
	UpdateChecksumTimer  = MustRegisterTimer("update_checksum_timer", TimeOperationBuckets)
	VerifyChecksumTimer  = MustRegisterTimer("verify_checksum_timer", TimeOperationBuckets)

	// facade
	DeleteSnapshot = MustRegisterHistWithGaugeTimer("delete_snapshot", TimeOperationBuckets)

	// kikimr
	KikimrReadChunkQueryTimer   = MustRegisterTimer("kikimr_read_chunk_query_timer", TimeOperationBuckets)
	GetChunksTimer              = MustRegisterTimer("get_chunks_timer", TimeOperationBuckets)
	KikimrGetSnapshotQueryTimer = MustRegisterTimer("kikimr_get_snapshot_query_timer", TimeOperationBuckets)
	BeginSnapshotTimer          = MustRegisterTimer("begin_snapshot_timer", TimeOperationBuckets)
	StoreChunkDB                = MustRegisterTimer("store_chunk_db", TimeOperationBuckets)
	EndSnapshotTimer            = MustRegisterTimer("end_snapshot_timer", TimeOperationBuckets)
	ClearSnapshot               = MustRegisterHistWithGaugeTimer("clear_snapshot", TimeOperationBuckets)
	GetSnapshotTimer            = MustRegisterTimer("get_snapshot_timer", TimeOperationBuckets)
	UpdateSnapshotStatus        = MustRegisterTimer("update_snapshot_status", TimeOperationBuckets)
	UpdateSnapshot              = MustRegisterTimer("update_snapshot", TimeOperationBuckets)
	KikimrRollbackQueryTimer    = MustRegisterTimer("kikimr_rollback_query_timer", TimeOperationBuckets)
	KikimrCommitQueryTimer      = MustRegisterTimer("kikimr_commit_query_timer", TimeOperationBuckets)
	KikimrTransactionTimer      = MustRegisterTimer("kikimr_transaction_timer", TimeOperationBuckets)
	KikimrQueryTimer            = MustRegisterTimer("kikimr_query_timer", TimeOperationBuckets)
	DeleteOwnChunksSpeed        = MustRegisterHistWithGaugeLabels("kikimr_delete_own_chunks_speed", SpeedDataBuckets)
	EndDeleteSnapshotTimer      = MustRegisterHistWithGaugeTimer("end_delete_snapshot", TimeOperationBuckets)
	PollUpdateLockTimer         = MustRegisterTimer("poll_update_lock_timer", TimeOperationBuckets)
	UnlockTimer                 = MustRegisterTimer("unlock_timer", TimeOperationBuckets)

	// move
	LoadChunkFromS3 = MustRegisterTimer("load_chunk_from_s3", TimeOperationBuckets)
	NBSStatTimer    = MustRegisterTimer("nbs_stat_timer", TimeOperationBuckets)

	ReadBlockMapTimer = MustRegisterHistWithGaugeTimer("read_block_map_timer", TimeOperationBuckets)
	ReadBlockMapItems = MustRegisterHistWithGaugeLabels("read_block_map_size", BoundedBuckets(100000, 10))
	ReadBlockMapBytes = MustRegisterHistWithGaugeLabels("read_block_map_bytes", BoundedBuckets(100*MB, 10))
	NBSMoveTaskTimer  = MustRegisterHistWithGaugeTimer("nbs_move_task_timer", TimeOperationBuckets)
	NBSMoveTaskSpeed  = MustRegisterHistWithGaugeLabels("nbs_move_task_speed", BoundedBuckets(GB, 10))
	URLMoveTaskTimer  = MustRegisterHistWithGaugeTimer("url_move_task_timer", LongTaskBucket)
	URLMoveTaskSpeed  = MustRegisterHistWithGaugeLabels("url_move_task_speed", BoundedBuckets(GB, 10))
	EndMoveTimer      = MustRegisterHistWithGaugeTimer("end_move_timer", LongTaskBucket)

	// copy
	EndShallowCopyTimer = MustRegisterTimer("end_shallow_copy_timer", LongTaskBucket)

	// other
	AuthorizeTimer = MustRegisterTimer("iam_authorize_timer", TimeOperationBuckets)
)

type timer struct {
	hist prometheus.Histogram
}

func (t *timer) Observe(c time.Duration) {
	t.hist.Observe(c.Seconds())
}

func (t *timer) ObserveSince(c time.Time) {
	t.Observe(time.Since(c))
}

func (t *timer) Start() *prometheus.Timer {
	return prometheus.NewTimer(t.hist)
}

type labeledTimer struct {
	hist *compoundMetric
}

func (t *labeledTimer) Start(ctx context.Context) *prometheus.Timer {
	return prometheus.NewTimer(t.hist.Observer(ctx))
}

type metricsNames map[string]struct{}

func (n metricsNames) add(name string) {
	if _, ok := n[name]; ok {
		panic(fmt.Sprintf("name %s found twice in metrics", name))
	}
	n[name] = struct{}{}
}

func CompoundBuckets(buckets ...[]float64) (res []float64) {
	for _, b := range buckets {
		res = append(res, b...)
	}
	return
}

func BoundedStepBuckets(start float64, end float64, width float64) []float64 {
	if end < start || width <= 0 {
		panic(fmt.Sprintf("invalid arguments: (%v, %v, %v)", start, end, width))
	}
	return prometheus.LinearBuckets(start, width, int(math.Floor((end-start)/width)))
}

func BoundedBuckets(bound float64, count int) []float64 {
	return prometheus.LinearBuckets(bound/float64(count), bound/float64(count), count)
}

func Speed(dx int, dt time.Duration) float64 {
	return float64(dx) / dt.Seconds()
}

func Speed64(dx int64, dt time.Duration) float64 {
	return float64(dx) / dt.Seconds()
}

func SpeedSince(dx int, t time.Time) float64 {
	return Speed(dx, time.Since(t))
}

func MustRegisterGauge(name string) prometheus.Gauge {
	g := prometheus.NewGauge(prometheus.GaugeOpts{Name: name})
	names.add(name)
	prometheus.MustRegister(g)
	return g
}

func MustRegisterCounter(name string) prometheus.Counter {
	c := prometheus.NewCounter(prometheus.CounterOpts{Name: name})
	names.add(name)
	prometheus.MustRegister(c)
	return c
}

func MustRegisterHistogram(name string, sample []float64) prometheus.Histogram {
	h := prometheus.NewHistogram(prometheus.HistogramOpts{
		Name:    name,
		Buckets: sample,
	})
	names.add(name)
	prometheus.MustRegister(h)
	return h
}

func MustRegisterSummary(name string, obj map[float64]float64) prometheus.Histogram {
	h := prometheus.NewSummary(prometheus.SummaryOpts{
		Name:       name,
		Objectives: obj,
	})
	names.add(name)
	prometheus.MustRegister(h)
	return h
}

// sample = seconds (explanation in tests)
func MustRegisterTimer(name string, sample []float64) *timer {
	hist := MustRegisterHistogram(name, sample)
	return &timer{hist: hist}
}

func MustRegisterHistWithHistLabels(name string, sample []float64) *compoundMetric {
	h := MustRegisterHistogram(name, sample)

	vecName := fmt.Sprintf("%s_labels", name)
	hIDs := prometheus.NewHistogramVec(prometheus.HistogramOpts{
		Name:    vecName,
		Buckets: sample,
	}, labels)

	names.add(vecName)
	prometheus.MustRegister(hIDs)

	// HistogramVec is not visible by DefaultGatherer until sub-histogram created so we can't check if it registered
	// creating empty histogram is a viable option to avoid this issue according to godoc
	// https://godoc.org/github.com/prometheus/client_golang/prometheus#HistogramVec.GetMetricWithLabelValues
	_, _ = hIDs.GetMetricWith(ToLabels(metriclabels.Labels{}))
	hist := &compoundMetric{
		Histogram:      h,
		labeled:        &labeledHistogram{HistogramVec: hIDs},
		opTimeout:      defaultOPTimeout,
		gcSleepTime:    defaultGCSleepTime,
		lastOccurrence: map[metriclabels.Labels]time.Time{},
		clock:          clockwork.NewRealClock(),
	}
	go hist.Start()
	return hist
}

func MustRegisterHistWithHistTimer(name string, sample []float64) *labeledTimer {
	return &labeledTimer{hist: MustRegisterHistWithHistLabels(name, sample)}
}

func MustRegisterHistWithGaugeLabels(name string, sample []float64) *compoundMetric {
	h := MustRegisterHistogram(name, sample)

	vecName := fmt.Sprintf("%s_labels", name)
	hIDs := prometheus.NewGaugeVec(prometheus.GaugeOpts{Name: vecName}, labels)

	names.add(vecName)
	prometheus.MustRegister(hIDs)

	// HistogramVec is not visible by DefaultGatherer until sub-histogram created so we can't check if it registered
	// creating empty histogram is a viable option to avoid this issue according to godoc
	// https://godoc.org/github.com/prometheus/client_golang/prometheus#HistogramVec.GetMetricWithLabelValues
	_, _ = hIDs.GetMetricWith(ToLabels(metriclabels.Labels{}))
	hist := &compoundMetric{
		Histogram:      h,
		labeled:        &labeledGauge{GaugeVec: hIDs},
		opTimeout:      defaultOPTimeout,
		gcSleepTime:    defaultGCSleepTime,
		lastOccurrence: map[metriclabels.Labels]time.Time{},
		clock:          clockwork.NewRealClock(),
	}
	go hist.Start()
	return hist
}

func MustRegisterHistWithGaugeTimer(name string, sample []float64) *labeledTimer {
	return &labeledTimer{hist: MustRegisterHistWithGaugeLabels(name, sample)}
}

func MustRegisterHistWithSummaryLabels(name string, buckets []float64, obj map[float64]float64) *compoundMetric {
	h := MustRegisterHistogram(name, buckets)

	vecName := fmt.Sprintf("%s_labels", name)
	hIDs := prometheus.NewSummaryVec(prometheus.SummaryOpts{
		Name:       vecName,
		Objectives: obj,
	}, labels)

	names.add(vecName)
	prometheus.MustRegister(hIDs)

	// HistogramVec is not visible by DefaultGatherer until sub-histogram created so we can't check if it registered
	// creating empty histogram is a viable option to avoid this issue according to godoc
	// https://godoc.org/github.com/prometheus/client_golang/prometheus#HistogramVec.GetMetricWithLabelValues
	_, _ = hIDs.GetMetricWith(ToLabels(metriclabels.Labels{}))
	hist := &compoundMetric{
		Histogram:      h,
		labeled:        &labeledSummary{SummaryVec: hIDs},
		opTimeout:      defaultOPTimeout,
		gcSleepTime:    defaultGCSleepTime,
		lastOccurrence: map[metriclabels.Labels]time.Time{},
		clock:          clockwork.NewRealClock(),
	}
	go hist.Start()
	return hist
}

func MustRegisterHistWithSummaryLabelsTimer(name string, buckets []float64, obj map[float64]float64) *labeledTimer {
	return &labeledTimer{hist: MustRegisterHistWithSummaryLabels(name, buckets, obj)}
}

// MetricsInterceptor adds custom metadata values to logging context and invokes a request.
func MetricsInterceptor() grpc.UnaryServerInterceptor {
	return func(ctx context.Context, req interface{}, info *grpc.UnaryServerInfo, handler grpc.UnaryHandler) (interface{}, error) {

		start := time.Now()
		log.G(ctx).Debug("Starting GRPC handle of request...", zap.Any("request", req))
		resp, err := handler(ctx, req)
		log.G(ctx).Debug("Done GRPC request, it took", zap.Any("duration", time.Since(start)), zap.Any("request", req))

		switch status.Code(err) {
		case codes.AlreadyExists, codes.FailedPrecondition, codes.InvalidArgument, codes.NotFound:
			GRPCRunInvalidRequest.Inc()
		case codes.Internal, codes.Canceled, codes.DeadlineExceeded:
			GRPCRunInternalError.Inc()
		case codes.OK:
			GRPCRunNoError.Inc()
		}
		return resp, err
	}
}
