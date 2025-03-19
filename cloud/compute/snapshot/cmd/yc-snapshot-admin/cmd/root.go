package cmd

import (
	"fmt"
	"os"

	"golang.org/x/net/context"

	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/globalauth"

	"github.com/spf13/cobra"
)

const (
	envForToken = "SNAPSHOT_ADMIN_TOKEN"

	envTokenPolicy = "env"
)

var (
	flagEndPoint    string
	flagNBSEndpoint string
	flagAuthMethod  string

	providedToken string
)

// rootCmd represents the base command when called without any subcommands
var rootCmd = &cobra.Command{
	Use:   "yc-snapshot-admin",
	Short: "Command line access to snapshot service",
	Long: `Command line access to snapshot service
Allow direct access to snapshot service API`,
	PersistentPreRun: func(cmd *cobra.Command, args []string) {
		switch flagAuthMethod {
		case globalauth.NoneTokenPolicy, globalauth.MetadataTokenPolicy:
			globalauth.InitCredentials(context.Background(), flagAuthMethod)
		case envTokenPolicy:
			providedToken = os.Getenv(envForToken)
		default:
			providedToken = flagAuthMethod
		}
	},
}

// Execute adds all child commands to the root command and sets flags appropriately.
// This is called by main.main(). It only needs to happen once to the rootCmd.
func Execute() {
	if err := rootCmd.Execute(); err != nil {
		fmt.Println(err)
		os.Exit(1)
	}
}

func init() {
	rootCmd.PersistentFlags().StringVar(&flagEndPoint, "endpoint", "localhost:7627", "endpoint for call snapshot service")
	rootCmd.PersistentFlags().StringVar(&flagNBSEndpoint, "nbs-endpoint", "", "endpoint for call nbs service (taken from snapshot config if empty)")
	rootCmd.PersistentFlags().StringVar(&flagAuthMethod, "auth", globalauth.NoneTokenPolicy, "can be none/metadata/env/<your token>; env reads from SNAPSHOT_ADMIN_TOKEN var")
}
