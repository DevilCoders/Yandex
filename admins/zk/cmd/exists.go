package main

import (
	"fmt"

	gozk "github.com/go-zookeeper/zk"
	"github.com/spf13/cobra"

	"a.yandex-team.ru/admins/zk"
)

func cmdExists(client zk.Client) *cobra.Command {
	cmd := &cobra.Command{
		Use:   "exists path [--watch]",
		Short: "show if node exists",
		Long: `
Exists checks for a node at the given path and writes "y" or "n" to
stdout according to its presence. If --watch is used, waits for a
change in the presence of the node before exiting.

Example:

    $ zk exists /path
    y`,
		Args: cobra.ExactArgs(1),
		Run: func(cmd *cobra.Command, args []string) {
			ctx := cmd.Context()
			path := fixPath(args[0])
			var events <-chan gozk.Event
			var present bool
			var err error
			if !flagWatch {
				present, _, err = client.Exists(ctx, path)

			} else {
				present, _, events, err = client.ExistsW(ctx, path)
			}
			must(err)
			if present {
				fmt.Printf("y\n")
			} else {
				fmt.Printf("n\n")
			}
			if events != nil {
				evt := <-events
				must(evt.Err)
			}
		},
	}
	cmd.Flags().BoolVarP(&flagWatch, "watch", "w", false, "watch for a change to node presence before returning")
	return cmd
}
