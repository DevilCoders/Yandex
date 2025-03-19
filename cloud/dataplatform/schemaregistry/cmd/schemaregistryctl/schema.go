package main

import (
	"bytes"
	"errors"
	"fmt"
	"io/ioutil"
	"os"
	"path/filepath"
	"strconv"
	"strings"

	"github.com/jhump/protoreflect/desc"
	"github.com/jhump/protoreflect/desc/protoprint"
	"github.com/spf13/cobra"
	"google.golang.org/grpc"
	"google.golang.org/grpc/status"
	"google.golang.org/protobuf/encoding/protojson"
	"google.golang.org/protobuf/proto"
	"google.golang.org/protobuf/reflect/protodesc"
	"google.golang.org/protobuf/reflect/protoreflect"
	"google.golang.org/protobuf/types/descriptorpb"

	"a.yandex-team.ru/cloud/dataplatform/api/schemaregistry"
	"a.yandex-team.ru/cloud/dataplatform/internal/heredoc"
	"a.yandex-team.ru/cloud/dataplatform/internal/printer"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/transfer_manager/go/pkg/util"
)

func SchemaCmd() *cobra.Command {
	cmd := &cobra.Command{
		Use:     "schema",
		Aliases: []string{"schema"},
		Short:   "Manage schema",
		Long: heredoc.Doc(`
			Work with schemas.
		`),
		Example: heredoc.Doc(`
			$ schemaregistry schema list
			$ schemaregistry schema create
			$ schemaregistry schema bulk-create
			$ schemaregistry schema view
			$ schemaregistry schema edit
			$ schemaregistry schema delete
			$ schemaregistry schema version
			$ schemaregistry schema graph
			$ schemaregistry schema print
			$ schemaregistry schema check
			$ schemaregistry schema bulk-check
		`),
		Annotations: map[string]string{
			"group:core": "true",
		},
	}

	cmd.AddCommand(createSchemaCmd())
	cmd.AddCommand(bulkCreateSchemaCmd())
	cmd.AddCommand(checkSchemaCmd())
	cmd.AddCommand(bulkCheckSchemaCmd())
	cmd.AddCommand(listSchemaCmd())
	cmd.AddCommand(getSchemaCmd())
	cmd.AddCommand(updateSchemaCmd())
	cmd.AddCommand(deleteSchemaCmd())
	cmd.AddCommand(diffSchemaCmd())
	cmd.AddCommand(versionSchemaCmd())
	cmd.AddCommand(printCmd())
	cmd.AddCommand(graphCmd())

	return cmd
}

