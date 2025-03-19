package config

import (
	"errors"
	"flag"
	"fmt"
	"log"
	"strings"
	"sync"
	"time"

	"a.yandex-team.ru/cloud/compute/go-common/tracing"

	"golang.org/x/xerrors"

	"github.com/BurntSushi/toml"
	werrors "github.com/pkg/errors"
	"go.uber.org/zap/zapcore"

	"a.yandex-team.ru/cloud/blockstore/public/sdk/go/client"
	"a.yandex-team.ru/cloud/compute/go-common/database/kikimr"
	"a.yandex-team.ru/cloud/compute/go-common/mon"
	"a.yandex-team.ru/cloud/compute/snapshot/internal/auth"
	"a.yandex-team.ru/cloud/compute/snapshot/internal/dockerprocess"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/globalauth"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/misc"
)

const (
	defaultCacheSize              = 1024
	defaultMaxSelectRows          = 100
	defaultClearChunkBatchSize    = 50
	defaultGCBatchSize            = 100
	defaultConvertWorkers         = 16
	defaultMoveWorkers            = 16
	defaultGCWorkers              = 1
	defaultClearChunkWorkers      = 16
	defaultVerifyWorkers          = 64
	defaultStoreMetadataBatchSize = 50
	defaultKikimrTimeSkewSec      = 5 * 60
	defaultKikimrLockPollSec      = 60
	defaultKikimrUpdateTimeSec    = 60
	defaultSessionCountLimit      = 1000
)

// Config is snapshot service config
type Config struct {
	General         GeneralConfig
	S3              S3Config
	Kikimr          kikimr.Config
	Server          ServerConfig
	Logging         LoggerConfig
	Tracing         TracingConfig
	DebugServer     DebugServerConfig
	GC              GCConfig
	Monitoring      mon.Config
	Nbs             NbsConfig
	Nbd             NbdConfig
	Performance     PerformanceConfig
	QemuDockerProxy ProxyConfig
	AccessService   auth.Config

	TaskLimiter TaskLimiterConfig
}

// Validate child configs
func (c *Config) Validate() error {
	if err := c.General.validate(); err != nil {
		return werrors.WithMessage(err, "General.validate error")
	}

	if err := c.S3.validate(); err != nil {
		return werrors.WithMessage(err, "S3.validate error")
	}

	for confName, kikimrConfig := range c.Kikimr {
		err := kikimrConfig.Validate()
		if err != nil {
			return werrors.WithMessage(err, "Kikimr.validate error (config: '"+confName+"')")
		}
	}

	if err := c.Server.validate(); err != nil {
		return werrors.WithMessage(err, "Server.validate error")
	}

	if err := c.Logging.validate(); err != nil {
		return werrors.WithMessage(err, "Logging.validate error")
	}

	if err := c.Tracing.Validate(); err != nil {
		return werrors.WithMessage(err, "Tracing.validate error")
	}

	if err := c.TaskLimiter.validate(); err != nil {
		return werrors.WithMessage(err, "TaskLimiter.validate error")
	}

	if err := c.DebugServer.validate(); err != nil {
		return werrors.WithMessage(err, "DebugServer.validate error")
	}

	if err := c.GC.validate(); err != nil {
		return werrors.WithMessage(err, "GC.validate error")
	}

	if err := c.Performance.validate(); err != nil {
		return werrors.WithMessage(err, "Performance,validate error")
	}

	if err := c.validateKikimr(); err != nil {
		return werrors.WithMessage(err, "ValidateKikimr error")
	}

	if err := c.validateNbs(); err != nil {
		return werrors.WithMessage(err, "ValidateNbs error")
	}

	if err := c.Nbd.Validate(); err != nil {
		return werrors.WithMessage(err, "ValidateNbd error")
	}

	return nil
}

func (c *Config) Parse() error {
	if err := c.Nbd.Parse(); err != nil {
		return xerrors.Errorf("parse config error: %w", err)
	}
	return nil
}

