package dockerprocess

import (
	"bytes"
	"context"
	"crypto/rand"
	"encoding/binary"
	"sync"
	"testing"
	"time"

	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/misc"

	"cuelang.org/go/pkg/strconv"

	"github.com/docker/docker/api/types"
	"github.com/stretchr/testify/require"

	"go.uber.org/zap"

	"go.uber.org/zap/zaptest"

	log "a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"

	"github.com/stretchr/testify/assert"
)

func TestDockerConfig_Parse(t *testing.T) {
	rt := require.New(t)
	cfg := DockerConfig{SecurityOptions: []string{"no-new-privileges", "apparmor=qemu-nbd"}}
	err := cfg.Parse()
	rt.NoError(err)
	rt.True(cfg.securityOptionsHasParsed)
	rt.Equal([]string{"no-new-privileges", "apparmor=qemu-nbd"}, cfg.securityOptionsParsed)
}

func TestDockerExec(t *testing.T) {
	at := assert.New(t)
	ctx, ctxCancel := context.WithTimeout(context.Background(), time.Minute)
	defer ctxCancel()

	ctx = log.WithLogger(ctx, zaptest.NewLogger(t).WithOptions(zap.Development()))

	res, err := Exec(ctx, []string{"echo", "-n", "123"}, nil)
	at.NoError(err)
	at.EqualValues([]byte("123"), res)
}

func TestDockerExecSecurityOptions(t *testing.T) {
	oldDockerConfig := dockerConfig
	defer func() {
		dockerConfig = oldDockerConfig
	}()

	dockerConfig.securityOptionsParsed = []string{"no-new-privileges"}

	rt := require.New(t)
	testTimeout := time.Second * 10
	ctx, ctxCancel := context.WithTimeout(context.Background(), testTimeout)
	defer ctxCancel()

	logger := zaptest.NewLogger(t).WithOptions(zap.Development())
	ctx = log.WithLogger(ctx, logger)

	var keyInt int64
	for {
		err := binary.Read(rand.Reader, binary.LittleEndian, &keyInt)
		rt.NoError(err)
		if keyInt > int64(testTimeout.Seconds()) {
			break
		}
	}
	key := strconv.FormatInt(keyInt, 10)
	logger.Debug("Generated key", zap.String("key", key))

	var wg sync.WaitGroup
	execCtx, execCtxCancel := context.WithCancel(ctx)
	wg.Add(1)
	go func() {
		defer wg.Done()

		_, execErr := Exec(execCtx, []string{"sleep", key}, nil)
		if execCtx.Err() == nil {
			rt.NoError(execErr)
		}
	}()

	dockerClient, err := newDockerClient()
	rt.NoError(err)
	defer func() {
		err := dockerClient.Close()
		rt.NoError(err)
	}()
	var containerID string
	err = misc.RetryExtended(execCtx, "readContainer",
		misc.RetryParams{MaxRetryDelay: time.Second, RetryTimeout: testTimeout},
		func() error {
			containersList, err := dockerClient.ContainerList(execCtx, types.ContainerListOptions{All: true})
			rt.NoError(err)
			for _, container := range containersList {
				containerJSON, _ := dockerClient.ContainerInspect(execCtx, container.ID)
				logger.Debug("container", zap.Any("container", containerJSON))
				for _, word := range containerJSON.Config.Cmd {
					if word == key {
						containerID = container.ID
						return nil
					}
				}
			}
			return misc.ErrInternalRetry
		})
	rt.NoError(err)
	rt.NotEqual("", containerID)
	containerJSON, err := dockerClient.ContainerInspect(execCtx, containerID)
	rt.NoError(err)
	hasNoNewPrivileges := false
	for _, opt := range containerJSON.HostConfig.SecurityOpt {
		if opt == "no-new-privileges" {
			hasNoNewPrivileges = true
			break
		}
	}
	rt.True(hasNoNewPrivileges)
	execCtxCancel()
	wg.Wait()
}

func TestShortenMessage(t *testing.T) {
	at := assert.New(t)
	at.Panics(func() {
		shortenMessage([]byte("asd"), -1)
	})

	testTable := []struct {
		mess   string
		maxLen int
		result string
	}{
		{"", 0, ""},
		{"1234567890abcdefghij", 20, "1234567890abcdefghij"},
		{"1234567890abcdefghij", 3, "123"},
		{"1234567890abcdefghijk", 20, "1234567 ... defghijk"},
		{"1234567890abcdefghijk", 19, "1234567 ... efghijk"},
	}

	for index, test := range testTable {
		if result := shortenMessage([]byte(test.mess), test.maxLen); !bytes.Equal(result, []byte(test.result)) {
			t.Errorf("test[%v]: '%#v', result: '%s'", index, test, result)
		}
	}
}
