package pssh

import (
	"context"
	"fmt"

	nbs "a.yandex-team.ru/cloud/blockstore/public/sdk/go/client"
	logutil "a.yandex-team.ru/cloud/storage/core/tools/common/go/log"
)

////////////////////////////////////////////////////////////////////////////////

func makeError(err error) error {
	return fmt.Errorf("pssh fatal error - max attempt count exceeded. %w", err)
}

////////////////////////////////////////////////////////////////////////////////

type durablePssh struct {
	logutil.WithLog

	impl        PsshIface
	maxAttempts int
}

func (c *durablePssh) Run(
	ctx context.Context,
	cmd string,
	target string,
) ([]string, error) {

	var err error
	var result []string

	for attempts := c.maxAttempts; attempts > 0; attempts-- {
		result, err = c.impl.Run(ctx, cmd, target)
		if err == nil {
			return result, nil
		}

		c.LogWarn(ctx, "failed to run command '%v' on host %v: %v", cmd, target, err)
	}

	return nil, makeError(err)
}

func (c *durablePssh) CopyFile(
	ctx context.Context,
	source string,
	target string,
) error {

	var err error

	for attempts := c.maxAttempts; attempts > 0; attempts-- {
		err = c.impl.CopyFile(ctx, source, target)
		if err == nil {
			return nil
		}

		c.LogWarn(ctx, "failed to copy '%v' to host %v: %v", source, target, err)
	}

	return makeError(err)
}

////////////////////////////////////////////////////////////////////////////////

func NewDurable(log nbs.Log, impl PsshIface, maxAttempts int) PsshIface {
	return &durablePssh{
		WithLog: logutil.WithLog{
			Log: log,
		},
		impl:        impl,
		maxAttempts: maxAttempts,
	}
}
