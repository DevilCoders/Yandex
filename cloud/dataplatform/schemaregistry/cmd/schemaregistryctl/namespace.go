package main

import (
	"fmt"
	"os"

	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/dataplatform/api/schemaregistry"
	"a.yandex-team.ru/cloud/dataplatform/internal/heredoc"
	"a.yandex-team.ru/cloud/dataplatform/internal/printer"
)

func NamespaceCmd() *cobra.Command {
	cmd := &cobra.Command{
		Use:     "namespace",
		Aliases: []string{"namespace"},
		Short:   "Manage namespace",
		Long:    "Work with namespaces.",
		Example: heredoc.Doc(`
			$ schemaregistry namespace list
			$ schemaregistry namespace create
			$ schemaregistry namespace view
			$ schemaregistry namespace edit
			$ schemaregistry namespace delete
		`),
		Annotations: map[string]string{
			"group:core": "true",
		},
	}

	cmd.AddCommand(listNamespaceCmd())
	cmd.AddCommand(createNamespaceCmd())
	cmd.AddCommand(getNamespaceCmd())
	cmd.AddCommand(updateNamespaceCmd())
	cmd.AddCommand(deleteNamespaceCmd())

	return cmd
}

func listNamespaceCmd() *cobra.Command {
	var host, folder string
	var req schemaregistry.ListNamespacesRequest

	cmd := &cobra.Command{
		Use:   "list",
		Short: "List all namespaces",
		Long:  "List and filter namespaces.",
		Args:  cobra.NoArgs,
		Example: heredoc.Doc(`
			$ schemaregistry namespace list
		`),
		Annotations: map[string]string{
			"group:core": "true",
		},
		RunE: func(cmd *cobra.Command, args []string) error {
			conn, err := resolveConn(host)
			if err != nil {
				return err
			}
			defer conn.Close()
			client := schemaregistry.NewNamespaceServiceClient(conn)
			req.FolderId = folder
			res, err := client.List(grpcContext(), &req)
			if err != nil {
				return err
			}

			report := [][]string{}

			namespaces := res.GetNamespaces()

			fmt.Printf(" \nShowing %[1]d of %[1]d namespaces \n \n", len(namespaces))

			report = append(report, []string{"INDEX", "NAMESPACE", "FORMAT", "COMPATIBILITY", "DESCRIPTION"})
			index := 1

			for _, n := range namespaces {
				report = append(report, []string{n, "-", "-", "-"})
				index++
			}
			printer.Table(os.Stdout, report)
			return nil
		},
	}

	cmd.Flags().StringVar(&folder, "folder", "", "folder ID")
	checkErr(cmd.MarkFlagRequired("folder"))
	cmd.Flags().StringVar(&host, "host", "schema-registry-testing.in.yandex-team.ru:8443", "schemaregistry host address eg: schema-registry-testing.in.yandex-team.ru:8443")
	checkErr(cmd.MarkFlagRequired("host"))

	return cmd
}

func createNamespaceCmd() *cobra.Command {
	var host, format, comp, folder string
	var desc string
	var req schemaregistry.CreateNamespaceRequest

	cmd := &cobra.Command{
		Use:   "create",
		Short: "Create a namespace",
		Args:  cobra.ExactArgs(1),
		Example: heredoc.Doc(`
			$ schemaregistry namespace create <namespace-id> --format=<schema-format> --comp=<schema-compatibility> --desc=<description> --folder=<folder>
		`),
		Annotations: map[string]string{
			"group:core": "true",
		},
		RunE: func(cmd *cobra.Command, args []string) error {
			conn, err := resolveConn(host)
			if err != nil {
				return err
			}
			defer conn.Close()

			id := args[0]

			req.Id = id
			req.Format = schemaregistry.Schema_Format(schemaregistry.Schema_Format_value[format])
			req.Compatibility = schemaregistry.Schema_Compatibility(schemaregistry.Schema_Compatibility_value[comp])
			req.Description = desc

			client := schemaregistry.NewNamespaceServiceClient(conn)
			res, err := client.Create(grpcContext(), &req)
			if err != nil {
				return err
			}

			namespace := res.GetNamespace()

			fmt.Printf("Namespace successfully created with id: %s", namespace.GetId())
			return nil
		},
	}

	cmd.Flags().StringVar(&host, "host", "", "schemaregistry host address eg: localhost:8000")
	checkErr(cmd.MarkFlagRequired("host"))

	cmd.Flags().StringVarP(&format, "format", "f", "", "schema format")
	checkErr(cmd.MarkFlagRequired("format"))

	cmd.Flags().StringVarP(&comp, "comp", "c", "", "schema compatibility")
	checkErr(cmd.MarkFlagRequired("comp"))
	cmd.Flags().StringVar(&folder, "folder", "", "folder ID")
	checkErr(cmd.MarkFlagRequired("folder"))

	cmd.Flags().StringVarP(&desc, "desc", "d", "", "description")

	return cmd
}

