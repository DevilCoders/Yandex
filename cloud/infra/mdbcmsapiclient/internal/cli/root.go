package cli

import (
	"context"
	"crypto/md5"
	"encoding/json"
	"errors"
	"fmt"

	"github.com/gofrs/uuid"
	"github.com/spf13/cobra"
	"google.golang.org/grpc/metadata"

	cmsapi "a.yandex-team.ru/cloud/mdb/cms/api/pkg/instanceclient/grpc"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

const appName = "yc-infra-tool"

var (
	apiEndpoint string
	instanceID  string
	operationID string
	comment     string
	iamToken    string
)

// PerRPCCredentialsStatic provides static credentials for authentication
type PerRPCCredentialsStatic struct {
	IamToken        string
}

func Execute(version string) error {
	cmd := rootCmd()
	cmd.Version = version
	return cmd.Execute()
}

func rootCmd() *cobra.Command {
	cmd := &cobra.Command{
		Use:   "mdp-api-cli",
		Short: "CLI for MDB CMS API",
		RunE: func(cmd *cobra.Command, args []string) error {
			return cobra.OnlyValidArgs(cmd, args)
		},
		SilenceUsage: true,
	}
	cmd.PersistentFlags().StringVarP(&apiEndpoint, "endpoint", "", "", "API endpoint")
	cmd.PersistentFlags().StringVarP(&iamToken, "iam-token", "", "", "iam-token for authentication")
	if err := cmd.MarkPersistentFlagRequired("endpoint"); err != nil {
		panic(err)
	}
	cmd.AddCommand(moveInstanceCMD())
	cmd.AddCommand(getOperationCMD())
	return cmd
}

func moveInstanceCMD() *cobra.Command {
	cmd := &cobra.Command{
		Use:       "move-instance",
		ValidArgs: []string{"id", "comment"},
		Short:     "Create request for move instance",
		RunE: func(cmd *cobra.Command, _ []string) error {
			client, err := newClient()
			if err != nil {
				return err
			}
			ctx, err := withIdempotenceID(instanceID)
			if err != nil {
				return err
			}
			operation, err := client.CreateMoveInstanceOperation(ctx, instanceID, comment)
			if err != nil {
				return err
			}
			return outputOperation(operation)
		},
	}
	cmd.Flags().StringVarP(&instanceID, "id", "", "", "instance id")
	cmd.Flags().StringVarP(&comment, "comment", "", "", "comment for request")
	_ = cmd.MarkFlagRequired("id")
	return cmd
}

func getOperationCMD() *cobra.Command {
	cmd := &cobra.Command{
		Use:       "get-operation",
		ValidArgs: []string{"id"},
		Short:     "Get migrate operation status",
		RunE: func(cmd *cobra.Command, _ []string) error {
			client, err := newClient()
			if err != nil {
				return err
			}
			operation, err := client.GetInstanceOperation(context.Background(), operationID)
			if err != nil {
				return err
			}
			return outputOperation(operation)
		},
	}
	cmd.Flags().StringVarP(&operationID, "id", "", "", "instance id")
	_ = cmd.MarkFlagRequired("id")
	return cmd
}

func (s *PerRPCCredentialsStatic) GetRequestMetadata(_ context.Context, _ ...string) (map[string]string, error) {
	return map[string]string{
		"authorization": "Bearer " + s.IamToken,
	}, nil
}

func (s *PerRPCCredentialsStatic) RequireTransportSecurity() bool {
	return true
}

func newClient() (*cmsapi.InstanceClient, error) {
	logger, err := zap.New(zap.CLIConfig(log.InfoLevel))
	if err != nil {
		return nil, err
	}

	clientCfg := cmsapi.DefaultConfig()
	clientCfg.Host = apiEndpoint
	creds := &PerRPCCredentialsStatic{IamToken: iamToken}
	return cmsapi.NewFromConfig(context.Background(), clientCfg, appName, logger, creds)
}

func withIdempotenceID(instanceID string) (context.Context, error) {
	if instanceID == "" {
		return nil, errors.New("create Idempotency-Key error: instance ID is empty")
	}

	hash := md5.Sum([]byte(instanceID))
	newUUID, err := uuid.FromBytes(hash[:])
	if err != nil {
		return nil, fmt.Errorf("create Idempotency-Key error: %w", err)
	}

	md := metadata.MD{}
	md.Set("idempotency-key", newUUID.String())
	return metadata.NewOutgoingContext(context.Background(), md), nil
}

func outputOperation(operation interface{}) error {
	data, err := json.Marshal(operation)
	if err != nil {
		return err
	}
	fmt.Println(string(data))
	return nil
}