func listSchemaCmd() *cobra.Command {
	var host string
	var req schemaregistry.ListSchemasRequest

	cmd := &cobra.Command{
		Use:   "list",
		Short: "List all schemas",
		Args:  cobra.ExactArgs(1),
		Example: heredoc.Doc(`
			$ schemaregistry schema list <namespace-id>
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
			defer conn.Close()

			namespaceID := args[0]
			req.Id = namespaceID

			client := schemaregistry.NewSchemaServiceClient(conn)
			res, err := client.List(grpcContext(), &req)
			if err != nil {
				return err
			}

			report := [][]string{}

			schemas := res.GetSchemas()

			spinner.Stop()

			if len(schemas) == 0 {
				fmt.Printf("%s has no schemas", namespaceID)
				return nil
			}

			fmt.Printf(" \nShowing %d schemas \n", len(schemas))

			report = append(report, []string{"SCHEMA"})

			for _, s := range schemas {
				report = append(report, []string{
					s,
				})
			}
			printer.Table(os.Stdout, report)
			return nil
		},
	}

	cmd.Flags().StringVar(&host, "host", "", "schemaregistry host address eg: localhost:8000")
	checkErr(cmd.MarkFlagRequired("host"))

	return cmd
}

func createSchemaCmd() *cobra.Command {
	var host, format, comp, filePath, namespaceID string
	var req schemaregistry.CreateSchemaRequest

	cmd := &cobra.Command{
		Use:   "create",
		Short: "Create a schema",
		Args:  cobra.ExactArgs(1),
		Example: heredoc.Doc(`
			$ schemaregistry schema create <schema-id> --namespace=<namespace-id> --format=<schema-format> –-comp=<schema-compatibility> –-filePath=<schema-filePath>
	    	`),
		Annotations: map[string]string{
			"group:core": "true",
		},
		RunE: func(cmd *cobra.Command, args []string) error {
			spinner := printer.Spin("")
			defer spinner.Stop()

			fileData, err := ioutil.ReadFile(filePath)
			if err != nil {
				return err
			}
			req.Data = fileData

			conn, err := resolveConn(host)
			if err != nil {
				return err
			}
			defer conn.Close()
			schemaID := args[0]

			req.NamespaceId = namespaceID
			req.SchemaId = schemaID
			req.Format = schemaregistry.Schema_Format(schemaregistry.Schema_Format_value[format])
			req.Compatibility = schemaregistry.Schema_Compatibility(schemaregistry.Schema_Compatibility_value[comp])

			client := schemaregistry.NewSchemaServiceClient(conn)
			res, err := client.Create(grpcContext(), &req)
			if err != nil {
				errStatus := status.Convert(err)
				return errors.New(errStatus.Message())
			}

			id := res.GetId()

			spinner.Stop()
			fmt.Printf("schema successfully created with id: %s", id)
			return nil
		},
	}

	cmd.Flags().StringVar(&host, "host", "", "schemaregistry host address eg: localhost:8000")
	checkErr(cmd.MarkFlagRequired("host"))

	cmd.Flags().StringVarP(&namespaceID, "namespace", "n", "", "parent namespace ID")
	checkErr(cmd.MarkFlagRequired("namespace"))

	cmd.Flags().StringVarP(&format, "format", "f", "", "schema format")
	checkErr(cmd.MarkFlagRequired("format"))

	cmd.Flags().StringVarP(&comp, "comp", "c", "", "schema compatibility")
	checkErr(cmd.MarkFlagRequired("comp"))

	cmd.Flags().StringVarP(&filePath, "filePath", "F", "", "path to the schema file")
	checkErr(cmd.MarkFlagRequired("filePath"))

	return cmd
}

func bulkCreateSchemaCmd() *cobra.Command {
	var host, format, comp, glob, namespaceID, output string

	cmd := &cobra.Command{
		Use:   "bulk-create",
		Short: "Bulk create a schema",
		Example: heredoc.Doc(`
			$ schemaregistry schema bulk-create --namespace=<namespace-id> --comp=<schema-compatibility> --glob=<glob-patterb>
	    	`),
		Annotations: map[string]string{
			"group:core": "true",
		},
		RunE: func(cmd *cobra.Command, args []string) error {
			resbuf := bytes.NewBuffer(nil)
			matches, err := filepath.Glob(glob)
			if err != nil {
				return err
			}
			fmt.Printf("found: %v with glob: %s files", len(matches), glob)
			conn, err := resolveConn(host)
			if err != nil {
				return err
			}
			defer conn.Close()

			var errs util.Errors
			for _, file := range matches {
				if !strings.HasSuffix(filepath.Base(file), ".desc") {
					continue
				}
				schemaID := strings.TrimSuffix(filepath.Base(file), ".desc")
				resbuf.WriteString(fmt.Sprintf("\n%s\n", schemaID))
				res, err := doCreateSchema(conn, namespaceID, schemaID, format, comp, file)
				if err != nil {
					resbuf.WriteString(fmt.Sprintf("	failed: %v", err))
					errs = append(errs, err)
					continue
				}
				resbuf.WriteString(fmt.Sprintf("	version: %d created. ID: %s\n", res.Version, res.Id))
			}
			if output != "" {
				if err := ioutil.WriteFile(output, resbuf.Bytes(), 0600); err != nil {
					errs = append(errs, err)
				}
			} else {
				fmt.Print(resbuf.String())
			}
			if len(errs) > 0 {
				return xerrors.Errorf("found: %v\nerrs: %w", len(errs), errs)
			}

			return nil
		},
	}

	cmd.Flags().StringVar(&host, "host", "", "schemaregistry host address eg: localhost:8000")
	checkErr(cmd.MarkFlagRequired("host"))

	cmd.Flags().StringVarP(&namespaceID, "namespace", "n", "", "parent namespace ID")
	checkErr(cmd.MarkFlagRequired("namespace"))

	cmd.Flags().StringVarP(&format, "format", "f", "", "schema format")
	checkErr(cmd.MarkFlagRequired("format"))

	cmd.Flags().StringVarP(&comp, "comp", "c", "", "schema compatibility")
	checkErr(cmd.MarkFlagRequired("comp"))

	cmd.Flags().StringVarP(&glob, "glob", "g", "", "glob to the schema file")
	checkErr(cmd.MarkFlagRequired("glob"))

	cmd.Flags().StringVarP(&output, "output", "o", "", "report output")

	return cmd
}

func doCreateSchema(conn *grpc.ClientConn, namespaceID string, schemaID string, format string, comp string, file string) (*schemaregistry.CreateSchemaResponse, error) {
	fileData, err := ioutil.ReadFile(file)
	if err != nil {
		return nil, err
	}

	var req schemaregistry.CreateSchemaRequest
	req.NamespaceId = namespaceID
	req.SchemaId = schemaID
	req.Data = fileData
	req.Format = schemaregistry.Schema_Format(schemaregistry.Schema_Format_value[format])
	req.Compatibility = schemaregistry.Schema_Compatibility(schemaregistry.Schema_Compatibility_value[comp])

	client := schemaregistry.NewSchemaServiceClient(conn)
	res, err := client.Create(grpcContext(), &req)
	if err != nil {
		errStatus := status.Convert(err)
		return nil, errors.New(errStatus.Message())
	}
	return res, nil
}

func checkSchemaCmd() *cobra.Command {
	var host, comp, filePath, namespaceID string
	var req schemaregistry.CheckCompatibilityRequest

	cmd := &cobra.Command{
		Use:   "check",
		Short: "Check schema compatibility",
		Args:  cobra.ExactArgs(1),
		Example: heredoc.Doc(`
			$ schemaregistry schema check <schema-id> --namespace=<namespace-id> comp=<schema-compatibility> filePath=<schema-filePath>
	    	`),
		Annotations: map[string]string{
			"group:core": "true",
		},
		RunE: func(cmd *cobra.Command, args []string) error {
			spinner := printer.Spin("")
			defer spinner.Stop()

			fileData, err := ioutil.ReadFile(filePath)
			if err != nil {
				return err
			}
			req.Data = fileData

			conn, err := resolveConn(host)
			if err != nil {
				return err
			}
			defer conn.Close()
			schemaID := args[0]

			req.NamespaceId = namespaceID
			req.SchemaId = schemaID
			req.Compatibility = schemaregistry.Schema_Compatibility(schemaregistry.Schema_Compatibility_value[comp])

			client := schemaregistry.NewSchemaServiceClient(conn)
			_, err = client.CheckCompatibility(grpcContext(), &req)
			if err != nil {
				errStatus := status.Convert(err)
				return errors.New(errStatus.Message())
			}

			spinner.Stop()
			fmt.Println("schema is compatible")
			return nil
		},
	}

	cmd.Flags().StringVar(&host, "host", "", "schemaregistry host address eg: localhost:8000")
	checkErr(cmd.MarkFlagRequired("host"))

	cmd.Flags().StringVarP(&namespaceID, "namespace", "n", "", "parent namespace ID")
	checkErr(cmd.MarkFlagRequired("namespace"))

	cmd.Flags().StringVarP(&comp, "comp", "c", "", "schema compatibility")
	checkErr(cmd.MarkFlagRequired("comp"))

	cmd.Flags().StringVarP(&filePath, "filePath", "F", "", "path to the schema file")
	checkErr(cmd.MarkFlagRequired("filePath"))

	return cmd
}

func bulkCheckSchemaCmd() *cobra.Command {
	var host, comp, output, glob, namespaceID string

	cmd := &cobra.Command{
		Use:   "bulk-check",
		Short: "Check schema compatibility for each descriptor in glob",
		Example: heredoc.Doc(`
			$ schemaregistry schema bulk-check --namespace=<namespace-id> --comp=<schema-compatibility> --glob=<glob-patterb>
	    	`),
		Annotations: map[string]string{
			"group:core": "true",
		},

		RunE: func(cmd *cobra.Command, args []string) error {
			resbuf := bytes.NewBuffer(nil)
			fmt.Printf("run bulk check for %s\n", glob)
			matches, err := filepath.Glob(glob)
			if err != nil {
				return err
			}
			fmt.Printf("found: %v with glob: %s files", len(matches), glob)
			conn, err := resolveConn(host)
			if err != nil {
				return err
			}
			defer conn.Close()

			var errs util.Errors
			for _, file := range matches {
				if !strings.HasSuffix(filepath.Base(file), ".desc") {
					continue
				}
				schemaID := strings.TrimSuffix(filepath.Base(file), ".desc")
				resbuf.WriteString(fmt.Sprintf("\n%s\n", schemaID))
				err := doSchemaCheck(conn, file, schemaID, namespaceID, comp)
				if err != nil {
					resbuf.WriteString(fmt.Sprintf("	is incompatible: \n		ERROR:%v\n", err))
					errs = append(errs, xerrors.Errorf("schema failed: %s: %w", schemaID, err))
				} else {
					resbuf.WriteString("	compatible\n")
				}
				diffs := getDiff(conn, file, schemaID, namespaceID)
				if len(diffs) > 0 {
					resbuf.WriteString(fmt.Sprintf("	has %d diffs\n", len(diffs)))
					for _, d := range diffs {
						resbuf.WriteString(fmt.Sprintf("		%s\n", d))
					}
				} else {
					resbuf.WriteString("	has no diff\n")
				}
			}
			if output != "" {
				if err := ioutil.WriteFile(output, resbuf.Bytes(), 0600); err != nil {
					errs = append(errs, err)
				}
			} else {
				fmt.Print(resbuf.String())
			}
			if len(errs) > 0 {
				return xerrors.Errorf("found: %v\nerrs: %w", len(errs), errs)
			}
			return nil
		},
	}

	cmd.Flags().StringVar(&host, "host", "", "schemaregistry host address eg: localhost:8000")
	checkErr(cmd.MarkFlagRequired("host"))

	cmd.Flags().StringVarP(&namespaceID, "namespace", "n", "", "parent namespace ID")
	checkErr(cmd.MarkFlagRequired("namespace"))

	cmd.Flags().StringVarP(&comp, "comp", "c", "", "schema compatibility")
	checkErr(cmd.MarkFlagRequired("comp"))

	cmd.Flags().StringVarP(&glob, "glob", "F", "", "glob to find schema files")
	checkErr(cmd.MarkFlagRequired("glob"))
	cmd.Flags().StringVarP(&output, "output", "o", "", "path to report")

	return cmd
}

func getDiff(conn *grpc.ClientConn, file string, schemaID string, nsID string) []string {
	fileData, err := ioutil.ReadFile(file)
	if err != nil {
		fmt.Printf("unable to get diff: %v\n", err)
		return nil
	}

	var req schemaregistry.DiffRequest
	req.Data = fileData
	req.NamespaceId = nsID
	req.SchemaId = schemaID

	client := schemaregistry.NewSchemaServiceClient(conn)
	resp, err := client.Diff(grpcContext(), &req)
	if err != nil {
		fmt.Printf("unable to get diff: %v\n", err)
		return nil
	}
	return resp.GetDiffs()
}

func doSchemaCheck(conn *grpc.ClientConn, file string, schemaID string, namespaceID string, comp string) error {
	fileData, err := ioutil.ReadFile(file)
	if err != nil {
		return err
	}

	var req schemaregistry.CheckCompatibilityRequest
	req.Data = fileData
	req.NamespaceId = namespaceID
	req.SchemaId = schemaID
	req.Compatibility = schemaregistry.Schema_Compatibility(schemaregistry.Schema_Compatibility_value[comp])

	client := schemaregistry.NewSchemaServiceClient(conn)
	_, err = client.CheckCompatibility(grpcContext(), &req)
	if err != nil {
		errStatus := status.Convert(err)
		return errors.New(errStatus.Message())
	}
	return nil
}

func updateSchemaCmd() *cobra.Command {
	var host, comp, namespaceID string
	var req schemaregistry.UpdateSchemaMetadataRequest

	cmd := &cobra.Command{
		Use:   "edit",
		Short: "Edit a schema",
		Args:  cobra.ExactArgs(1),
		Example: heredoc.Doc(`
			$ schemaregistry schema edit <schema-id> --namespace=<namespace-id> --comp=<schema-compatibility>
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
			defer conn.Close()

			schemaID := args[0]

			req.NamespaceId = namespaceID
			req.SchemaId = schemaID
			req.Compatibility = schemaregistry.Schema_Compatibility(schemaregistry.Schema_Compatibility_value[comp])

			client := schemaregistry.NewSchemaServiceClient(conn)
			_, err = client.UpdateMetadata(grpcContext(), &req)
			if err != nil {
				return err
			}

			spinner.Stop()

			fmt.Printf("Schema successfully updated")
			return nil
		},
	}

	cmd.Flags().StringVar(&host, "host", "", "schemaregistry host address eg: localhost:8000")
	checkErr(cmd.MarkFlagRequired("host"))

	cmd.Flags().StringVarP(&namespaceID, "namespace", "n", "", "parent namespace ID")
	checkErr(cmd.MarkFlagRequired("namespace"))

	cmd.Flags().StringVarP(&comp, "comp", "c", "", "schema compatibility")
	checkErr(cmd.MarkFlagRequired("comp"))

	return cmd
}

