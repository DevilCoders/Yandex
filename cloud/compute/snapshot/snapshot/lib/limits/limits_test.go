package limits

import (
	"context"
	"runtime"
	"testing"

	log "a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"
	"go.uber.org/zap"

	"github.com/stretchr/testify/assert"
)

func TestPrLimit(t *testing.T) {
	var resCmd string
	var resArgs []string

	ctx := log.WithLogger(context.Background(), zap.NewNop())

	resCmd, resArgs = prlimit(ctx, PrlimitParams{}, "")
	assert.Equal(t, "prlimit", resCmd)
	assert.Equal(t, []string{""}, resArgs)

	resCmd, resArgs = prlimit(ctx, PrlimitParams{}, "cmd", "test1", "test2")
	assert.Equal(t, "prlimit", resCmd)
	assert.Equal(t, []string{"cmd", "test1", "test2"}, resArgs)

	resCmd, resArgs = prlimit(ctx, PrlimitParams{QemuCPUTime: 123}, "cmd", "test1", "test2")
	assert.Equal(t, "prlimit", resCmd)
	assert.Equal(t, []string{"--cpu=123", "cmd", "test1", "test2"}, resArgs)

	resCmd, resArgs = prlimit(ctx, PrlimitParams{QemuAddressSpace: 222}, "cmd", "test1", "test2")
	assert.Equal(t, "prlimit", resCmd)
	assert.Equal(t, []string{"--as=222", "cmd", "test1", "test2"}, resArgs)

	resCmd, resArgs = prlimit(ctx, PrlimitParams{QemuCPUTime: 22, QemuAddressSpace: 11}, "cmd", "test1", "test2")
	assert.Equal(t, "prlimit", resCmd)
	assert.Equal(t, []string{"--cpu=22", "--as=11", "cmd", "test1", "test2"}, resArgs)

}

func TestPrlimitPublic(t *testing.T) {
	ctx := log.WithLogger(context.Background(), zap.NewNop())
	if runtime.GOOS == "darwin" || runtime.GOOS == "windows" {
		resCmdPublic, resArgsPublic := Prlimit(ctx, PrlimitParams{QemuCPUTime: 22, QemuAddressSpace: 11}, "ddd", "aaa", "ccc")
		assert.Equal(t, "ddd", resCmdPublic)
		assert.Equal(t, []string{"aaa", "ccc"}, resArgsPublic)
	} else {
		resCmdPublic, resArgsPublic := Prlimit(ctx, PrlimitParams{QemuCPUTime: 22, QemuAddressSpace: 11}, "ddd", "aaa", "ccc")
		resCmdPrivate, resArgsPrivate := prlimit(ctx, PrlimitParams{QemuCPUTime: 22, QemuAddressSpace: 11}, "ddd", "aaa", "ccc")
		assert.Equal(t, resCmdPrivate, resCmdPublic)
		assert.Equal(t, resArgsPrivate, resArgsPublic)

	}
}