func updateNamespaceCmd() *cobra.Command {
	var host, format, comp string
	var desc string
	var req schemaregistry.UpdateNamespaceRequest

	cmd := &cobra.Command{
		Use:   "edit",
		Short: "Edit a namespace",
		Args:  cobra.ExactArgs(1),
		Example: heredoc.Doc(`
			$ schemaregistry namespace edit <namespace-id> --format=<schema-format> --comp=<schema-compatibility> --desc=<description>
		`),
		Annotations: map[string]string{
			"group:core": "true",
		},
		RunE: func(cmd *cobra.Command, args []string) error {
			conn, err := resolveConn(host)
			if err != nil {
				return err
			}
			defer conn.Close()

			id := args[0]

			req.Id = id
			req.Format = schemaregistry.Schema_Format(schemaregistry.Schema_Format_value[format])
			req.Compatibility = schemaregistry.Schema_Compatibility(schemaregistry.Schema_Compatibility_value[comp])
			req.Description = desc

			client := schemaregistry.NewNamespaceServiceClient(conn)
			_, err = client.Update(grpcContext(), &req)
			if err != nil {
				return err
			}

			fmt.Printf("Namespace successfully updated")
			return nil
		},
	}

	cmd.Flags().StringVar(&host, "host", "", "schemaregistry host address eg: localhost:8000")
	checkErr(cmd.MarkFlagRequired("host"))

	cmd.Flags().StringVarP(&format, "format", "f", "", "schema format")
	checkErr(cmd.MarkFlagRequired("format"))

	cmd.Flags().StringVarP(&comp, "comp", "c", "", "schema compatibility")
	checkErr(cmd.MarkFlagRequired("comp"))

	cmd.Flags().StringVarP(&desc, "desc", "d", "", "description")
	checkErr(cmd.MarkFlagRequired("desc"))

	return cmd
}

func getNamespaceCmd() *cobra.Command {
	var host string
	var req schemaregistry.GetNamespaceRequest

	cmd := &cobra.Command{
		Use:   "view",
		Short: "View a namespace",
		Args:  cobra.ExactArgs(1),
		Example: heredoc.Doc(`
			$ schemaregistry namespace view <namespace-id>
		`),
		Annotations: map[string]string{
			"group:core": "true",
		},
		RunE: func(cmd *cobra.Command, args []string) error {
			conn, err := resolveConn(host)
			if err != nil {
				return err
			}
			defer conn.Close()

			id := args[0]

			req.Id = id

			client := schemaregistry.NewNamespaceServiceClient(conn)
			res, err := client.Get(grpcContext(), &req)
			if err != nil {
				return err
			}

			report := [][]string{}

			namespace := res.GetNamespace()

			report = append(report, []string{"ID", "FORMAT", "COMPATIBILITY", "DESCRIPTION"})
			report = append(report, []string{
				namespace.GetId(),
				namespace.GetFormat().String(),
				namespace.GetCompatibility().String(),
				namespace.GetDescription(),
			})
			printer.Table(os.Stdout, report)
			return nil
		},
	}

	cmd.Flags().StringVar(&host, "host", "", "schemaregistry host address eg: localhost:8000")
	checkErr(cmd.MarkFlagRequired("host"))

	return cmd
}

func deleteNamespaceCmd() *cobra.Command {
	var host string
	var req schemaregistry.DeleteNamespaceRequest

	cmd := &cobra.Command{
		Use:   "delete",
		Short: "Delete a namespace",
		Args:  cobra.ExactArgs(1),
		Example: heredoc.Doc(`
			$ schemaregistry namespace delete <namespace-id>
		`),
		Annotations: map[string]string{
			"group:core": "true",
		},
		RunE: func(cmd *cobra.Command, args []string) error {
			conn, err := resolveConn(host)
			if err != nil {
				return err
			}
			defer checkErr(conn.Close())

			id := args[0]

			req.Id = id

			client := schemaregistry.NewNamespaceServiceClient(conn)
			_, err = client.Delete(grpcContext(), &req)
			if err != nil {
				return err
			}

			fmt.Printf("Namespace successfully deleted")

			return nil
		},
	}

	cmd.Flags().StringVar(&host, "host", "", "schemaregistry host address eg: localhost:8000")
	checkErr(cmd.MarkFlagRequired("host"))

	return cmd
}