func getSchemaCmd() *cobra.Command {
	var host, output, namespaceID string
	var version int32
	var metadata bool
	var data []byte
	var resMetadata *schemaregistry.GetSchemaMetadataResponse

	cmd := &cobra.Command{
		Use:   "view",
		Short: "View a schema",
		Args:  cobra.ExactArgs(1),
		Example: heredoc.Doc(`
			$ schemaregistry schema view <schema-id> --namespace=<namespace-id> --version <version> --metadata <metadata>
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
			defer conn.Close()

			schemaID := args[0]

			client := schemaregistry.NewSchemaServiceClient(conn)

			data, resMetadata, err = fetchSchemaAndMetadata(client, version, namespaceID, schemaID)
			if err != nil {
				return err
			}
			spinner.Stop()

			if output != "" {
				err = os.WriteFile(output, data, 0666)
				if err != nil {
					return err
				}
				fmt.Printf("Schema successfully written to %s\n", output)
			}

			if resMetadata == nil || !metadata {
				return nil
			}

			report := [][]string{}

			fmt.Printf("\nMETADATA\n")
			report = append(report, []string{"FORMAT", "COMPATIBILITY", "AUTHORITY"})

			report = append(report, []string{
				schemaregistry.Schema_Format_name[int32(resMetadata.GetFormat())],
				schemaregistry.Schema_Compatibility_name[int32(resMetadata.GetCompatibility())],
				resMetadata.GetAuthority(),
			})

			printer.Table(os.Stdout, report)

			return nil
		},
	}
	cmd.Flags().StringVar(&host, "host", "", "schemaregistry host address eg: localhost:8000")
	checkErr(cmd.MarkFlagRequired("host"))

	cmd.Flags().StringVarP(&namespaceID, "namespace", "n", "", "parent namespace ID")
	checkErr(cmd.MarkFlagRequired("namespace"))

	cmd.Flags().Int32VarP(&version, "version", "v", 0, "version of the schema")

	cmd.Flags().BoolVarP(&metadata, "metadata", "m", false, "set this flag to get metadata")
	checkErr(cmd.MarkFlagRequired("metadata"))

	cmd.Flags().StringVarP(&output, "output", "o", "", "path to the output file")

	return cmd
}

func deleteSchemaCmd() *cobra.Command {
	var host, namespaceID string
	var req schemaregistry.DeleteSchemaRequest
	var reqVer schemaregistry.DeleteVersionRequest
	var version int32

	cmd := &cobra.Command{
		Use:   "delete",
		Short: "Delete a schema",
		Args:  cobra.ExactArgs(1),
		Example: heredoc.Doc(`
			$ schemaregistry schema delete <schema-id> --namespace=<namespace-id>
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
			defer conn.Close()

			schemaID := args[0]

			client := schemaregistry.NewSchemaServiceClient(conn)

			if version == 0 {
				req.NamespaceId = namespaceID
				req.SchemaId = schemaID

				_, err = client.Delete(grpcContext(), &req)
				if err != nil {
					return err
				}
			} else {
				reqVer.NamespaceId = namespaceID
				reqVer.SchemaId = schemaID
				reqVer.VersionId = version

				_, err = schemaregistry.NewVersionServiceClient(conn).Delete(grpcContext(), &reqVer)
				if err != nil {
					return err
				}
			}

			spinner.Stop()

			fmt.Printf("schema successfully deleted")

			return nil
		},
	}

	cmd.Flags().StringVar(&host, "host", "", "schemaregistry host address eg: localhost:8000")
	checkErr(cmd.MarkFlagRequired("host"))

	cmd.Flags().StringVarP(&namespaceID, "namespace", "n", "", "parent namespace ID")
	checkErr(cmd.MarkFlagRequired("namespace"))

	cmd.Flags().Int32VarP(&version, "version", "v", 0, "particular version to be deleted")

	return cmd
}

