package main

import (
	"context"
	"crypto/tls"
	"fmt"
	"os"
	"strings"

	"github.com/spf13/cobra"
	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials"
	"google.golang.org/grpc/metadata"

	"a.yandex-team.ru/cloud/dataplatform/api/connman"
	"a.yandex-team.ru/cloud/dataplatform/internal/heredoc"
	"a.yandex-team.ru/cloud/dataplatform/internal/logger"
	"a.yandex-team.ru/cloud/dataplatform/internal/printer"
	"a.yandex-team.ru/transfer_manager/go/pkg/cleanup"
)

func connectionCmd() *cobra.Command {
	cmd := &cobra.Command{
		Use:     "connection",
		Aliases: []string{"con"},
		Short:   "Manage connection",
		Long: heredoc.Doc(`
			Work with connection info.
		`),
		Example: heredoc.Doc(`
			$ connman connection list
		`),
		Annotations: map[string]string{
			"group:core": "true",
		},
	}

	cmd.AddCommand(listConnectionCmd())

	return cmd
}

func listConnectionCmd() *cobra.Command {
	var host string
	var req connman.ListConnectionRequest

	cmd := &cobra.Command{
		Use:   "list",
		Short: "List all connections <folder-id>",
		Args:  cobra.ExactArgs(1),
		Example: heredoc.Doc(`
			$ connman connection list
	    	`),
		Annotations: map[string]string{
			"group:core": "true",
		},
		RunE: func(cmd *cobra.Command, args []string) error {
			spinner := printer.Spin("")
			defer spinner.Stop()

			conn, err := resolveConn(host)
			if err != nil {
				return err
			}
			cleanup.Close(conn, logger.Log)

			folderID := args[0]
			req.FolderId = folderID

			client := connman.NewConnectionServiceClient(conn)
			res, err := client.List(grpcContext(), &req)
			if err != nil {
				return err
			}

			report := [][]string{}

			schemas := res.GetConnection()

			spinner.Stop()

			if len(schemas) == 0 {
				fmt.Printf("%s has no connections", folderID)
				return nil
			}

			fmt.Printf(" \nShowing %d connections \n", len(schemas))

			report = append(report, []string{"NAME", "TYPE"})

			for _, s := range schemas {
				report = append(report, []string{
					s.Name,
					fmt.Sprintf("%T", s.GetParams().Type),
				})
			}
			printer.Table(os.Stdout, report)
			return nil
		},
	}

	cmd.Flags().StringVar(&host, "host", "", "connman host address eg: localhost:8000")
	checkErr(cmd.MarkFlagRequired("host"))

	return cmd
}

func checkErr(err error) {
	if err != nil {
		logger.Log.Fatalf("error: %v", err)
	}
}

func grpcContext() context.Context {
	ctx := context.Background()
	if os.Getenv("IAM_TOKEN") != "" {
		ctx = metadata.AppendToOutgoingContext(ctx, "Authorization", fmt.Sprintf("OAuth %s", os.Getenv("SR_TOKEN")))
	} else {
		logger.Log.Fatalf("unable to find schema registry token in ENV (SR_TOKEN)")
	}
	return ctx
}

func resolveConn(host string) (*grpc.ClientConn, error) {
	if strings.HasPrefix(host, "localhost") || os.Getenv("GRPC_PLAINTEXT") == "1" {
		conn, err := grpc.Dial(host, grpc.WithInsecure())
		if err != nil {
			return nil, err
		}
		return conn, err
	}
	conn, err := grpc.Dial(host, grpc.WithTransportCredentials(credentials.NewTLS(&tls.Config{InsecureSkipVerify: true})))
	if err != nil {
		return nil, err
	}
	return conn, err
}
