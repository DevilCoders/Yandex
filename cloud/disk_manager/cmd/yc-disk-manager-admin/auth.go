package main

import (
	"context"
	"fmt"
	"io/ioutil"
	"os/user"
	"path/filepath"
	"strings"

	"google.golang.org/grpc/metadata"

	client_config "a.yandex-team.ru/cloud/disk_manager/internal/pkg/configs/client/config"
)

////////////////////////////////////////////////////////////////////////////////

func getTokenFilename(
	config *client_config.ClientConfig,
) (string, error) {

	filename := config.GetIAMTokenFile()
	if strings.HasPrefix(filename, "~/") {
		u, err := user.Current()
		if err != nil {
			return "", err
		}
		filename = filepath.Join(u.HomeDir, filename[2:])
	}

	return filename, nil
}

////////////////////////////////////////////////////////////////////////////////

func getIAMToken(
	config *client_config.ClientConfig,
) (string, error) {

	filename, err := getTokenFilename(config)
	if err != nil {
		return "", err
	}

	bytes, err := ioutil.ReadFile(filename)
	if err != nil {
		return "", err
	}

	token := string(bytes)
	return strings.Trim(token, "\n\t "), nil
}

////////////////////////////////////////////////////////////////////////////////

func addAuthHeader(
	ctx context.Context,
	config *client_config.ClientConfig,
) (context.Context, error) {

	token, err := getIAMToken(config)
	if err != nil {
		return nil, fmt.Errorf("failed to get IAM token: %w", err)
	}

	return metadata.NewOutgoingContext(
		ctx,
		metadata.Pairs("authorization", fmt.Sprintf("Bearer %v", token)),
	), nil
}
