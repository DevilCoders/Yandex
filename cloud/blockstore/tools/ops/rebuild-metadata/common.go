package main

import (
	"bufio"
	"context"
	"fmt"
	stdlog "log"
	"os"
	"sync"
	"time"

	private_protos "a.yandex-team.ru/cloud/blockstore/private/api/protos"
	nbs "a.yandex-team.ru/cloud/blockstore/public/sdk/go/client"
)

type ctxKey int

const (
	logKey ctxKey = iota
	nbsClientKey
)

////////////////////////////////////////////////////////////////////////////////

type options struct {
	verbose bool
	noAuth  bool

	tokenFile string

	parallelism int
	batchSize   uint32

	host       string
	port       int
	securePort int

	completionLogFile string
	inputFile         string

	numWorkers int

	rebuildType    private_protos.ERebuildMetadataType
	completedDisks completedDisks
	token          string
}

////////////////////////////////////////////////////////////////////////////////

type rebuildStats struct {
	diskID            string
	processed         uint64
	total             uint64
	operationDuration time.Duration
	extra             string
}

////////////////////////////////////////////////////////////////////////////////

type opResponse struct {
	stats  *rebuildStats
	result error
}

////////////////////////////////////////////////////////////////////////////////

var logLevelStrings = map[nbs.LogLevel]string{
	nbs.LOG_ERROR: "[ERROR] ",
	nbs.LOG_WARN:  "[WARN] ",
	nbs.LOG_INFO:  "[INFO] ",
	nbs.LOG_DEBUG: "[DEBUG] ",
}

////////////////////////////////////////////////////////////////////////////////

type rebuildMetadataLogger struct {
	stdLogger *stdlog.Logger
	prefix    string
}

func (l rebuildMetadataLogger) Print(ctx context.Context, v ...interface{}) {
	l.stdLogger.Print(l.prefix)
	l.stdLogger.Print(v...)
}

func (l rebuildMetadataLogger) Printf(
	ctx context.Context,
	format string,
	v ...interface{}) {

	l.stdLogger.Printf(l.prefix+format, v...)
}

////////////////////////////////////////////////////////////////////////////////

type rebuildMetadataLog struct {
	stdLogger    *stdlog.Logger
	logLevel     nbs.LogLevel
	loggers      map[nbs.LogLevel]nbs.Logger
	loggersMutex sync.Mutex
}

func (l *rebuildMetadataLog) Logger(level nbs.LogLevel) nbs.Logger {
	if level > l.logLevel {
		return nil
	}
	logger, ok := l.loggers[level]
	if ok {
		return logger
	}
	logger = &rebuildMetadataLogger{
		stdLogger: l.stdLogger,
		prefix:    logLevelStrings[level],
	}

	l.loggersMutex.Lock()
	l.loggers[level] = logger
	l.loggersMutex.Unlock()
	return logger
}

func (l *rebuildMetadataLog) logError(
	ctx context.Context,
	format string,
	v ...interface{}) {

	if logger := l.Logger(nbs.LOG_ERROR); logger != nil {
		logger.Printf(ctx, format, v...)
	}
}

func (l *rebuildMetadataLog) logWarn(
	ctx context.Context,
	format string,
	v ...interface{}) {

	if logger := l.Logger(nbs.LOG_WARN); logger != nil {
		logger.Printf(ctx, format, v...)
	}
}

func (l *rebuildMetadataLog) logInfo(
	ctx context.Context,
	format string,
	v ...interface{}) {

	if logger := l.Logger(nbs.LOG_INFO); logger != nil {
		logger.Printf(ctx, format, v...)
	}
}

func (l *rebuildMetadataLog) logDebug(
	ctx context.Context,
	format string,
	v ...interface{}) {

	if logger := l.Logger(nbs.LOG_DEBUG); logger != nil {
		logger.Printf(ctx, format, v...)
	}
}

func createRebuildMetadataLog(logLevel nbs.LogLevel) *rebuildMetadataLog {
	return &rebuildMetadataLog{
		stdLogger: stdlog.New(
			os.Stdout,
			"",
			stdlog.Ltime,
		),
		logLevel: logLevel,
		loggers:  make(map[nbs.LogLevel]nbs.Logger),
	}
}

////////////////////////////////////////////////////////////////////////////////

func withNbsClient(ctx context.Context, client nbs.Client) context.Context {
	return context.WithValue(ctx, nbsClientKey, client)
}

func withLog(ctx context.Context, log *rebuildMetadataLog) context.Context {
	return context.WithValue(ctx, logKey, log)
}

func getClient(ctx context.Context) nbs.Client {
	return ctx.Value(nbsClientKey).(nbs.Client)
}

func getLog(ctx context.Context) *rebuildMetadataLog {
	return ctx.Value(logKey).(*rebuildMetadataLog)
}

////////////////////////////////////////////////////////////////////////////////

func readFile(filename string) ([]string, error) {
	file, err := os.Open(filename)
	if err != nil {
		return nil, err
	}
	defer file.Close()

	result := make([]string, 0)

	scanner := bufio.NewScanner(file)

	for scanner.Scan() {
		line := scanner.Text()

		result = append(result, line)
	}

	return result, scanner.Err()
}

////////////////////////////////////////////////////////////////////////////////

func readToken(opts *options) (string, error) {
	if opts.noAuth {
		return "", nil
	}
	data, err := readFile(opts.tokenFile)
	if err != nil {
		return "", fmt.Errorf("unable to read token file: %s", opts.tokenFile)
	}
	if len(data) != 1 {
		return "", fmt.Errorf("token file is empty or invalid: %s", opts.tokenFile)
	}
	return data[0], nil
}

////////////////////////////////////////////////////////////////////////////////

func setupGrpcOptions(ctx context.Context, opts *options) (*nbs.GrpcClientOpts, error) {
	var grpcOpts = nbs.GrpcClientOpts{}

	if !opts.noAuth {
		grpcOpts.Endpoint = fmt.Sprintf("%v:%v", opts.host, opts.securePort)

		log := getLog(ctx)

		log.logDebug(ctx, "connecting to %v with token '%v'", grpcOpts.Endpoint, opts.token)

		grpcOpts.Credentials = &nbs.ClientCredentials{
			AuthToken: opts.token,
		}
	} else {
		grpcOpts.Endpoint = fmt.Sprintf("%v:%v", opts.host, opts.port)
	}

	return &grpcOpts, nil
}

func createNbsClient(ctx context.Context, opts *options) (*nbs.Client, error) {
	grpcOpts, err := setupGrpcOptions(ctx, opts)
	if err != nil {
		return nil, err
	}
	return nbs.NewClient(grpcOpts, &nbs.DurableClientOpts{}, getLog(ctx))
}
