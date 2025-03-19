package main

import (
	"io/ioutil"
	"os"

	gozk "github.com/go-zookeeper/zk"
	"github.com/spf13/cobra"

	"a.yandex-team.ru/admins/zk"
)

var flagForce bool

func cmdCreate(client zk.Client) *cobra.Command {
	cmd := &cobra.Command{
		Use:   "create path [data] [-f]",
		Short: "create node with initial data",
		Long: `
Create makes a new node at the given path with the data given by
reading stdin or from arguments.

Example:

    $ echo content | zk create /path

    $ zk create /path "node data"`,
		Args: cobra.RangeArgs(1, 2),
		Run: func(cmd *cobra.Command, args []string) {
			ctx := cmd.Context()
			nodePath := fixPath(args[0])
			var data []byte
			switch len(args) {
			case 1:
				data, _ = ioutil.ReadAll(os.Stdin)
			case 2:
				data = []byte(args[1])
			}
			flags := int32(0)
			acl := gozk.WorldACL(gozk.PermAll)
			_, err := client.Create(ctx, nodePath, data, flags, acl)
			if flagForce && (err == gozk.ErrNodeExists || err == gozk.ErrNoNode) {
				err = client.EnsureZkPath(ctx, nodePath)
				must(err)
				_, err = client.Set(ctx, nodePath, data, -1)
			}
			must(err)
		},
	}
	cmd.Flags().BoolVarP(&flagForce, "force", "f", false, "force creation (ensure paren node)")
	return cmd
}
