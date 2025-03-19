package main

import (
	"context"
	"log"

	"github.com/spf13/cobra"
	"golang.org/x/sync/errgroup"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/cmd/local/mockgrpc"
)

func run(cmd *cobra.Command, args []string) {
	cont := getContainer()

	runCtx, stop := cont.GetRunContext()
	defer stop()

	iamConfig, err := cont.GetRMConfig()
	if err != nil {
		cmd.PrintErrln(err)
		return
	}
	if iamConfig.Endpoint != "" {
		go runMock(runCtx, iamConfig.Endpoint)
	}

	stats, err := cont.GetHTTPStatusServer()
	if err != nil {
		cmd.PrintErrln(err)
		return
	}
	dumpers, err := cont.GetDumperServices()
	if err != nil {
		cmd.PrintErrln(err)
		return
	}
	resharders, err := cont.GetResharderServices()
	if err != nil {
		cmd.PrintErrln(err)
		return
	}

	wg, mainCtx := errgroup.WithContext(runCtx)
	wg.Go(stats.ListenAndServe)
	for _, d := range dumpers {
		wg.Go(d.Run)
	}
	for _, r := range resharders {
		wg.Go(r.Run)
	}

	<-mainCtx.Done()
	shutdownContainer(cont)
	if err := wg.Wait(); err != nil {
		cmd.PrintErrln(err)
	}
}

func runMock(ctx context.Context, endpoint string) {
	if err := mockgrpc.RunMock(ctx, endpoint); err != nil {
		log.Printf("shutdown error: %s", err.Error())
	}
}