func diffSchemaCmd() *cobra.Command {
	var fullname string
	var host string
	var namespace string
	var earlierVersion int32
	var laterVersion int32

	var schemaFetcher = func(req *schemaregistry.GetSchemaRequest, client schemaregistry.SchemaServiceClient) ([]byte, error) {
		res, err := client.Get(grpcContext(), req)
		if err != nil {
			return nil, err
		}
		return res.Data, nil
	}
	var protoSchemaFetcher = func(req *schemaregistry.GetSchemaRequest, client schemaregistry.SchemaServiceClient) ([]byte, error) {
		if fullname == "" {
			return nil, fmt.Errorf("fullname flag is mandator for FORMAT_PROTO")
		}
		res, err := client.Get(grpcContext(), req)
		if err != nil {
			return nil, err
		}
		fds := &descriptorpb.FileDescriptorSet{}
		if err := proto.Unmarshal(res.Data, fds); err != nil {
			return nil, fmt.Errorf("descriptor set file is not valid. %w", err)
		}
		files, err := protodesc.NewFiles(fds)
		if err != nil {
			return nil, fmt.Errorf("file is not fully contained descriptor file. hint: generate file descriptorset with --include_imports option. %w", err)
		}
		desc, err := files.FindDescriptorByName(protoreflect.FullName(fullname))
		if err != nil {
			return nil, fmt.Errorf("unable to find message. %w", err)
		}
		mDesc, ok := desc.(protoreflect.MessageDescriptor)
		if !ok {
			return nil, fmt.Errorf("not a message desc")
		}
		jsonByte, err := protojson.Marshal(protodesc.ToDescriptorProto(mDesc))
		if err != nil {
			return nil, fmt.Errorf("fail to convert json. %w", err)
		}
		return jsonByte, nil
	}

	cmd := &cobra.Command{
		Use:   "diff",
		Short: "Diff(s) of two schema versions",
		Args:  cobra.ExactArgs(1),
		Example: heredoc.Doc(`
		$ schemaregistry schema diff <schema-id> --namespace=<namespace-id> --later-version=<later-version> --earlier-version=<earlier-version> --fullname=<fullname>
		`),
		RunE: func(cmd *cobra.Command, args []string) error {
			spinner := printer.Spin("")
			defer spinner.Stop()

			schemaID := args[0]

			metaReq := schemaregistry.GetSchemaMetadataRequest{
				NamespaceId: namespace,
				SchemaId:    schemaID,
			}
			eReq := &schemaregistry.GetSchemaRequest{
				NamespaceId: namespace,
				SchemaId:    schemaID,
				VersionId:   earlierVersion,
			}
			lReq := &schemaregistry.GetSchemaRequest{
				NamespaceId: namespace,
				SchemaId:    schemaID,
				VersionId:   laterVersion,
			}

			conn, err := resolveConn(host)
			if err != nil {
				return err
			}
			defer conn.Close()
			client := schemaregistry.NewSchemaServiceClient(conn)

			meta, err := client.GetMetadata(grpcContext(), &metaReq)
			if err != nil {
				return err
			}

			var getSchema = schemaFetcher
			if meta.Format == *schemaregistry.Schema_FORMAT_PROTOBUF.Enum() {
				getSchema = protoSchemaFetcher
			}

			eJSON, err := getSchema(eReq, client)
			if err != nil {
				return err
			}

			lJSON, err := getSchema(lReq, client)
			if err != nil {
				return err
			}

			// TODO: FIXME
			//d, err := gojsondiff.New().Compare(eJSON, lJSON)
			//if err != nil {
			//	return err
			//}
			//
			//var placeholder map[string]interface{}
			//json.Unmarshal(eJSON, &placeholder)
			//config := formatter.AsciiFormatterConfig{
			//	ShowArrayIndex: true,
			//	Coloring:       true,
			//}
			//
			//formatter := formatter.NewAsciiFormatter(placeholder, config)
			//diffString, err := formatter.Format(d)
			//if err != nil {
			//	return err
			//}
			//
			//spinner.Stop()
			if string(lJSON) == string(eJSON) {
				fmt.Print("No diff!")
				return nil
			}
			//fmt.Print(diffString)

			return nil
		},
	}

	cmd.Flags().StringVar(&host, "host", "", "schemaregistry host address eg: localhost:8000")
	checkErr(cmd.MarkFlagRequired("host"))
	cmd.Flags().StringVarP(&namespace, "namespace", "n", "", "parent namespace ID")
	checkErr(cmd.MarkFlagRequired("namespace"))
	cmd.Flags().Int32Var(&earlierVersion, "earlier-version", 0, "earlier version of the schema")
	checkErr(cmd.MarkFlagRequired("earlier-version"))
	cmd.Flags().Int32Var(&laterVersion, "later-version", 0, "later version of the schema")
	checkErr(cmd.MarkFlagRequired("later-version"))
	cmd.Flags().StringVar(&fullname, "fullname", "", "only required for FORMAT_PROTO. fullname of proto schema eg: odpf.common.v1.Version")
	return cmd
}

