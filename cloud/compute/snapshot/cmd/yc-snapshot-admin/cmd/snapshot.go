package cmd

import (
	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/common"
)

var (
	snapshotID string
)

// snapshotCmd represents create/delete nbs checkpoint
var snapshotCmd = &cobra.Command{
	Use:   "snapshot",
	Short: "Manage snapshots",
}

var snapshotCreateCmd = &cobra.Command{
	Use:   "create --snapshot-id=<snapshot-id> --disk-id=<disk-id> --cluster-id=<disk-cluster-id> [--checkpoint-id=<checkpoint-id>]",
	Short: "Create snapshot from disk from specified checkpoint",
	Args:  cobra.ExactArgs(0),
	RunE: func(cmd *cobra.Command, args []string) error {
		ctx := createContext()
		client, err := snapshotClient(ctx)
		if err != nil {
			return err
		}

		snapshotID = ensureAdminSuffix(snapshotID)

		return client.MoveSnapshot(ctx, &common.MoveRequest{
			OperationProjectID: adminOperationProjectID,
			Src: &common.NbsMoveSrc{
				ClusterID:        clusterID,
				DiskID:           diskID,
				LastCheckpointID: checkpointID,
			},
			Dst: &common.SnapshotMoveDst{
				SnapshotID: snapshotID,
				Create:     true,
				Commit:     true,
				Fail:       true,
				Name:       snapshotID,
			},
			Params: common.MoveParams{
				TaskID: createTaskID(ctx),
			},
		})
	},
}

var snapshotDeleteCmd = &cobra.Command{
	Use:   "delete --snapshot-id=<snapshot-id>",
	Short: "Delete snapshot",
	Args:  cobra.ExactArgs(0),
	RunE: func(cmd *cobra.Command, args []string) error {
		ctx := createContext()
		client, err := snapshotClient(ctx)
		if err != nil {
			return err
		}

		if doAdminAction(snapshotID) {
			return client.DeleteSnapshot(ctx, &common.DeleteRequest{
				ID: snapshotID,
				Params: common.DeleteParams{
					TaskID: createTaskID(ctx),
				},
			})
		} else {
			return nil
		}
	},
}

var snapshotRestoreCmd = &cobra.Command{
	Use:   "restore --snapshot-id=<snapshot-id> --disk-id=<disk-id> --cluster-id=<disk-cluster-id>",
	Short: "Restore snapshot to disk",
	Args:  cobra.ExactArgs(0),
	RunE: func(cmd *cobra.Command, args []string) error {
		ctx := createContext()
		client, err := snapshotClient(ctx)
		if err != nil {
			return err
		}

		return client.MoveSnapshot(ctx, &common.MoveRequest{
			OperationProjectID: adminOperationProjectID,
			Src: &common.SnapshotMoveSrc{
				SnapshotID: snapshotID,
			},
			Dst: &common.NbsMoveDst{
				ClusterID: clusterID,
				DiskID:    diskID,
				Mount:     true,
			},
			Params: common.MoveParams{
				TaskID: createTaskID(ctx),
			},
		})
	},
}

func init() {
	snapshotCmd.PersistentFlags().StringVar(&snapshotID, "snapshot-id", "", "snapshot id")
	_ = snapshotCmd.MarkPersistentFlagRequired("snapshot-id")

	snapshotCreateCmd.Flags().StringVar(&checkpointID, "checkpoint-id", "", "nbs checkpoint id")
	snapshotCreateCmd.Flags().StringVar(&diskID, "disk-id", "", "nbs disk id")
	_ = snapshotCreateCmd.MarkFlagRequired("disk-id")
	snapshotCreateCmd.Flags().StringVar(&clusterID, "cluster-id", "ru-central1-b", "nbs cluster id")
	_ = snapshotCreateCmd.MarkFlagRequired("cluster-id")

	snapshotCmd.AddCommand(snapshotCreateCmd)

	snapshotCmd.AddCommand(snapshotDeleteCmd)

	snapshotRestoreCmd.Flags().StringVar(&diskID, "disk-id", "", "nbs disk id")
	_ = snapshotRestoreCmd.MarkFlagRequired("disk-id")
	snapshotRestoreCmd.Flags().StringVar(&clusterID, "cluster-id", "ru-central1-b", "nbs cluster id")
	_ = snapshotRestoreCmd.MarkFlagRequired("cluster-id")
	snapshotCmd.AddCommand(snapshotRestoreCmd)

	rootCmd.AddCommand(snapshotCmd)
}
