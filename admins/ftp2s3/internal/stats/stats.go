package stats

import (
	"flag"
	"net/http"
	"strings"
	"time"

	liblog "a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/nop"
	"a.yandex-team.ru/library/go/core/metrics"
	"a.yandex-team.ru/library/go/core/metrics/solomon"
	"a.yandex-team.ru/library/go/yandex/solomon/reporters/puller/httppuller"
)

var (
	log             liblog.Logger = &nop.Logger{}
	statsEndpoint   string
	statsTags       string
	durationBuckets metrics.DurationBuckets
)

func init() {
	flag.StringVar(&statsEndpoint, "stats-endpoint", ":8080", "Solomon metrics exporter endpoint")
	flag.StringVar(&statsTags, "stats-tags", "", "key=value comma separated tags")
	durationBuckets = metrics.MakeExponentialDurationBuckets(time.Millisecond, 1.85, 15)
}

func NewStatsRegistry(l liblog.Logger) *solomon.Registry {
	log = l
	tags := map[string]string{}
	if statsTags != "" {
		log.Infof("stats: Add extra tags %q", statsTags)
		extraTags := strings.Split(statsTags, ",")
		for _, tag := range extraTags {
			kv := strings.Split(tag, "=")
			if len(kv) != 2 {
				log.Fatalf("stats: Malformed tags %s", statsTags)
			}
			tags[kv[0]] = kv[1]
		}
	}
	registry := solomon.NewRegistry(
		solomon.NewRegistryOpts().SetRated(true).SetTags(tags),
	)
	return registry
}

func ServeStats(registry *solomon.Registry) {
	http.Handle("/solomon", httppuller.NewHandler(registry, httppuller.WithSpack()))
	log.Infof("stats: Solomon metrics exporter listen on: %s", statsEndpoint)
	err := http.ListenAndServe(statsEndpoint, nil)
	if err != nil {
		log.Fatalf("%v", err)
	}
}

type authStats struct {
	Success metrics.Counter
	Failed  metrics.Counter
	Unknown metrics.Counter
}

func GetAuthStats(r metrics.Registry) *authStats {
	return &authStats{
		Success: r.Counter("auth.success"),
		Failed:  r.Counter("auth.failed"),
		Unknown: r.Counter("auth.unknown"),
	}
}

type SFTPIOStats struct {
	Open        metrics.Counter
	OutOfOrder  metrics.Counter
	Enqueue     metrics.Counter
	EnqueueHist metrics.Timer
	Close       metrics.Counter
}

func GetSFTPIOStats(r metrics.Registry) *SFTPIOStats {
	return &SFTPIOStats{
		Open:        r.Counter("open"),
		Enqueue:     r.Counter("enqueue"),
		OutOfOrder:  r.Counter("outOfOrder"),
		EnqueueHist: r.DurationHistogram("enqueue.time", durationBuckets),
		Close:       r.Counter("close"),
	}
}

type s3Stats struct {
	Open  metrics.Counter
	Close metrics.Counter
	Seek  metrics.Counter
	Read  metrics.Counter
	Write metrics.Counter
}

func getS3Stats(r metrics.Registry) s3Stats {
	return s3Stats{
		Open:  r.Counter("open"),
		Close: r.Counter("close"),
		Seek:  r.Counter("seek"),
		Read:  r.Counter("read"),
		Write: r.Counter("write"),
	}
}

type S3WriteStats struct {
	s3Stats

	WriteHist      metrics.Timer
	UploadPart     metrics.Counter
	UploadHist     metrics.Timer
	AbortUpload    metrics.Counter
	CompleteUpload metrics.Counter
	ResumeUpload   metrics.Counter
}

func GetS3WriteStats(r metrics.Registry) *S3WriteStats {
	return &S3WriteStats{
		s3Stats: getS3Stats(r),

		WriteHist:      r.DurationHistogram("write.time", durationBuckets),
		UploadPart:     r.Counter("uploadPart"),
		UploadHist:     r.DurationHistogram("uploadPart.time", durationBuckets),
		AbortUpload:    r.Counter("abortUpload"),
		CompleteUpload: r.Counter("completeUpload"),
		ResumeUpload:   r.Counter("resumeUpload"),
	}
}

func (r *S3WriteStats) WriteDuration(start time.Time) {
	r.WriteHist.RecordDuration(time.Since(start))
}
func (r *S3WriteStats) UploadDuration(start time.Time) {
	r.UploadHist.RecordDuration(time.Since(start))
}

type S3ReadStats struct {
	s3Stats

	ReadHist metrics.Timer
}

func GetS3ReadStats(r metrics.Registry) *S3ReadStats {
	return &S3ReadStats{
		s3Stats:  getS3Stats(r),
		ReadHist: r.DurationHistogram("read.time", durationBuckets),
	}
}

func (r *S3ReadStats) ReadDuration(start time.Time) {
	r.ReadHist.RecordDuration(time.Since(start))
}