func versionSchemaCmd() *cobra.Command {
	var host, namespaceID string
	var req schemaregistry.ListVersionsRequest

	cmd := &cobra.Command{
		Use:   "version",
		Short: "Version(s) of a schema",
		Args:  cobra.ExactArgs(1),
		Example: heredoc.Doc(`
			$ schemaregistry schema version <schema-id> --namespace=<namespace-id>
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
			defer conn.Close()

			schemaID := args[0]

			req.NamespaceId = namespaceID
			req.SchemaId = schemaID

			client := schemaregistry.NewVersionServiceClient(conn)
			res, err := client.List(grpcContext(), &req)
			if err != nil {
				return err
			}

			report := [][]string{}
			versions := res.GetVersions()

			spinner.Stop()

			if len(versions) == 0 {
				fmt.Printf("%s has no versions in %s", schemaID, namespaceID)
				return nil
			}

			report = append(report, []string{"VERSIONS(s)"})

			for _, v := range versions {
				report = append(report, []string{
					strconv.FormatInt(int64(v), 10),
				})
			}
			printer.Table(os.Stdout, report)
			return nil
		},
	}

	cmd.Flags().StringVar(&host, "host", "", "schemaregistry host address eg: localhost:8000")
	checkErr(cmd.MarkFlagRequired("host"))

	cmd.Flags().StringVarP(&namespaceID, "namespace", "n", "", "parent namespace ID")
	checkErr(cmd.MarkFlagRequired("namespace"))

	return cmd
}

func printCmd() *cobra.Command {
	var output, filterPathPrefix, host, namespaceID, schemaID string
	var version int32

	cmd := &cobra.Command{
		Use:   "print",
		Short: "Prints snapshot details into .proto files",
		Args:  cobra.ExactArgs(1),
		Example: heredoc.Doc(`
			$ schemaregistry schema print <schema-id> --namespace=<namespace-id> --version <version> --output=<output-path> --filter-path=<path-prefix>
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
			client := schemaregistry.NewSchemaServiceClient(conn)

			schemaID := args[0]

			data, resMetadata, err := fetchSchemaAndMetadata(client, version, namespaceID, schemaID)
			if err != nil {
				return err
			}

			format := schemaregistry.Schema_Format_name[int32(resMetadata.GetFormat())]

			if format == "FORMAT_AVRO" || format == "FORMAT_JSON" {
				if output == "" {
					fmt.Printf("\n// ----\n// SCHEMA\n// ----\n\n")
					_, err := os.Stdout.Write(data)
					if err != nil {
						return fmt.Errorf("schema is not valid. %w", err)
					}
				} else {
					err = os.WriteFile(output, data, 0666)
					if err != nil {
						return err
					}

					fmt.Printf("Schema successfully written to %s\n", output)
				}
			} else {
				fds := &descriptorpb.FileDescriptorSet{}
				if err := proto.Unmarshal(data, fds); err != nil {
					return fmt.Errorf("descriptor set file is not valid. %w", err)
				}
				fdsMap, err := desc.CreateFileDescriptorsFromSet(fds)
				if err != nil {
					return err
				}

				var filteredFds []*desc.FileDescriptor
				for fdName, fd := range fdsMap {
					if filterPathPrefix != "" && strings.HasPrefix(fdName, filterPathPrefix) {
						continue
					}
					filteredFds = append(filteredFds, fd)
				}

				protoPrinter := &protoprint.Printer{}

				if output == "" {
					for _, fd := range filteredFds {
						protoAsString, err := protoPrinter.PrintProtoToString(fd)
						if err != nil {
							return err
						}
						fmt.Printf("\n// ----\n// %s\n// ----\n%s", fd.GetName(), protoAsString)
					}
				} else {
					if err := protoPrinter.PrintProtosToFileSystem(filteredFds, output); err != nil {
						return err
					}
				}
			}
			return nil
		},
	}

	cmd.Flags().StringVar(&host, "host", "", "schemaregistry host address eg: localhost:8000")
	checkErr(cmd.MarkFlagRequired("host"))

	cmd.Flags().StringVarP(&namespaceID, "namespace", "n", "", "provide namespace/group or entity name")
	checkErr(cmd.MarkFlagRequired("namespace"))

	cmd.Flags().StringVarP(&schemaID, "schema", "s", "", "provide proto repo name")
	//checkErr(cmd.MarkFlagRequired("name"))

	cmd.Flags().Int32VarP(&version, "version", "v", 0, "provide version number")

	cmd.Flags().StringVarP(&output, "output", "o", "", "the directory path to write the descriptor files, default is to print on stdout")

	cmd.Flags().StringVar(&filterPathPrefix, "filter-path", "", "filter protocol buffer files by path prefix, e.g., --filter-path=google/protobuf")

	return cmd
}

