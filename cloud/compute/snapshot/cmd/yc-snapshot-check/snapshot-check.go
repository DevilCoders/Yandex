package main

import (
	"bufio"
	"flag"

	"go.uber.org/zap/zapcore"

	log "a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"
	"a.yandex-team.ru/cloud/compute/snapshot/pkg/version"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/config"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/misc"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/logging"

	"context"
	"fmt"
	"os"

	"go.uber.org/zap"
)

var fmtVersion = fmt.Sprintf("version=%s build=%s hash=%s tag=%s", version.Version, version.Build, version.GitHash, version.GitTag)

func main() {
	flag.Usage = usage
	flag.Parse()

	if *showVersionParam {
		fmt.Println(fmtVersion)
		return
	}

	logger, err := zap.NewDevelopment()
	if err != nil {
		_, _ = os.Stderr.WriteString("Can't create logger")
		os.Exit(1)
	}
	ctx := log.WithLogger(context.Background(), logger)

	cfg, err := config.GetConfig()
	if err != nil {
		log.G(ctx).Fatal("Can't read config", zap.Error(err))
	}

	facade, err := lib.NewFacadeOps(ctx, &cfg, misc.TableOpDBOnly)
	if err != nil {
		log.G(ctx).Fatal("Can't create facage", zap.Error(err))
	}

	idsChannel := make(chan string)

	go func() {
		defer close(idsChannel)
		if flag.NArg() > 0 {
			for _, id := range flag.Args() {
				idsChannel <- id
			}
		} else {
			scanner := bufio.NewScanner(os.Stdin)
			for scanner.Scan() {
				idsChannel <- scanner.Text()
			}
		}
	}()

	for id := range idsChannel {
		idContext := log.WithLogger(ctx, log.G(ctx).With(logging.SnapshotID(id)))
		if details := checkSnapshot(idContext, facade, id); details != "" {
			fmt.Println(id, "fail", details)
		} else {
			fmt.Println(id, "ok")
		}
	}
}

func checkSnapshot(ctx context.Context, facade *lib.Facade, id string) string {
	ctx = log.WithLogger(ctx, log.G(ctx).WithOptions(zap.AddStacktrace(zapcore.FatalLevel)))
	report, err := facade.VerifyChecksum(ctx, id, true)
	logger := log.G(ctx).WithOptions(zap.AddStacktrace(zapcore.FatalLevel))
	if err != nil {
		logger.Error("Error verify checksum process", zap.Error(err))
		return err.Error()
	}
	if !report.ChecksumMatched {
		logger.Error("checksum mismatch", zap.String("details", report.Details))
		for _, mismatch := range report.Mismatches {
			logger.Error("checksum mismatch", zap.String("chunk_id", mismatch.ChunkID),
				zap.Int64("offset", mismatch.Offset), zap.String("details", mismatch.Details))
		}
		return fmt.Sprintf("details=%s mismatches=%v", report.Details, report.Mismatches)
	}
	logger.Info("checksum ok")
	return ""
}

func usage() {
	fmt.Print("ToDo help")
	flag.CommandLine.SetOutput(os.Stdout)
	flag.CommandLine.PrintDefaults()
}
