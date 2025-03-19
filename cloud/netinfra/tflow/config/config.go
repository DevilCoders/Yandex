package config

import (
	"flag"
	"fmt"
	"io/ioutil"
	"os"
	"os/exec"
	"path"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
	yaml "gopkg.in/yaml.v2"
)

const release string = "tFlow 0.1.0"

type CollectorCfg struct {
	Enabled  bool   `yaml:"enabled"`
	Listen   string `yaml:"listen"`
	UDPSize  int    `yaml:"udp-size"`
	UDPQueue int    `yaml:"udp-queue"`
	Workers  int    `yaml:"workers"`
}

type IpfixCollectorCfg struct {
	Cfg CollectorCfg `yaml:",inline"`
}

type SFlowCollectorCfg struct {
	Cfg   CollectorCfg `yaml:",inline"`
	Encap bool         `yaml:"encap"`
}

type ConsumerCfg struct {
	SFlowLogFile      string `yaml:"sflow"`
	SFlowEncapLogFile string `yaml:"sflow-encap"`
	IpfixNATLogFile   string `yaml:"ipfix-nat"`
	MsgQueue          int    `yaml:"msg-queue"`
}

// Config all options
type Config struct {
	// global options
	Verbose  bool   `yaml:"verbose"`
	LogFile  string `yaml:"log-file"`
	LogLevel string `yaml:"log-level"`
	PIDFile  string `yaml:"pid-file"`

	// stats options
	StatsEnabled  bool   `yaml:"stats-enabled"`
	StatsHTTPAddr string `yaml:"stats-http-addr"`
	StatsHTTPPort string `yaml:"stats-http-port"`

	// sFlow options
	SFlowConfig *SFlowCollectorCfg `yaml:"sflow-collector"`
	// IPFIX options
	IpfixConfig *IpfixCollectorCfg `yaml:"ipfix-collector"`
	// Consumer options
	ConsumerConfig *ConsumerCfg `yaml:"consumer"`

	version    bool
	configPath string
	configFile string
}

// NewOptions Populate options
func NewOptions() *Config {
	return &Config{
		Verbose:    false,
		version:    false,
		PIDFile:    "/var/run/tflow.pid",
		configPath: "/etc/tflow",
		LogLevel:   "warn",

		StatsEnabled: true,
		// TODO: use single option like for SFlowListen?
		StatsHTTPPort: ":8081",
		StatsHTTPAddr: "",

		SFlowConfig: &SFlowCollectorCfg{
			Cfg: CollectorCfg{
				Enabled:  true,
				Listen:   ":6343",
				UDPSize:  1500,
				UDPQueue: 5000, //found by @atsygane in an empiric way
				Workers:  8,
			},
			Encap: false,
		},

		IpfixConfig: &IpfixCollectorCfg{
			Cfg: CollectorCfg{
				Enabled:  true,
				Listen:   ":4739",
				UDPSize:  1500,
				UDPQueue: 5000, //found by @atsygane in an empiric way
				Workers:  8,
			},
		},

		ConsumerConfig: &ConsumerCfg{
			SFlowLogFile:      "/var/log/sflow-tskv.log",
			SFlowEncapLogFile: "/var/log/tflow/encap.log",
			IpfixNATLogFile:   "/var/log/tflow/tskv-nat.log",
			MsgQueue:          5000, //found by @atsygane in an empiric way
		},
	}
}

// GetOptions gets options through cmd and file
func GetOptions() (*Config, log.Logger) {
	cfg := NewOptions()

	cfg.parseOptions()
	cfg.showVersion()

	logger := makeLogger(cfg)

	logger.Debugf("encap: %v\n", cfg.SFlowConfig.Encap)
	logger.Debugf("config file: %v\n", cfg.configFile)

	if cfg.Verbose {
		logger.Infof("the full logging enabled")
	}

	if ok := cfg.isAlreadyRunning(); ok {
		logger.Fatalf("tFlow already running!")
		os.Exit(1)
	}

	cfg.pidWrite(logger)
	cfg.normalizeConsumerConfig()

	logger.Infof("Welcome to %s", release)

	return cfg, logger
}

//helper function to remove mentions of unneeded files from consumer config
func (cfg *Config) normalizeConsumerConfig() {
	if !cfg.SFlowConfig.Cfg.Enabled {
		cfg.ConsumerConfig.SFlowLogFile = ""
	}
	if !cfg.SFlowConfig.Encap {
		cfg.ConsumerConfig.SFlowEncapLogFile = ""
	}
	if !cfg.IpfixConfig.Cfg.Enabled {
		cfg.ConsumerConfig.IpfixNATLogFile = ""
	}
}

func makeLogger(cfg *Config) log.Logger {
	level, err := log.ParseLevel(cfg.LogLevel)
	if err != nil {
		panic(err)
	}
	logCfg := zap.ConsoleConfig(level)
	if cfg.LogFile != "" {
		f, err := os.OpenFile(cfg.LogFile, os.O_RDWR|os.O_CREATE|os.O_APPEND, 0660)
		if err != nil {
			panic(err)
		}
		err = f.Close()
		if err != nil {
			panic(err)
		} else {
			logCfg.OutputPaths = []string{"file://" + cfg.LogFile}
		}
	}

	logger, err := zap.New(logCfg)
	if err != nil {
		panic(err)
	}
	return logger
}

