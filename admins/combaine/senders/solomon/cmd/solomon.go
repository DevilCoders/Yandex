package main

import (
	"flag"
	"fmt"
	"os"

	"a.yandex-team.ru/admins/combaine/senders"
	"a.yandex-team.ru/admins/combaine/senders/solomon"
	"a.yandex-team.ru/library/go/core/buildinfo"
	"google.golang.org/grpc"
	"google.golang.org/grpc/grpclog"
)

const (
	defaultConfigPath = "/etc/combaine/solomon.yaml"
)

var (
	endpoint   string
	logoutput  string
	configPath string
	tracing    bool
	version    bool
	loglevel   string
)

func init() {
	flag.StringVar(&endpoint, "endpoint", ":10052", "endpoint")
	flag.StringVar(&logoutput, "logoutput", "/dev/stderr", "path to logfile")
	flag.StringVar(&configPath, "config", defaultConfigPath, "path to config")
	flag.BoolVar(&tracing, "trace", false, "enable tracing")
	flag.BoolVar(&version, "version", false, "show version")
	flag.StringVar(&loglevel, "loglevel", "info", "debug|info|warn|warning|error|panic in any case")
	flag.Parse()
	grpc.EnableTracing = tracing

	senders.InitializeLogger(loglevel, logoutput)
	grpclog.SetLoggerV2(senders.NewGRPCLoggerV2WithVerbosity(0))
}

func main() {
	if version {
		fmt.Println(buildinfo.Info.ProgramVersion)
		os.Exit(0)
	}
	ss := solomon.NewGRPCService(configPath)
	senders.Serve(endpoint, ss)
}
