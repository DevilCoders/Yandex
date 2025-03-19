package cli

import (
	"context"
	"fmt"

	"github.com/fatih/color"
	"github.com/gofrs/uuid"

	"a.yandex-team.ru/cloud/mdb/deploy/agent/internal/pkg/cli/client"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

type Config = client.Config

var DefaultConfig = client.DefaultConfig

func RunCommand(cfg Config, cmd string, args []string, version string, quiet bool) error {
	ctx := context.Background()

	l, err := zap.New(zap.KVConfig(log.WarnLevel))
	if err != nil {
		return fmt.Errorf("create logger: %w", err)
	}
	c, err := client.New(ctx, cfg, l)
	if err != nil {
		return fmt.Errorf("init client: %w", err)
	}

	id, err := uuid.NewV1()
	if err != nil {
		return fmt.Errorf("generate cmd id: %w", err)
	}

	res, err := c.Run(ctx, client.CommandRequest{
		ID:       id.String(),
		Cmd:      cmd,
		Args:     args,
		Version:  version,
		Progress: !quiet,
	})
	if err != nil {
		return fmt.Errorf("run command: %w", err)
	}

	printDebug := color.New(color.FgWhite)
	printDebug.Add(color.Faint)
	printError := color.New(color.FgHiRed)
	for rsp := range res {
		if rsp.Progress != "" {
			_, _ = printDebug.Println("*", rsp.Progress)
		}
		if rsp.Error != "" {
			_, _ = printError.Println("Command execution error:", rsp.Error)
		}
		if rsp.Stderr != "" {
			fmt.Println("Captured stderr")
			fmt.Println(rsp.Stderr)
		}
		if rsp.Stdout != "" {
			if rsp.Error != "" || rsp.Stderr != "" {
				fmt.Println("Captured stdout")
			}
			fmt.Println(rsp.Stdout)
		}
	}

	return nil
}