//nolint:unparam
func (c *Config) validateKikimr() error {
	for name, cfg := range c.Kikimr {
		// ensure that only one driver will be used
		if !strings.HasPrefix(cfg.Root, cfg.DBName) || !strings.HasPrefix(cfg.DBName, "/") {
			return errors.New("path to tables shell be in form '/DBName/long-path' if new driver used")
		}

		if cfg.MaxSelectRows == 0 {
			cfg.MaxSelectRows = defaultMaxSelectRows
		}

		if cfg.LockPollIntervalSec == 0 {
			cfg.LockPollIntervalSec = defaultKikimrLockPollSec
		}

		if cfg.TimeSkewSec == 0 {
			cfg.TimeSkewSec = defaultKikimrTimeSkewSec
		}

		if cfg.UpdateGraceTimeSec == 0 {
			cfg.UpdateGraceTimeSec = defaultKikimrUpdateTimeSec
		}

		if cfg.MaxSessionCount == 0 {
			cfg.MaxSessionCount = defaultSessionCountLimit
		}

		c.Kikimr[name] = cfg
	}
	return nil
}

func (c *Config) validateNbs() error {
	for index := range c.Nbs {
		cfg := c.Nbs[index]
		if cfg.Host == "" && len(cfg.Hosts) == 0 {
			return fmt.Errorf("Nbs.Host or cfg.Hosts must be non-empty")
		}
		if len(cfg.Hosts) == 0 {
			cfg.Hosts = []string{cfg.Host}
		}
		c.Nbs[index] = cfg
	}
	return nil
}

// GeneralConfig contains Misc stuff
type GeneralConfig struct {
	ZoneID                          string
	Tmpdir                          string
	Storage                         string
	CacheSize                       int
	DummyFsRoot                     string
	StartSplit                      int
	ClearChunkBatchSize             int
	TokenPolicy                     string
	ExperimentalIncrementalSnapshot bool
	MetricDetailsFile               string
	ChecksumAlgorithm               string
}

func (c *GeneralConfig) validate() error {
	if c.Tmpdir == "" {
		return fmt.Errorf("General.Tmpdir must be non-empty")
	}
	if c.Storage == "" {
		return fmt.Errorf("General.Storage must be non-empty")
	}
	if c.CacheSize == 0 {
		c.CacheSize = defaultCacheSize
	}
	if c.StartSplit == 0 {
		return fmt.Errorf("General.StartSplit must be non-zero")
	}
	if c.ClearChunkBatchSize == 0 {
		c.ClearChunkBatchSize = defaultClearChunkBatchSize
	}

	if c.TokenPolicy == "" {
		c.TokenPolicy = globalauth.NoneTokenPolicy
	}

	if c.TokenPolicy != globalauth.MetadataTokenPolicy && c.TokenPolicy != globalauth.NoneTokenPolicy {
		return fmt.Errorf(`token policy should be either "%s" or "%s"'`,
			globalauth.MetadataTokenPolicy, globalauth.NoneTokenPolicy)
	}

	if c.ChecksumAlgorithm == "" {
		c.ChecksumAlgorithm = misc.SHA512Hash
	}

	if c.ChecksumAlgorithm != misc.SHA512Hash && c.ChecksumAlgorithm != misc.Blake2Hash && c.ChecksumAlgorithm != misc.CRC32Hash {
		return fmt.Errorf("invalid checksum algorithm %s", c.ChecksumAlgorithm)
	}

	return nil
}

// S3Config is S3 object storage config (image source for conversion)
type S3Config struct {
	Dummy           bool
	EnableRedirects bool
	RegionName      string
	Endpoint        string
	TokenEndpoint   string
	Profile         string
}

func (c *S3Config) validate() error {
	if c.Dummy {
		// We expect HTTP, do not check anything
		return nil
	}

	if c.RegionName == "" {
		return fmt.Errorf("S3.RegionName must be non-empty")
	}
	if c.Endpoint == "" {
		return fmt.Errorf("S3.Endpoint must be non-empty")
	}

	return nil
}

