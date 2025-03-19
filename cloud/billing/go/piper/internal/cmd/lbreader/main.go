package main

import (
	"context"
	"crypto/tls"
	"fmt"
	"os"
	"os/signal"
	"strings"
	"syscall"
	"time"

	"github.com/spf13/cobra"

	"a.yandex-team.ru/kikimr/public/sdk/go/persqueue"
	"a.yandex-team.ru/kikimr/public/sdk/go/persqueue/log/corelogadapter"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

var (
	readCmd = &cobra.Command{
		Use: "read",
		Run: read,
	}

	mainCtx, globalStop            = context.WithCancel(context.Background())
	logger              log.Logger = func() log.Logger {
		cfg := zap.StandardConfig("cli", log.DebugLevel)
		cfg.OutputPaths = []string{"./log.txt"}
		return zap.Must(cfg)
	}()

	lbInst   string
	topic    string
	consumer string
	keepAll  bool

	startTime = time.Now()
)

func init() {
	readCmd.PersistentFlags().StringVarP(&lbInst, "logbroker", "l", "", "Logbroker installation")
	readCmd.PersistentFlags().StringVarP(&topic, "topic", "t", "", "Topic")
	readCmd.PersistentFlags().StringVarP(&consumer, "consumer", "c", "", "Consumer")
	readCmd.PersistentFlags().BoolVarP(&keepAll, "keep-all", "k", false, "Keep all records")

	_ = readCmd.MarkPersistentFlagRequired("logbroker")
	_ = readCmd.MarkPersistentFlagRequired("topic")
	_ = readCmd.MarkPersistentFlagRequired("consumer")

	sigChan := make(chan os.Signal, 1)
	signal.Notify(sigChan, syscall.SIGINT, syscall.SIGTERM)
	go func() {
		select {
		case <-sigChan:
			globalStop()
		case <-mainCtx.Done():
		}
	}()
}

func main() {
	if err := readCmd.Execute(); err != nil {
		os.Exit(1)
	}
}

func read(cmd *cobra.Command, args []string) {
	instCfg, ok := lbInstallations[lbInst]
	if !ok {
		cmd.PrintErrf("Unknown logbroker installation: %s\n", lbInst)
		return
	}

	if token == "" {
		cmd.PrintErr("Auth token not found in $IAM_TOKEN\n")
		return
	}
	tlsCfg := &tls.Config{MinVersion: tls.VersionTLS12}
	if instCfg.DisableTLS {
		tlsCfg = nil
	}

	opts := persqueue.ReaderOptions{
		Credentials:               tokenCreds(token),
		Database:                  instCfg.DB,
		TLSConfig:                 tlsCfg,
		Endpoint:                  instCfg.Host,
		Port:                      instCfg.Port,
		Consumer:                  consumer,
		Topics:                    []persqueue.TopicInfo{{Topic: topic}},
		Logger:                    corelogadapter.New(logger),
		ManualPartitionAssignment: true,
	}

	// reader := persqueue.NewReaderV1(opts)
	reader := persqueue.NewReader(opts)

	if _, err := reader.Start(mainCtx); err != nil {
		cmd.PrintErrf("reader error: %s\n", err.Error())
		return
	}

	for e := range reader.C() {
		switch m := e.(type) {
		case persqueue.DataMessage:
			m.Commit()
			for _, b := range m.Batches() {
				for _, msg := range b.Messages {
					if keepAll || msg.WriteTime.After(startTime) {
						writeMsg(fmt.Sprintf("%s:%d (%d)", b.Topic, b.Partition, msg.Offset), string(msg.Data))
					}
				}
			}
		case *persqueue.Lock:
			m.StartRead(true, m.ReadOffset, m.ReadOffset)
		case *persqueue.LockV1:
			m.StartRead(true, m.ReadOffset, m.ReadOffset)
		case *persqueue.ReleaseV1:
			m.Release()
		default:
			logger.Debugf("EVENT: %#v", e)
		}
	}
}

func writeMsg(prefix string, msgs string) {
	if msgs == "" {
		fmt.Println(prefix)
		return
	}
	for _, msg := range strings.Split(msgs, "\n") {
		if msg != "" {
			fmt.Printf("%s \t%s\n", prefix, msg)
		}
	}
}
