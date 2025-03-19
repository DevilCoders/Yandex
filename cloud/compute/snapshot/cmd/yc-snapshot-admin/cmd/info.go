package cmd

import (
	"encoding/json"
	"fmt"
	"os"

	"github.com/spf13/cobra"
	"golang.org/x/xerrors"

	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/client"
)

var (
	searchSubstring string
)

// infoCmd represents the info command
var infoCmd = &cobra.Command{
	Use:   "info snapshot-id[ snapshot-id [ ...] ]",
	Short: "Show info about snapshot",
	RunE: func(cmd *cobra.Command, args []string) error {
		if len(args) == 0 {
			return xerrors.New("Need snapshot id")
		}
		ctx := createContext()
		client, err := snapshotClient(ctx)
		if err != nil {
			return err
		}

		var lastErr error
		for _, id := range args {
			info, err := client.GetSnapshotInfo(ctx, id)
			if err == nil {
				jsonBytes, _ := json.MarshalIndent(info, "", "  ")
				fmt.Printf("%s\n", jsonBytes)
			} else {
				lastErr = err
				_, _ = fmt.Fprintf(os.Stderr, "error while get info about snapshot id '%v': %v\n", id, err)
			}
		}

		return lastErr
	},
}

var snapshotListCmd = &cobra.Command{
	Use:   "list [--like <subname>]",
	Short: "Lists snapshot with like param as substring. Searches for \"admin\" snapshots by default",
	Args:  cobra.ExactArgs(0),
	RunE: func(cmd *cobra.Command, args []string) error {
		ctx := createContext()
		cl, err := snapshotClient(ctx)
		if err != nil {
			return err
		}

		if data, err := cl.ListSnapshots(ctx, client.ListSubstrArg(searchSubstring)); err == nil {
			jsonBytes, err := json.MarshalIndent(data, "", "  ")
			fmt.Printf("%s\n", jsonBytes)
			return err
		} else {
			return err
		}
	},
}

func init() {
	snapshotListCmd.Flags().StringVar(&searchSubstring, "like", "admin", "snapshots search option")
	rootCmd.AddCommand(snapshotListCmd)

	rootCmd.AddCommand(infoCmd)

	// Here you will define your flags and configuration settings.

	// Cobra supports Persistent Flags which will work for this command
	// and all subcommands, e.g.:
	// infoCmd.PersistentFlags().String("foo", "", "A help for foo")

	// Cobra supports local flags which will only run when this command
	// is called directly, e.g.:
	// infoCmd.Flags().BoolP("toggle", "t", false, "Help message for toggle")
}