// ServerConfig is snapshot API server config
type ServerConfig struct {
	SSL          bool
	KeyFile      string
	CertFile     string
	HTTPEndpoint *ServeEndpoint
	GRPCEndpoint *ServeEndpoint
}

func (c *ServerConfig) validate() error {
	if c.SSL {
		if c.KeyFile == "" {
			return fmt.Errorf("Server.KeyFile must be non-empty if SSL enabled")
		}
		if c.CertFile == "" {
			return fmt.Errorf("Server.CertFile must be non-empty if SSL enabled")
		}
	}

	if c.GRPCEndpoint == nil {
		return fmt.Errorf("Server.GRPCEndpoint must be specified")
	}

	return nil
}

type TracingConfig struct {
	tracing.Config

	TraceAllCopyWorkers            bool
	LoopRateLimiterInitialInterval duration
	LoopRateLimiterMaxInterval     duration
	TraceOldYDBClient              bool
}

func (tc *TracingConfig) Validate() error {
	const infiniteDuration = time.Hour * 24 * 365 * 10
	if tc.LoopRateLimiterInitialInterval.Duration < 0 {
		tc.LoopRateLimiterInitialInterval.Duration = infiniteDuration
	}
	if tc.LoopRateLimiterInitialInterval.Duration == 0 {
		tc.LoopRateLimiterInitialInterval.Duration = time.Minute
	}
	if tc.LoopRateLimiterMaxInterval.Duration == 0 {
		tc.LoopRateLimiterMaxInterval.Duration = time.Minute * 15
	}
	if tc.LoopRateLimiterMaxInterval.Duration < 0 {
		tc.LoopRateLimiterMaxInterval.Duration = infiniteDuration
	}
	return nil
}

type TaskLimiterConfig struct {
	CheckInterval duration

	ZoneUnknown int

	ZoneTotal     int
	CloudTotal    int
	ProjectTotal  int
	SnapshotTotal int

	ZoneShallowCopy     int
	CloudShallowCopy    int
	ProjectShallowCopy  int
	SnapshotShallowCopy int

	ZoneImport    int
	CloudImport   int
	ProjectImport int

	ZoneSnapshot    int
	CloudSnapshot   int
	ProjectSnapshot int

	ZoneRestore     int
	CloudRestore    int
	ProjectRestore  int
	SnapshotRestore int

	ZoneDelete    int
	CloudDelete   int
	ProjectDelete int
}

func (tlc *TaskLimiterConfig) validate() error {
	if tlc.CheckInterval.Duration == 0 {
		tlc.CheckInterval.Duration = time.Minute
	}

	return nil
}

// DebugServerConfig is debug server config
type DebugServerConfig struct {
	HTTPEndpoint *ServeEndpoint
	GCEndpoint   *ServeEndpoint
}

func (*DebugServerConfig) validate() error {
	// NOTE: no checks now. Probably we will chech that the endpoint is local only
	return nil
}

// LoggerConfig contains logging settings
type LoggerConfig struct {
	Level        zapcore.Level
	EnableKikimr bool
	EnableSpans  bool
	EnableGrpc   bool
	EnableNbs    bool
	NbsLogLevel  string
	Output       string
}

func (*LoggerConfig) validate() error {
	// NOTE: no checks now
	return nil
}

func (c *LoggerConfig) GetNbsLogLevel() client.LogLevel {
	switch strings.TrimSpace(strings.ToLower(c.NbsLogLevel)) {
	case "err", "error":
		return client.LOG_ERROR
	case "warn", "warning":
		return client.LOG_WARN
	case "info":
		return client.LOG_INFO
	case "debug":
		return client.LOG_DEBUG
	default:
		// If don't know level - use debug as default
		return client.LOG_DEBUG
	}
}

type duration struct {
	time.Duration
}

