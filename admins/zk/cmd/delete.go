package main

import (
	"a.yandex-team.ru/admins/zk"
	"github.com/spf13/cobra"
)

func cmdDelete(client zk.Client) *cobra.Command {
	cmd := &cobra.Command{
		Use:     "rm path [-r | -v version]",
		Aliases: []string{"delete"},
		Short:   "delete node",
		Long: `
Delete removes the node at the given path. If a version is given,
submits that version with the delete request for verification
against the current version. Otherwise delete will clobber the
current data regardless of its version.

Examples:

    $ zk delete /path

    $ zk stat /path | grep Version
    Version:        7
    $ zk delete /path -v 7`,
		Args: cobra.ExactArgs(1),
		Run: func(cmd *cobra.Command, args []string) {
			ctx := cmd.Context()
			path := fixPath(args[0])
			deleteCallback := func(p string) {
				err := client.Delete(ctx, p, -1)
				must(err)
			}

			if flagRecursive {
				processChildrenRecursivelyPostorder(ctx, client, path, deleteCallback)
				deleteCallback(path)
				return
			}

			logger.Debugf("Delete node: %s", path)
			err := client.Delete(ctx, path, flagVersion)
			must(err)
		},
	}
	cmd.Flags().Int32VarP(&flagVersion, "version", "v", -1, "Expected znode version")
	cmd.Flags().BoolVarP(&flagRecursive, "recursive", "r", false, "delete recursively")
	return cmd
}
