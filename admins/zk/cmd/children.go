package main

import (
	"fmt"
	"sort"

	"a.yandex-team.ru/admins/zk"
	"github.com/spf13/cobra"
)

func cmdChildren(client zk.Client) *cobra.Command {
	cmd := &cobra.Command{
		Use:     "ls [path] [--watch] [-r]",
		Aliases: []string{"children"},
		Short:   "list node children",
		Long: `
Lists the names of the children of the node at the given
path, one name per line. If --watch is used, it waits for a change
in the names of given node's children before returning.

Example:

    $ zk ls /people
    alice
    bob
    fred`,
		Args: cobra.RangeArgs(0, 1),
		Run: func(cmd *cobra.Command, args []string) {
			ctx := cmd.Context()
			path := ""
			if len(args) > 0 {
				path = args[0]
			}
			path = fixPath(path)

			printCallback := func(p string) bool { fmt.Println(p); return false }

			if !flagWatch {
				if flagRecursive {
					processChildrenRecursivelyPreorder(ctx, client, path, printCallback)
					return
				}
				children, _, err := client.Children(ctx, path)
				must(err)
				sort.Strings(children)
				for _, child := range children {
					fmt.Printf("%s\n", child)
				}
			} else {
				children, _, events, err := client.ChildrenW(ctx, path)
				must(err)
				sort.Strings(children)
				for _, child := range children {
					fmt.Printf("%s\n", child)
				}
				evt := <-events
				must(evt.Err)
			}
		},
	}
	cmd.Flags().BoolVarP(&flagWatch, "watch", "w", false, "watch for a change to node children names before returning")
	cmd.Flags().BoolVarP(&flagRecursive, "recursive", "r", false, "recursively list subdirectories encountered")
	return cmd
}
