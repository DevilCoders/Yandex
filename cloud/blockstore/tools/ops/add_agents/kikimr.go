package main

import (
	"context"
	"fmt"
	"strings"

	"a.yandex-team.ru/cloud/storage/core/tools/common/go/pssh"

	nbs "a.yandex-team.ru/cloud/blockstore/public/sdk/go/client"
	logutil "a.yandex-team.ru/cloud/storage/core/tools/common/go/log"
)

////////////////////////////////////////////////////////////////////////////////

type kikimrClientIface interface {
	SetUserAttribute(
		ctx context.Context,
		name string,
		value string,
	) error

	GetUserAttribute(
		ctx context.Context,
		name string,
	) (string, error)
}

////////////////////////////////////////////////////////////////////////////////

type kikimrClient struct {
	logutil.WithLog

	host string
	db   string

	pssh pssh.PsshIface
}

func (kikimr *kikimrClient) SetUserAttribute(
	ctx context.Context,
	name string,
	value string,
) error {

	// TODO: token
	lines, err := kikimr.pssh.Run(
		ctx,
		fmt.Sprintf(
			"kikimr db schema user-attribute set %v %v=%v",
			kikimr.db,
			name,
			value,
		),
		kikimr.host,
	)
	if err != nil {
		return fmt.Errorf("can't set user attribute %v: %w", name, err)
	}

	if len(lines) != 0 {
		return fmt.Errorf("can't set user attribute %v. Unexpected output: %v", name, lines)
	}

	return nil
}

func (kikimr *kikimrClient) GetUserAttribute(
	ctx context.Context,
	name string,
) (string, error) {

	lines, err := kikimr.pssh.Run(
		ctx,
		fmt.Sprintf(
			"kikimr db schema user-attribute get %v",
			kikimr.db,
		),
		kikimr.host,
	)
	if err != nil {
		return "", fmt.Errorf("can't get user attribute %v: %w", name, err)
	}

	prefix := name + ": "
	for _, line := range lines {
		if strings.HasPrefix(line, prefix) {
			return line[len(prefix):], nil
		}
	}

	return "", fmt.Errorf("can't get user attribute %v. Not found in output: %v", name, lines)
}

////////////////////////////////////////////////////////////////////////////////

func newKikimrClient(
	log nbs.Log,
	host string,
	db string,
	pssh pssh.PsshIface,
) kikimrClientIface {

	return &kikimrClient{
		WithLog: logutil.WithLog{Log: log},
		host:    host,
		db:      db,
		pssh:    pssh,
	}
}