func (cfg Config) showVersion() {
	if cfg.version {
		fmt.Printf("%s\n", release)
		os.Exit(0)
	}
}

func (cfg *Config) parseOptions() {

	// TODO: если для файлов дан относительный путь, переделать его в абсолютный перед использованием
	// может потребоваться для reopenLogs

	var configFile string
	flag.StringVar(&configFile, "config", "/etc/tflow/tflow.conf", "path to config file")

	loadCfg(cfg)

	// global options
	flag.BoolVar(&cfg.Verbose, "verbose", cfg.Verbose, "enable/disable verbose logging")
	flag.BoolVar(&cfg.version, "version", cfg.version, "show version")
	flag.StringVar(&cfg.LogFile, "log-file", cfg.LogFile, "log file name")
	flag.StringVar(&cfg.LogLevel, "log-level", cfg.LogLevel, "[trace|debug|info|warn|error|fatal]")
	flag.StringVar(&cfg.PIDFile, "pid-file", cfg.PIDFile, "pid file name")

	// stats options
	flag.BoolVar(&cfg.StatsEnabled, "stats-enabled", cfg.StatsEnabled, "enable/disable stats listener")
	flag.StringVar(&cfg.StatsHTTPPort, "stats-http-port", cfg.StatsHTTPPort, "stats port listener")
	flag.StringVar(&cfg.StatsHTTPAddr, "stats-http-addr", cfg.StatsHTTPAddr, "stats bind address listener")

	// sflow options
	flag.BoolVar(&cfg.SFlowConfig.Cfg.Enabled, "sflow-enabled", cfg.SFlowConfig.Cfg.Enabled, "enable/disable sflow listener")
	flag.StringVar(&cfg.SFlowConfig.Cfg.Listen, "sflow-listen", cfg.SFlowConfig.Cfg.Listen, "sflow listening on ip and port. Default \":6343\"")
	flag.IntVar(&cfg.SFlowConfig.Cfg.UDPSize, "sflow-max-udp-size", cfg.SFlowConfig.Cfg.UDPSize, "sflow maximum UDP size")
	flag.IntVar(&cfg.SFlowConfig.Cfg.Workers, "sflow-workers", cfg.SFlowConfig.Cfg.Workers, "sflow workers number")
	flag.BoolVar(&cfg.SFlowConfig.Encap, "sflow-encap", cfg.SFlowConfig.Encap, "parsing headers under MPLS or MPLS-over-GRE encapsulation and saving it in separate log")

	// ipfix options
	flag.BoolVar(&cfg.IpfixConfig.Cfg.Enabled, "ipfix-enabled", cfg.IpfixConfig.Cfg.Enabled, "enable/disable sflow listener")
	flag.StringVar(&cfg.IpfixConfig.Cfg.Listen, "ipfix-listen", cfg.IpfixConfig.Cfg.Listen, "sflow listening on ip and port. Default \":6343\"")
	flag.IntVar(&cfg.IpfixConfig.Cfg.UDPSize, "ipfix-max-udp-size", cfg.IpfixConfig.Cfg.UDPSize, "sflow maximum UDP size")
	flag.IntVar(&cfg.IpfixConfig.Cfg.Workers, "ipfix-workers", cfg.IpfixConfig.Cfg.Workers, "sflow workers number")

	// consumer options
	flag.StringVar(&cfg.ConsumerConfig.SFlowLogFile, "sflow-log", cfg.ConsumerConfig.SFlowLogFile, "sflow tskv log file name")
	flag.StringVar(&cfg.ConsumerConfig.SFlowEncapLogFile, "encap-log", cfg.ConsumerConfig.SFlowEncapLogFile, "sflow encap log file name")
	flag.StringVar(&cfg.ConsumerConfig.IpfixNATLogFile, "ipfix-nat-log", cfg.ConsumerConfig.IpfixNATLogFile, "ipfix nat log file name")

	flag.Usage = func() {
		flag.PrintDefaults()
	}

	flag.Parse()
}

func loadCfg(cfg *Config) {
	var file = path.Join(cfg.configPath, "tflow.conf")

	for i, flag := range os.Args {
		if flag == "-config" {
			file = os.Args[i+1]
			cfg.configPath, _ = path.Split(file)
			break
		}
	}

	//if config file exists
	if _, err := os.Stat(file); err == nil {
		b, err := ioutil.ReadFile(file)
		if err != nil {
			panic(err)
		}
		err = yaml.Unmarshal(b, cfg)
		if err != nil {
			panic(err)
		}
	}
	cfg.configFile = file
}

func (cfg Config) isAlreadyRunning() bool {
	b, err := ioutil.ReadFile(cfg.PIDFile)
	if err != nil {
		return false
	}

	cmd := exec.Command("kill", "-0", string(b))
	_, err = cmd.Output()
	return err == nil
}

func (cfg Config) pidWrite(logger log.Logger) {
	f, err := os.OpenFile(cfg.PIDFile, os.O_WRONLY|os.O_CREATE, 0666)
	if err != nil {
		logger.Errorf("%v\n", err)
		return
	}

	_, err = fmt.Fprintf(f, "%d", os.Getpid())
	if err != nil {
		logger.Errorf("%v\n", err)
	}

	defer func() {
		err := f.Close()
		if err != nil {
			logger.Errorf("%v\n", err)
		}
	}()
}
