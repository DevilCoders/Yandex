package sender

import (
	"io/ioutil"
	"time"

	"github.com/c2h5oh/datasize"
	"gopkg.in/yaml.v2"
)

// Arguments for sender binary
type Arguments struct {
	LogTrace bool
	Config   string
}

// GraphiteCfg config for graphite target
type GraphiteCfg struct {
	MaxReconnect  int      `yaml:"max_reconnect"`
	MinConnTimeMs int      `yaml:"min_conn_time_ms"`
	MaxConnTimeMs int      `yaml:"max_conn_time_ms"`
	Servers       []string `yaml:"server"`
}

// SendingCfg config for sender
type SendingCfg struct {
	DelayFactor int `yaml:"delayed_max_metrics_factor"`
	MaxMetrics  int `yaml:"max_metrics"`
	Port        int
	// divider for send interval in cache sending thread
	CacheSpeedup int           `yaml:"cache_speedup"`
	QueueTimeout time.Duration `yaml:"queue_timeout"`
	SendInterval time.Duration `yaml:"send_interval"`
	QueueDir     string        `yaml:"queue_dir"`
	LogFile      string        `yaml:"logfile"`
	LogLevel     string        `yaml:"loglevel"`
}

// StatusCfg config for internal stats
type StatusCfg struct {
	MaxOfflineTime time.Duration     `yaml:"max_offline_time"`
	MaxInvalidRps  float64           `yaml:"max_invalid_rps"`
	MaxDropRps     float64           `yaml:"max_drop_rps"`
	MaxMemoryUsage datasize.ByteSize `yaml:"max_memory_usage"`
	Port           int
}

// Config for sender
type Config struct {
	Graphite GraphiteCfg
	Sender   SendingCfg
	Status   StatusCfg
}

func loadConfig(args *Arguments) (*Config, error) {
	data, err := ioutil.ReadFile(args.Config)
	if err != nil {
		return nil, err
	}
	cfg := defaultConfig()

	err = yaml.Unmarshal(data, cfg)
	if err != nil {
		return nil, err
	}
	return cfg, nil
}

func defaultConfig() *Config {
	return &Config{
		Graphite: GraphiteCfg{
			Servers:       []string{"127.0.0.1:2003"},
			MaxReconnect:  3,
			MinConnTimeMs: 300,
			MaxConnTimeMs: 2000,
		},
		Sender: SendingCfg{
			DelayFactor:  10,
			MaxMetrics:   1000,
			Port:         8081,
			CacheSpeedup: 2,
			QueueTimeout: 3 * time.Hour,
			SendInterval: 1 * time.Minute,
			QueueDir:     "/var/spool/graphite-sender",
			LogFile:      "stdout",
			LogLevel:     "debug",
		},
		Status: StatusCfg{
			MaxOfflineTime: 5 * time.Minute,
			MaxInvalidRps:  0.02,
			MaxDropRps:     0,
			MaxMemoryUsage: 1 * datasize.GB,
			Port:           8082,
		},
	}
}
