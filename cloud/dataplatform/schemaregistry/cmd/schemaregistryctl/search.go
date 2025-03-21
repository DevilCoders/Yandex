package main

import (
	"fmt"
	"os"
	"strconv"
	"strings"

	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/dataplatform/api/schemaregistry"
	"a.yandex-team.ru/cloud/dataplatform/internal/heredoc"
	"a.yandex-team.ru/cloud/dataplatform/internal/printer"
)

func SearchCmd() *cobra.Command {
	var host, namespaceID, schemaID string
	var versionID int32
	var history bool
	var req schemaregistry.SearchRequest

	cmd := &cobra.Command{
		Use:     "search <query>",
		Aliases: []string{"search"},
		Short:   "Search",
		Long:    "Search your queries on schemas",
		Args:    cobra.ExactArgs(1),
		Example: heredoc.Doc(`
			$ schemaregistry search <query> --namespace=<namespace> --schema=<schema> --version=<version> --history=<history>
		`),
		Annotations: map[string]string{
			"group:core": "true",
		},
		RunE: func(cmd *cobra.Command, args []string) error {
			s := printer.Spin("")
			defer s.Stop()

			conn, err := resolveConn(host)
			if err != nil {
				return err
			}
			defer conn.Close()

			query := args[0]
			req.Query = query

			if len(schemaID) > 0 && len(namespaceID) == 0 {
				s.Stop()
				fmt.Println("Namespace ID not specified for", schemaID)
				return nil
			}
			req.NamespaceId = namespaceID
			req.SchemaId = schemaID

			if versionID != 0 {
				req.Version = &schemaregistry.SearchRequest_VersionId{
					VersionId: versionID,
				}
			} else if history {
				req.Version = &schemaregistry.SearchRequest_History{
					History: history,
				}
			}

			client := schemaregistry.NewSearchServiceClient(conn)
			res, err := client.Search(grpcContext(), &req)
			if err != nil {
				return err
			}

			hits := res.GetHits()

			report := [][]string{}
			total := 0
			s.Stop()

			if len(hits) == 0 {
				fmt.Printf("No results found")
				return nil
			}

			fmt.Printf(" \nFound results across %d schema(s)/version(s) \n\n", len(hits))

			report = append(report, []string{"TYPES", "NAMESPACE", "SCHEMA", "VERSION", "FIELDS"})
			for _, h := range hits {
				m := groupByType(h.GetFields())
				for t, f := range m {
					report = append(report, []string{
						t,
						h.GetNamespaceId(),
						h.GetSchemaId(),
						strconv.Itoa(int(h.GetVersionId())),
						strings.Join(f, ", "),
					})

					total++
				}
				report = append(report, []string{"", "", "", "", ""})
			}
			printer.Table(os.Stdout, report)

			fmt.Println("TOTAL: ", total)

			return nil
		},
	}

	cmd.Flags().StringVar(&host, "host", "", "schemaregistry host address eg: localhost:8000")
	checkErr(cmd.MarkFlagRequired("host"))
	cmd.Flags().StringVarP(&namespaceID, "namespace", "n", "", "parent namespace ID")
	cmd.Flags().StringVarP(&schemaID, "schema", "s", "", "related schema ID")
	cmd.Flags().Int32VarP(&versionID, "version", "v", 0, "version of the schema")
	cmd.Flags().BoolVarP(&history, "history", "h", false, "set this to enable history")

	return cmd
}

func groupByType(fields []string) map[string][]string {
	m := make(map[string][]string)
	for _, field := range fields {
		f := field[strings.LastIndex(field, ".")+1:]
		t := field[:strings.LastIndex(field, ".")]
		if m[t] != nil {
			m[t] = append(m[t], f)
		} else {
			m[t] = []string{f}
		}
	}
	return m
}
