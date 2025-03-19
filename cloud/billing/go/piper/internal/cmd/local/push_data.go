package main

import (
	"bufio"
	"os"
	"time"

	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling/logging"
	lbks "a.yandex-team.ru/cloud/billing/go/pkg/logbroker"
	"a.yandex-team.ru/cloud/billing/go/pkg/logbroker/lbtypes"
	"a.yandex-team.ru/kikimr/public/sdk/go/persqueue"
	"a.yandex-team.ru/kikimr/public/sdk/go/persqueue/log/corelogadapter"
)

var (
	pushInstallation string
	pushTopic        string
)

func init() {
	cmd.AddCommand(pushCmd)
	pushCmd.PersistentFlags().StringVarP(&pushInstallation, "installation", "i", "local", "logbroker installation")
	pushCmd.PersistentFlags().StringVarP(&pushTopic, "topic", "t", "", "logbroker topic")
}

const srcID lbtypes.SourceID = "local-test"

func pushData(cmd *cobra.Command, args []string) {
	if len(args) == 0 {
		cmd.Println("no files for push")
		return
	}

	cont := getContainer()

	runCtx, stop := cont.GetRunContext()
	defer stop()

	instConfig, err := cont.GetLbInstallationConfig(pushInstallation)
	if err != nil {
		cmd.PrintErrln(err)
		return
	}

	wo := persqueue.WriterOptions{
		Database: instConfig.Database,
		Endpoint: instConfig.Host,
		Port:     instConfig.Port,
		Topic:    pushTopic,
		Logger:   corelogadapter.New(logging.Logger().Logger()),
		Codec:    persqueue.Gzip,
	}

	wrtr, err := lbks.NewShardProducer(wo, "test-route", 1, 1, 1000)
	if err != nil {
		cmd.PrintErrln(err)
		return
	}

	off := uint64(time.Now().Unix()) * 1000000
	cmd.Println("start offset", off)

	messages := []lbtypes.ShardMessage{}
	for _, fn := range args {
		f, err := os.Open(fn)
		if err != nil {
			cmd.PrintErrln(err)
			return
		}
		s := bufio.NewScanner(f)
		for s.Scan() {
			off++
			messages = append(messages, stringMessage{data: s.Text(), offset: off})
		}
	}

	woff, err := wrtr.Write(runCtx, "local-data", 0, messages)
	if err != nil {
		cmd.PrintErrln(err)
		return
	}
	cmd.Printf("write completed\n messages %d \n offset %d\n", len(messages), woff)
}

type stringMessage struct {
	data   string
	offset uint64
}

func (s stringMessage) Data() ([]byte, error) { return []byte(s.data), nil }
func (s stringMessage) Offset() uint64        { return s.offset }
