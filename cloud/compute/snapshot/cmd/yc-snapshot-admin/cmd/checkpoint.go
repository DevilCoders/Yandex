package cmd

import (
	"github.com/spf13/cobra"
)

var (
	checkpointID string
	diskID       string
	clusterID    string
)

// checkpointCmd represents create/delete nbs checkpoint
var checkpointCmd = &cobra.Command{
	Use:   "checkpoint",
	Short: "Manage nbs disk checkpoint",
}

var checkpointCreateCmd = &cobra.Command{
	Use:   "create --checkpoint-id=<checkpoint-id> --disk-id=<disk-id>",
	Short: "Create nbs disk checkpoint",
	Args:  cobra.ExactArgs(0),
	RunE: func(cmd *cobra.Command, args []string) error {
		if client, err := nbsClient(flagNBSEndpoint); err == nil {
			checkpointID = ensureAdminSuffix(checkpointID)
			return client.CreateCheckpoint(createContext(), diskID, checkpointID)
		} else {
			return err
		}
	},
}

var checkpointDeleteCmd = &cobra.Command{
	Use:   "delete --checkpoint-id=<checkpoint-id> --disk-id=<disk-id>",
	Short: "Delete nbs disk checkpoint",
	Args:  cobra.ExactArgs(0),
	RunE: func(cmd *cobra.Command, args []string) error {
		if client, err := nbsClient(flagNBSEndpoint); err == nil {
			if doAdminAction(checkpointID) {
				return client.DeleteCheckpoint(createContext(), diskID, checkpointID)
			} else {
				return nil
			}
		} else {
			return err
		}
	},
}

func init() {
	checkpointCmd.PersistentFlags().StringVar(&checkpointID, "checkpoint-id", "", "nbs checkpoint id")
	_ = checkpointCmd.MarkPersistentFlagRequired("checkpoint-id")
	checkpointCmd.PersistentFlags().StringVar(&diskID, "disk-id", "", "nbs disk id")
	_ = checkpointCmd.MarkPersistentFlagRequired("disk-id")

	checkpointCmd.AddCommand(checkpointCreateCmd)
	checkpointCmd.AddCommand(checkpointDeleteCmd)

	rootCmd.AddCommand(checkpointCmd)
}
