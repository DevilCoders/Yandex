package main

import (
	"io/ioutil"
	"os"

	"a.yandex-team.ru/admins/zk"
	"github.com/spf13/cobra"
)

func cmdSet(client zk.Client) *cobra.Command {
	cmd := &cobra.Command{
		Use:   "set path [data] [-f] [-v version]",
		Short: "write node data",
		Long: `
Set updates the node at the given path with the data given by reading
stdin or from arguments. If a version is given, submits that version with
the write request for verification against the current version.
Otherwise set will clobber the current data regardless of its
version.

Examples:

    $ echo new-content | zk set /path

    $ zk stat /path | grep Version
    Version:        3
    $ zk set /path "new-content" -v 3`,
		Args: cobra.RangeArgs(1, 2),
		Run: func(cmd *cobra.Command, args []string) {
			ctx := cmd.Context()
			path := fixPath(args[0])
			var data []byte
			switch len(args) {
			case 1:
				data, _ = ioutil.ReadAll(os.Stdin)
			case 2:
				data = []byte(args[1])
			}
			if flagForce {
				err := client.EnsureZkPath(ctx, path)
				must(err)
			}
			_, err := client.Set(ctx, path, data, flagVersion)
			must(err)
		},
	}
	cmd.Flags().Int32VarP(&flagVersion, "version", "v", -1, "Expected znode version")
	cmd.Flags().BoolVarP(&flagForce, "force", "f", false, "force creation (ensure paren node)")
	return cmd
}
