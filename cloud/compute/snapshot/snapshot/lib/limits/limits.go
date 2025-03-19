package limits

import (
	"context"
	"runtime"
	"strconv"

	log "a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"
	"go.uber.org/zap"
)

// PrlimitParams is paremeters for tune restrictions by prlimit command
type PrlimitParams struct {
	QemuAddressSpace int
	QemuCPUTime      int
}

// Prlimit run command with prlimit for linux and ignore for macos and windows (developer computers).
func Prlimit(ctx context.Context, limits PrlimitParams, cmd string, args ...string) (resCmd string, resArgs []string) {
	if runtime.GOOS == "linux" {
		return prlimit(ctx, limits, cmd, args...)
	}
	log.G(ctx).Warn("Supress prlimit for work in developer env", zap.Any("limits", limits),
		zap.String("cmd", cmd), zap.Strings("args", args))
	return cmd, args
}

func prlimit(ctx context.Context, limits PrlimitParams, cmd string, args ...string) (resCmd string, resArgs []string) {
	var prLimitArgs []string
	if limits.QemuCPUTime != 0 {
		prLimitArgs = append(prLimitArgs, "--cpu="+strconv.Itoa(limits.QemuCPUTime))
	}
	if limits.QemuAddressSpace != 0 {
		prLimitArgs = append(prLimitArgs, "--as="+strconv.Itoa(limits.QemuAddressSpace))
	}

	resArgs = append(prLimitArgs, cmd)
	resArgs = append(resArgs, args...)

	log.G(ctx).Debug("Add prlimit to command.", zap.Strings("append_prlimit_args", prLimitArgs),
		zap.Strings("result_prlimit_args", resArgs))

	return "prlimit", resArgs
}
