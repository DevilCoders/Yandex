package config

import (
	"time"

	"a.yandex-team.ru/cloud/mdb/dataproc-agent/pkg/s3logger"
)

// YarnConfig defines yarn-related configuration of job manager
type YarnConfig struct {
	ServerAddress           string
	RequestTimeout          time.Duration `json:"request_timeout" yaml:"request_timeout"`
	CheckJobStatusInterval  time.Duration `json:"check_job_status_interval" yaml:"check_job_status_interval"`
	RepeatOnFailureInterval time.Duration `json:"repeat_on_failure_interval" yaml:"repeat_on_failure_interval"`
}

// DataprocManagerConfig defines dataproc-manager-related configuration of job manager
type DataprocManagerConfig struct {
	RequestTimeout             time.Duration `json:"request_timeout" yaml:"request_timeout"`
	GetNewJobsInterval         time.Duration `json:"get_new_jobs_interval" yaml:"get_new_jobs_interval"`
	RetryFailedRequestInterval time.Duration `json:"retry_failed_request_interval" yaml:"retry_failed_request_interval"`
}

// Config holds configuration of job manager
type Config struct {
	// feature flag to enable job manager component within agent application
	Enabled                      bool                  `json:"enabled" yaml:"enabled"`
	MaxConcurrentJobs            int                   `json:"max_concurrent_jobs" yaml:"max_concurrent_jobs"`
	MinFreeMemoryToEnqueueNewJob uint64                `json:"min_free_memory_to_enqueue_new_job" yaml:"min_free_memory_to_enqueue_new_job"`
	Yarn                         YarnConfig            `json:"yarn" yaml:"yarn"`
	DataprocManager              DataprocManagerConfig `json:"dataproc_manager" yaml:"dataproc_manager"`
	S3Logger                     s3logger.Config       `json:"s3logger" yaml:"s3logger"`
	// some parts of job output (stdout/stderr writes from main function) is not part of any containers
	// and is not being saved anywhere in YARN
	// that is why job output must be saved to FS to let logging agent to send it.
	// a cron job is needed to clean old files in this directory
	JobOutputDirectory string `json:"job_output_directory" yaml:"job_output_directory"`
	Cid                string
}

// DefaultConfig provides sensible defaults for job manager config
func DefaultConfig() Config {
	return Config{
		MaxConcurrentJobs:            6,
		MinFreeMemoryToEnqueueNewJob: 1073741824,
		Yarn: YarnConfig{
			RequestTimeout:          2 * time.Second,
			CheckJobStatusInterval:  1 * time.Second,
			RepeatOnFailureInterval: 30 * time.Second,
		},
		DataprocManager: DataprocManagerConfig{
			RequestTimeout:             5 * time.Second,
			GetNewJobsInterval:         30 * time.Second,
			RetryFailedRequestInterval: 30 * time.Second,
		},
		S3Logger: s3logger.DefaultConfig(),
	}
}
