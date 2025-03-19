package cmd

import (
	"context"
	"time"

	"github.com/spf13/cobra"
	"github.com/spf13/pflag"

	"a.yandex-team.ru/cloud/mdb/image-releaser/internal"
	"a.yandex-team.ru/cloud/mdb/internal/cli"
)

var (
	computeSubCommand = &cli.Command{
		Cmd: &cobra.Command{
			Use: "compute",
		},
		SubCommands: []*cli.Command{computeReleaseSubCommand, dataprocReleaseSubCommand, computeAgeSubCommand, computeCleanupSubCommand},
	}
	computeReleaseSubCommand = &cli.Command{
		Cmd: &cobra.Command{
			Short: "Creates or releases images in compute",
			Use:   "release",
		},
		Run: releaseCompute,
	}
	dataprocReleaseSubCommand = &cli.Command{
		Cmd: &cobra.Command{
			Short: "Creates or releases Dataproc images in compute",
			Use:   "dataproc-release",
		},
		Run: releaseDataproc,
	}
	computeCleanupSubCommand = &cli.Command{
		Cmd: &cobra.Command{
			Use:   "cleanup",
			Short: "Removes old images",
		},
		Run: cleanupCompute,
	}
	computeAgeSubCommand = &cli.Command{
		Cmd: &cobra.Command{
			Use:   "age",
			Short: "Checks image age",
		},
		Run: ageCompute,
	}

	//flags
	computeReqStability                   time.Duration
	computeImageName                      string
	computeCheckHost, computeCheckService string
	computeNoCheck                        bool
	computeCleanup                        bool
	computePoolSize                       int
	computeFolderID                       string
	computeKeepImages                     int
	computeOS                             string
	productIDs                            []string
	version                               string
	versionPrefix                         string
	computeImageID                        string
	imageFamily                           string
	isForce                               bool
)

func init() {
	computeCommonFlags := pflag.NewFlagSet("compute", pflag.ContinueOnError)
	computeCommonFlags.StringVar(&computeImageName, "name", "", "Image name")
	computeCommonFlags.StringVar(&computeFolderID, "folder", "", "Destination folder ID")

	//release
	computeReleaseSubCommand.Cmd.PersistentFlags().DurationVar(&computeReqStability, "stable", time.Hour*4, "Required stability duration")
	computeReleaseSubCommand.Cmd.PersistentFlags().StringVar(&computeCheckHost, "check-host", "", "Juggler check hostname")
	computeReleaseSubCommand.Cmd.PersistentFlags().StringVar(&computeCheckService, "check-service", "", "Juggler check service")
	computeReleaseSubCommand.Cmd.PersistentFlags().BoolVar(&computeNoCheck, "no-check", false, "Release without checking stability")
	computeReleaseSubCommand.Cmd.PersistentFlags().BoolVar(&computeCleanup, "cleanup", false, "Cleanup images after release")
	computeReleaseSubCommand.Cmd.PersistentFlags().IntVar(&computeKeepImages, "keep-images", 3, "Number of last images to keep")
	computeReleaseSubCommand.Cmd.PersistentFlags().IntVar(&computePoolSize, "pool-size", 3, "Size of NBS-pool to create")
	computeReleaseSubCommand.Cmd.PersistentFlags().StringVar(&computeOS, "os", "linux", "Type of OS: linux or windows")
	computeReleaseSubCommand.Cmd.PersistentFlags().StringSliceVar(&productIDs, "product-ids", nil, "product IDs for billing")
	addFlags(computeReleaseSubCommand.Cmd, computeCommonFlags, []string{"name", "folder"})

	//release dataproc
	dataprocReleaseSubCommand.Cmd.PersistentFlags().StringVar(&computeImageID, "image-id", "", "Image id to set new version")
	_ = dataprocReleaseSubCommand.Cmd.MarkPersistentFlagRequired("image-id")
	dataprocReleaseSubCommand.Cmd.PersistentFlags().StringVar(&computeFolderID, "folder", "", "Compute folder ID")
	_ = dataprocReleaseSubCommand.Cmd.MarkPersistentFlagRequired("folder")
	dataprocReleaseSubCommand.Cmd.PersistentFlags().StringVar(&imageFamily, "image-family", "", "Image family")
	_ = dataprocReleaseSubCommand.Cmd.MarkPersistentFlagRequired("image-family")
	dataprocReleaseSubCommand.Cmd.PersistentFlags().StringVar(&version, "version", "", "Specific version to set for image instead of autoincrement bump")
	dataprocReleaseSubCommand.Cmd.PersistentFlags().StringVar(&versionPrefix, "version-prefix", "", "Version prefix like \"\", \"1\", \"2.0\" or \"3.1.4\"")
	dataprocReleaseSubCommand.Cmd.PersistentFlags().BoolVar(&isForce, "force", false, "Set image version even if it is already assigned")

	//cleanup
	computeCleanupSubCommand.Cmd.PersistentFlags().IntVar(&computeKeepImages, "keep-images", 3, "Number of last images to keep")
	addFlags(computeCleanupSubCommand.Cmd, computeCommonFlags, []string{"name", "folder"})
	//age
	computeAgeSubCommand.Cmd.PersistentFlags().DurationVar(&warnSince, "warn", time.Hour*25, "Warn level")
	computeAgeSubCommand.Cmd.PersistentFlags().DurationVar(&critSince, "crit", time.Hour*50, "Crit level")
	addFlags(computeAgeSubCommand.Cmd, computeCommonFlags, []string{"name", "folder"})
}

func releaseCompute(ctx context.Context, env *cli.Env, cmd *cobra.Command, args []string) {
	releaser := internal.NewApp(conf, env.App)
	releaser.ReleaseCompute(ctx, computeImageName, computeOS, productIDs, computePoolSize, computeFolderID, computeNoCheck, computeCheckHost, computeCheckService, computeReqStability)
	if computeCleanup {
		releaser.CleanupComputeImages(ctx, computeImageName, computeKeepImages, computeFolderID)
	}
}

func releaseDataproc(ctx context.Context, env *cli.Env, cmd *cobra.Command, args []string) {
	releaser := internal.NewApp(conf, env.App)
	releaser.ReleaseDataproc(ctx, computeImageID, version, versionPrefix, imageFamily, isForce, computeFolderID)
}

func cleanupCompute(ctx context.Context, env *cli.Env, cmd *cobra.Command, args []string) {
	releaser := internal.NewApp(conf, env.App)
	releaser.CleanupComputeImages(ctx, computeImageName, computeKeepImages, computeFolderID)
}
