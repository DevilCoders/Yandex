package main

import (
	"fmt"
	"os"

	"a.yandex-team.ru/admins/zk"
	"github.com/mattn/go-isatty"
	"github.com/spf13/cobra"
)

func cmdGet(client zk.Client) *cobra.Command {
	cmd := &cobra.Command{
		Use:   "get path [--watch]",
		Short: "show node data",
		Long: `
Get reads the node data at the given path and writes it to stdout.
If --watch is used, waits for a change to the node before exiting.

Example:

    $ zk get /path
    content`,
		Args: cobra.ExactArgs(1),
		Run: func(cmd *cobra.Command, args []string) {
			ctx := cmd.Context()
			path := fixPath(args[0])
			if !flagWatch {
				data, _, err := client.Get(ctx, path)
				must(err)
				_, _ = os.Stdout.Write(data)
				if isatty.IsTerminal(os.Stdout.Fd()) {
					dataLen := len(data)
					if dataLen > 0 && data[dataLen-1] != '\n' {
						fmt.Println()
					}
				}
			} else {
				data, _, events, err := client.GetW(ctx, path)
				must(err)
				_, _ = os.Stdout.Write(data)
				evt := <-events
				must(evt.Err)
			}
		},
	}
	cmd.Flags().BoolVarP(&flagWatch, "watch", "w", false, "watch for a change to node state before returning")
	return cmd
}
