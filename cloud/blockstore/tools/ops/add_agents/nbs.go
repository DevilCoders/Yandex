package main

import (
	"context"
	"fmt"
	"io/ioutil"
	"path"
	"strings"

	"a.yandex-team.ru/cloud/blockstore/public/api/protos"
	nbs "a.yandex-team.ru/cloud/blockstore/public/sdk/go/client"
	logutil "a.yandex-team.ru/cloud/storage/core/tools/common/go/log"
	"a.yandex-team.ru/cloud/storage/core/tools/common/go/pssh"

	"github.com/golang/protobuf/proto"
)

////////////////////////////////////////////////////////////////////////////////

type nbsClientIface interface {
	DescribeDiskRegistryConfig(ctx context.Context) (
		*protos.TUpdateDiskRegistryConfigRequest,
		error,
	)

	UpdateDiskRegistryConfig(
		ctx context.Context,
		dr *protos.TUpdateDiskRegistryConfigRequest,
	) error
}

////////////////////////////////////////////////////////////////////////////////

type nbsClient struct {
	logutil.WithLog

	host     string
	userName string
	tmpDir   string
	token    string

	pssh pssh.PsshIface
}

func (client *nbsClient) DescribeDiskRegistryConfig(ctx context.Context) (
	*protos.TUpdateDiskRegistryConfigRequest,
	error,
) {
	var blockstoreClientCmd = "blockstore-client describediskregistryconfig --verbose error"

	var commands []string
	if len(client.token) != 0 {
		commands = append(
			commands,
			fmt.Sprintf("echo '%v' > token", client.token),
			blockstoreClientCmd+" --secure-port 9768 --iam-token-file token",
		)
	} else {
		commands = append(commands, blockstoreClientCmd)
	}

	lines, err := client.pssh.Run(
		ctx,
		strings.Join(commands, " && "),
		client.host,
	)
	if err != nil {
		return nil, fmt.Errorf("can't load DR config. %v. Error: %w", lines, err)
	}

	if len(lines) < 1 {
		return nil, fmt.Errorf("unexpected DR config: %v", lines)
	}

	data := strings.Join(lines[1:], "\n")

	dr := &protos.TUpdateDiskRegistryConfigRequest{}
	err = proto.UnmarshalText(data, dr)
	if err != nil {
		return nil, fmt.Errorf("can't unmarshal DR config: %w", err)
	}

	return dr, nil
}

func (client *nbsClient) UpdateDiskRegistryConfig(
	ctx context.Context,
	dr *protos.TUpdateDiskRegistryConfigRequest,
) error {

	file, err := ioutil.TempFile(client.tmpDir, "dr_config-")
	if err != nil {
		return fmt.Errorf("can't create temp file: %w", err)
	}
	defer file.Close()

	err = proto.MarshalText(file, dr)
	if err != nil {
		return fmt.Errorf("can't save config to file: %w", err)
	}
	_ = file.Close()

	err = client.pssh.CopyFile(
		ctx,
		file.Name(),
		fmt.Sprintf("%v:/home/%v/", client.host, client.userName),
	)
	if err != nil {
		return fmt.Errorf("can't copy config to remote host: %w", err)
	}

	var blockstoreClientCmd = fmt.Sprintf(
		"blockstore-client updatediskregistryconfig --input %v --verbose error --proto",
		path.Base(file.Name()),
	)

	var commands []string
	if len(client.token) != 0 {
		commands = append(
			commands,
			fmt.Sprintf("echo '%v' > token", client.token),
			blockstoreClientCmd+" --secure-port 9768 --iam-token-file token",
		)
	} else {
		commands = append(commands, blockstoreClientCmd)
	}

	lines, err := client.pssh.Run(
		ctx,
		strings.Join(commands, " && "),
		client.host,
	)
	if err != nil {
		return fmt.Errorf("can't execute 'updatediskregistryconfig' on remote host: %w", err)
	}

	if len(lines) != 0 {
		return fmt.Errorf("unexpected output: %v", lines)
	}

	return nil
}

////////////////////////////////////////////////////////////////////////////////

func newNBS(
	log nbs.Log,
	host string,
	userName string,
	tmpDir string,
	token string,
	pssh pssh.PsshIface,
) nbsClientIface {

	return &nbsClient{
		WithLog:  logutil.WithLog{Log: log},
		host:     host,
		userName: userName,
		tmpDir:   tmpDir,
		token:    token,
		pssh:     pssh,
	}
}
