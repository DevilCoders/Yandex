package main

import (
	"fmt"
	"io/ioutil"
	"os"

	"github.com/golang/protobuf/proto"
	"github.com/spf13/cobra"

	client_config "a.yandex-team.ru/cloud/disk_manager/internal/pkg/configs/client/config"
	server_config "a.yandex-team.ru/cloud/disk_manager/internal/pkg/configs/server/config"
)

////////////////////////////////////////////////////////////////////////////////

func parseConfig(
	configFileName string,
	config proto.Message,
) error {

	configBytes, err := ioutil.ReadFile(configFileName)
	if err != nil {
		return fmt.Errorf(
			"failed to read config file %v: %w",
			configFileName,
			err,
		)
	}

	err = proto.UnmarshalText(string(configBytes), config)
	if err != nil {
		return fmt.Errorf(
			"failed to parse config file %v as protobuf: %w",
			configFileName,
			err,
		)
	}

	return nil
}

////////////////////////////////////////////////////////////////////////////////

func main() {
	var clientConfigFileName, serverConfigFileName string
	clientConfig := &client_config.ClientConfig{}
	serverConfig := &server_config.ServerConfig{}

	rootCmd := &cobra.Command{
		Use:   "yc-disk-manager-admin",
		Short: "Admin console for yc-disk-manager",
		PersistentPreRunE: func(cmd *cobra.Command, args []string) error {
			return parseConfig(clientConfigFileName, clientConfig)
		},
	}
	rootCmd.PersistentFlags().StringVar(
		&clientConfigFileName,
		"config",
		"/etc/yc/disk-manager/client-config.txt",
		"Path to the client config file",
	)
	rootCmd.PersistentFlags().StringVar(
		&serverConfigFileName,
		"server-config",
		"/etc/yc/disk-manager/server-config.txt",
		"Path to the server config file",
	)

	rootCmd.AddCommand(
		createOperationsCmd(clientConfig),
		createPrivateCmd(clientConfig),
	)

	additionalCommands := []*cobra.Command{
		createDisksCmd(clientConfig, serverConfig),
		createTasksCmd(clientConfig, serverConfig),
		createImagesCmd(clientConfig, serverConfig),
		createSnapshotsCmd(clientConfig, serverConfig),
		createFilesystemCmd(clientConfig, serverConfig),
		createPlacementGroupCmd(clientConfig, serverConfig),
	}

	parseClientAndServerConfig := func(cmd *cobra.Command, args []string) error {
		err := rootCmd.PersistentPreRunE(cmd, args)
		if err != nil {
			return err
		}

		return parseConfig(serverConfigFileName, serverConfig)
	}

	for _, cmd := range additionalCommands {
		cmd.PersistentPreRunE = parseClientAndServerConfig
		rootCmd.AddCommand(cmd)
	}

	if err := rootCmd.Execute(); err != nil {
		os.Exit(1)
	}
}
