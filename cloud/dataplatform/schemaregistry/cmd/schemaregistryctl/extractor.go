package main

import (
	"fmt"
	"io/ioutil"
	"os"

	"github.com/golang/protobuf/descriptor"
	"github.com/jhump/protoreflect/desc"
	"github.com/spf13/cobra"
	"google.golang.org/protobuf/proto"
	"google.golang.org/protobuf/reflect/protodesc"
	"google.golang.org/protobuf/reflect/protoreflect"
	"google.golang.org/protobuf/types/descriptorpb"
	"google.golang.org/protobuf/types/dynamicpb"

	"a.yandex-team.ru/cloud/dataplatform/api/schemaregistry/options"
	"a.yandex-team.ru/cloud/dataplatform/internal/heredoc"
)

func ExtractCmd() *cobra.Command {
	var fdsPath, outputPath string
	var wellKnownLibs []string

	cmd := &cobra.Command{
		Use:     "extract",
		Aliases: []string{"e"},
		Short:   "Extract tagged descriptors from FDS file",
		Long:    "Work with namespaces.",
		Example: heredoc.Doc(`
			$ schemaregistry namespace list
		`),
		Annotations: map[string]string{
			"group:core": "true",
		},
		RunE: func(cmd *cobra.Command, args []string) error {
			return loadProtodesc(fdsPath, outputPath, wellKnownLibs)
		},
	}

	cmd.Flags().StringVar(&fdsPath, "input", "", "Input fds file")
	cmd.Flags().StringVar(&outputPath, "output", "", "Output folder")
	cmd.Flags().StringArrayVarP(&wellKnownLibs, "well-known-libs", "", nil, "List of files that well known by schema registry")

	return cmd
}

func loadProtodesc(descFilePath, outPath string, wellKnownLibs []string) error {
	data, err := ioutil.ReadFile(descFilePath)
	if err != nil {
		return err
	}
	var fds descriptorpb.FileDescriptorSet
	if err := proto.Unmarshal(data, &fds); err != nil {
		return err
	}
	registry, err := protodesc.NewFiles(&fds)
	if err != nil {
		return err
	}
	fmt.Printf("descriptor load: %v files\n", registry.NumFiles())
	fileRegistry, err := desc.CreateFileDescriptors(fds.File)
	if err != nil {
		return err
	}
	for _, file := range fds.File {
		descr, err := registry.FindFileByPath(file.GetName())
		if err != nil {
			return err
		}
		for i := 0; i < descr.Messages().Len(); i++ {
			messageDescriptor := descr.Messages().Get(i)
			extenstion := proto.GetExtension(messageDescriptor.Options(), options.E_Schemaregistry)
			if extenstion == nil {
				continue
			}
			schemaExtension, ok := extenstion.(*options.Options)
			if !ok {
				continue
			}
			if schemaExtension == nil || !schemaExtension.Enabled {
				continue
			}
			fmt.Printf("%s generate descriptor \n", messageDescriptor.FullName())
			if err := genDescForMessage(outPath, messageDescriptor, fileRegistry, wellKnownLibs); err != nil {
				return err
			}
		}
	}
	return nil
}

func genDescForMessage(path string, message protoreflect.MessageDescriptor, reg map[string]*desc.FileDescriptor, wellKnownLibs []string) error {
	messageDescriptorProto, _ := descriptor.MessageDescriptorProto(dynamicpb.NewMessage(message))
	deps := loadDependencies(reg, messageDescriptorProto)
	fileDescriptors, err := desc.CreateFileDescriptors(deps)
	if err != nil {
		return err
	}
	var res []*desc.FileDescriptor
	for _, f := range fileDescriptors {
		res = append(res, f)
	}
	fd, err := desc.CreateFileDescriptor(messageDescriptorProto, res...)
	if err != nil {
		return err
	}
	ffds := desc.ToFileDescriptorSet(fd)
	var filteredFds []*descriptorpb.FileDescriptorProto
	for _, fd := range ffds.File {
		for _, path := range wellKnownLibs {
			if fd.GetName() == path {
				continue
			}
		}
		filteredFds = append(filteredFds, fd)
	}
	ffds.File = filteredFds
	data, err := proto.Marshal(ffds)
	if err != nil {
		return err
	}
	if err := os.Mkdir(path, 0777); err != nil {
		if !os.IsExist(err) {
			return err
		}
	}
	resFile := fmt.Sprintf("%s/%s.desc", path, message.FullName())
	if err := ioutil.WriteFile(resFile, data, 0600); err != nil {
		return err
	}
	return nil
}

func loadDependencies(reg map[string]*desc.FileDescriptor, mdesc *descriptorpb.FileDescriptorProto) []*descriptorpb.FileDescriptorProto {
	var res []*descriptorpb.FileDescriptorProto
	for _, dep := range mdesc.Dependency {
		fd, ok := reg[dep]
		if !ok {
			panic("missed dep: " + dep)
		}
		fdsproto := fd.AsFileDescriptorProto()
		if len(fdsproto.Dependency) > 0 {
			res = append(res, loadDependencies(reg, fdsproto)...)
		}
		res = append(res, fdsproto)
	}
	return res
}