func graphCmd() *cobra.Command {
	var host, output, namespaceID string
	var version int32

	cmd := &cobra.Command{
		Use:     "graph",
		Aliases: []string{"g"},
		Short:   "Generate file descriptorset dependencies graph",
		Args:    cobra.ExactArgs(1),
		Example: heredoc.Doc(`
			$ schemaregistry schema graph <schema-id> --namespace=<namespace-id> --version=<version> --output=<output-path>
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

			client := schemaregistry.NewSchemaServiceClient(conn)

			schemaID := args[0]

			data, resMetadata, err := fetchSchemaAndMetadata(client, version, namespaceID, schemaID)
			if err != nil {
				return err
			}

			format := schemaregistry.Schema_Format_name[int32(resMetadata.GetFormat())]
			if format != "FORMAT_PROTOBUF" {
				fmt.Printf("cannot create graph for %s", format)
				return nil
			}

			msg := &descriptorpb.FileDescriptorSet{}
			err = proto.Unmarshal(data, msg)
			if err != nil {
				return fmt.Errorf("invalid file descriptorset file. %w", err)
			}

			// TODO: FIXME
			//graph, err := graph.GetProtoFileDependencyGraph(msg)
			//if err != nil {
			//	return err
			//}
			//if err = os.WriteFile(output, []byte(graph.String()), 0666); err != nil {
			//	return err
			//}
			//
			//fmt.Println(".dot file has been created in", output)
			return nil
		},
	}

	cmd.Flags().StringVar(&host, "host", "", "schemaregistry host address eg: localhost:8000")
	checkErr(cmd.MarkFlagRequired("host"))

	cmd.Flags().StringVarP(&namespaceID, "namespace", "n", "", "provide namespace/group or entity name")
	checkErr(cmd.MarkFlagRequired("namespace"))

	cmd.Flags().Int32VarP(&version, "version", "v", 0, "provide version number")

	cmd.Flags().StringVarP(&output, "output", "o", "./proto_vis.dot", "write to .dot file")

	return cmd
}

func fetchSchemaAndMetadata(client schemaregistry.SchemaServiceClient, version int32, namespaceID, schemaID string) ([]byte, *schemaregistry.GetSchemaMetadataResponse, error) {
	var req schemaregistry.GetSchemaRequest
	var reqLatest schemaregistry.GetLatestSchemaRequest
	var reqMetadata schemaregistry.GetSchemaMetadataRequest
	var data []byte

	if version != 0 {
		req.NamespaceId = namespaceID
		req.SchemaId = schemaID
		req.VersionId = version
		res, err := client.Get(grpcContext(), &req)
		if err != nil {
			return nil, nil, err
		}
		data = res.GetData()
	} else {
		reqLatest.NamespaceId = namespaceID
		reqLatest.SchemaId = schemaID
		res, err := client.GetLatest(grpcContext(), &reqLatest)
		if err != nil {
			return nil, nil, err
		}
		data = res.GetData()
	}

	reqMetadata.NamespaceId = namespaceID
	reqMetadata.SchemaId = schemaID
	resMetadata, err := client.GetMetadata(grpcContext(), &reqMetadata)
	if err != nil {
		return data, nil, err
	}

	return data, resMetadata, nil
}
