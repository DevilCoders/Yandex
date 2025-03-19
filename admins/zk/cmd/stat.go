package main

import (
	"fmt"

	"a.yandex-team.ru/admins/zk"
	"github.com/spf13/cobra"
)

func cmdStat(client zk.Client) *cobra.Command {
	var cmdStat = &cobra.Command{
		Use:   "stat path",
		Short: "show node details",
		Long: `
	Stat writes to stdout details of the node at the given path.

	Example:

		$ zk stat /path
		Czxid:          337
		Mzxid:          460
		Ctime:          2014-05-17T08:11:24-07:00
		Mtime:          2014-05-17T14:49:45-07:00
		Version:        1
		Cversion:       3
		Aversion:       0
		EphemeralOwner: 0
		DataLength:     3
		Pzxid:          413`,
		Args: cobra.ExactArgs(1),
		Run: func(cmd *cobra.Command, args []string) {
			path := fixPath(args[0])
			_, stat, err := client.Get(cmd.Context(), path)
			must(err)
			fmt.Printf("Czxid:          %d\n", stat.Czxid)
			fmt.Printf("Mzxid:          %d\n", stat.Mzxid)
			fmt.Printf("Ctime:          %s\n", formatTime(stat.Ctime))
			fmt.Printf("Mtime:          %s\n", formatTime(stat.Mtime))
			fmt.Printf("Version:        %d\n", stat.Version)
			fmt.Printf("Cversion:       %d\n", stat.Cversion)
			fmt.Printf("Aversion:       %d\n", stat.Aversion)
			fmt.Printf("EphemeralOwner: %d\n", stat.EphemeralOwner)
			fmt.Printf("DataLength:     %d\n", stat.DataLength)
			fmt.Printf("Pzxid:          %d\n", stat.Pzxid)
		},
	}
	return cmdStat
}