func (d *duration) UnmarshalText(text []byte) error {
	var err error
	d.Duration, err = time.ParseDuration(string(text))
	return err
}

// GCConfig is garbage collection config
type GCConfig struct {
	Enabled          bool
	BatchSize        int
	Interval         duration
	FailedCreation   duration
	FailedConversion duration
	FailedDeletion   duration
	Tombstone        duration
}

func (c *GCConfig) validate() error {
	if !c.Enabled {
		return nil
	}
	if c.BatchSize == 0 {
		c.BatchSize = defaultGCBatchSize
	}
	if c.FailedCreation.Duration == 0 {
		return fmt.Errorf("GC.FailedCreation must be non-empty")
	}
	if c.FailedConversion.Duration == 0 {
		return fmt.Errorf("GC.FailedConversion must be non-empty")
	}
	if c.FailedDeletion.Duration == 0 {
		return fmt.Errorf("GC.FailedDeletion must be non-empty")
	}
	if c.Tombstone.Duration == 0 {
		return fmt.Errorf("GC.Tombstone must be non-empty")
	}
	return nil
}

type ProxyConfig struct {
	SocketPath    string
	HostForDocker string
	IDLengthBytes int
}

// NbsClientConfig is nbs client config
type NbsClientConfig struct {
	Host                  string
	Hosts                 []string
	SupportsChangedBlocks bool
	SSL                   bool
	KeyFile               string
	CertFile              string
	RootCertsFile         string
	DiscoveryHostLimit    uint32
}

// NbsConfig contains multiple nbs enpoint configs
type NbsConfig map[string]NbsClientConfig

type NbdConfig struct {
	dockerprocess.DockerConfig
}

// PerformanceConfig contains performance settings
type PerformanceConfig struct {
	ConvertWorkers                 int
	MoveWorkers                    int
	GCWorkers                      int
	ClearChunkWorkers              int
	VerifyWorkers                  int
	StoreSnapshotMetadataBatchSize int
}

//nolint:unparam
func (c *PerformanceConfig) validate() error {
	if c.ConvertWorkers == 0 {
		c.ConvertWorkers = defaultConvertWorkers
	}
	if c.MoveWorkers == 0 {
		c.MoveWorkers = defaultMoveWorkers
	}
	if c.GCWorkers == 0 {
		c.GCWorkers = defaultGCWorkers
	}
	if c.ClearChunkWorkers == 0 {
		c.ClearChunkWorkers = defaultClearChunkWorkers
	}
	if c.VerifyWorkers == 0 {
		c.VerifyWorkers = defaultVerifyWorkers
	}
	if c.StoreSnapshotMetadataBatchSize == 0 {
		c.StoreSnapshotMetadataBatchSize = defaultStoreMetadataBatchSize
	}
	return nil
}

var (
	confpath = flag.String("config", "config.toml", "path to server config file")

	conf         *Config
	decodeConfig sync.Once
)

// LoadConfig loads config from given file
func LoadConfig(path string) error {
	conf = new(Config)
	_, err := toml.DecodeFile(path, conf)
	if err != nil {
		log.Print(err)
		return err
	}
	err = conf.Parse()
	if err != nil {
		log.Print(err)
		return err
	}

	err = conf.Validate()
	if err != nil {
		log.Print(err)
	}
	return err
}

// GetConfig returns previously loaded config or loads a new one
func GetConfig() (Config, error) {
	if conf != nil {
		return *conf, nil
	}

	var err error
	var intConf Config
	decodeConfig.Do(func() {
		_, err = toml.DecodeFile(*confpath, &intConf)
	})

	if err != nil {
		log.Print(err)
		return Config{}, err
	}

	err = intConf.Parse()
	if err != nil {
		return Config{}, err
	}

	err = intConf.Validate()
	if err != nil {
		return Config{}, err
	}
	conf = &intConf
	return *conf, err
}

// SetConfig sets global config
func SetConfig(cfg Config) {
	*conf = cfg
}
