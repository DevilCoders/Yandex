package dockerprocess

import (
	"bytes"
	"context"
	"io"
	"io/ioutil"
	"os"
	"strconv"
	"time"

	"github.com/docker/docker/api/types"
	"github.com/docker/docker/api/types/container"
	"github.com/docker/docker/api/types/filters"
	"github.com/docker/docker/api/types/mount"
	"github.com/docker/docker/client"
	"go.uber.org/zap"
	"golang.org/x/xerrors"

	log "a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/misc"
)

const (
	ProxySocketInContainer = "/connect/image-proxy"

	nonFloodPause          = time.Second
	stopContainerTimeout   = time.Second
	dockerLabel            = "a.yandex-team.ru/cloud/compute/snapshot/qemu"
	removeContainerTimeout = time.Second * 5
	maxStdoutSize          = misc.GB * 1
)

type DockerProcess struct {
	docker          *client.Client
	mounts          map[string]string // mounts[host-path]container-path
	id              string
	containerConfig container.Config
	hostConfig      container.HostConfig
}

var dockerConfig DockerConfig

type DockerConfig struct {
	DockerImage              string
	DockerMemoryMB           int64
	DockerDiskMB             int64
	DockerMaxPidCount        int64
	DockerCPUQuotaPercent    float64
	SecurityOptions          []string
	securityOptionsHasParsed bool
	securityOptionsParsed    []string
}

func (c *DockerConfig) Parse() error {
	securityOpts, err := parseSecurityOpts(c.SecurityOptions)
	if err != nil {
		return xerrors.Errorf("parse security options of docker config: %w", err)
	}
	securityOpts, _, _ = parseSystemPaths(securityOpts)
	c.securityOptionsHasParsed = true
	c.securityOptionsParsed = securityOpts
	return nil
}

func init() {
	dockerConfig.fillDockerConfigDefaults()
}

func SetDockerConfig(c DockerConfig) {
	c.fillDockerConfigDefaults()

	dockerConfig = c
}

func (c *DockerConfig) fillDockerConfigDefaults() {
	if c.DockerImage == "" {
		c.DockerImage = "snapshot_qemu_nbd"
	}
	if c.DockerMaxPidCount == 0 {
		c.DockerMaxPidCount = 1000
	}
	if c.DockerCPUQuotaPercent == 0 {
		c.DockerCPUQuotaPercent = 100
	}
}

func (c *DockerConfig) Validate() error {
	if !c.securityOptionsHasParsed {
		return xerrors.New("docker security options doesn't parsed")
	}

	return nil
}

func (dp *DockerProcess) Start(ctx context.Context) error {
	if len(dp.hostConfig.Mounts) == 0 && len(dp.mounts) != 0 {
		mounts := make([]mount.Mount, 0, len(dp.mounts))
		for host, container := range dp.mounts {
			mounts = append(mounts, mount.Mount{Source: host, Target: container, Type: mount.TypeBind})
		}
		dp.hostConfig.Mounts = mounts
	}

	createResp, err := dp.docker.ContainerCreate(ctx, &dp.containerConfig, &dp.hostConfig,
		nil, "")

	log.DebugErrorCtx(ctx, err, "Create docker container", zap.String("docker_container_id", createResp.ID),
		zap.Strings("warnings", createResp.Warnings), zap.Strings("command", dp.containerConfig.Cmd),
		zap.String("image", dockerConfig.DockerImage), zap.String("label", dockerLabel), zap.Any("mounts", dp.mounts),
		zap.String("user", dp.containerConfig.User),
	)
	if err != nil {
		return xerrors.Errorf("create docker container error: %w", err)
	}
	dp.id = createResp.ID
	err = dp.docker.ContainerStart(ctx, dp.id, types.ContainerStartOptions{})
	log.DebugErrorCtx(ctx, err, "Start docker container", zap.String("docker_container_id", dp.id))
	if err != nil {
		return xerrors.Errorf("can't start docker container: %w", err)
	}
	return nil
}

func (dp *DockerProcess) Wait(ctx context.Context) error {
	log.G(ctx).Debug("Wait until docker containder finished", zap.String("id", dp.id))
	okChan, errChan := dp.docker.ContainerWait(ctx, dp.id, "not-running")
	ctxDone := ctx.Done()
	for {
		select {
		case <-ctxDone:
			return xerrors.Errorf("wait container context cancelled: %w", ctx.Err())
		case ok := <-okChan:
			if ok.Error != nil || ok.StatusCode != 0 {
				var errMess string
				if ok.Error != nil {
					errMess = ok.Error.Message
				}
				log.G(ctx).Error("wait container error", zap.Int64("status_code", ok.StatusCode), zap.String("error_message", errMess))
				return xerrors.Errorf("wait container stop. Exitstatus: '%v' Error message: '%v'", ok.StatusCode, ok.Error)
			}
			return nil
		case err := <-errChan:
			log.G(ctx).Error("connection error while wait container", zap.Error(err))
			timer := time.NewTimer(nonFloodPause)
			select {
			case <-timer.C:
				// pass
			case <-ctxDone:
				// pass
			}
			timer.Stop()
		}
	}
}

func (dp *DockerProcess) Kill(ctx context.Context) error {
	timeout := stopContainerTimeout
	err := dp.docker.ContainerStop(ctx, dp.id, &timeout)
	log.DebugErrorCtx(ctx, err, "Stop docker container", zap.String("docker_id", dp.id))
	return err
}

func (dp *DockerProcess) RemoveContainer(ctx context.Context) error {
	client, err := newDockerClient()
	if err != nil {
		return xerrors.Errorf("docker client create: %w", err)
	}
	defer client.Close()
	err = client.ContainerRemove(ctx, dp.id, types.ContainerRemoveOptions{Force: true})
	log.DebugErrorCtx(ctx, err, "Remove container", zap.String("docker_id", dp.id))
	if err != nil {
		return xerrors.Errorf("remove docker container: %w", err)
	}
	return nil
}

func CreateDockerProcess(command []string, mounts map[string]string) (*DockerProcess, error) {
	dockerClient, err := newDockerClient()
	if err != nil {
		return nil, xerrors.Errorf("create docker client failed: %w", err)
	}
	defer dockerClient.Close()

	dp := DockerProcess{
		docker: dockerClient,
		mounts: mounts,
	}
	dp.containerConfig, dp.hostConfig = defaultDockerContainerConfigs(dockerConfig)
	dp.containerConfig.Cmd = command
	return &dp, nil
}

func Exec(ctx context.Context, command []string, mounts map[string]string) ([]byte, error) {
	dockerClient, err := newDockerClient()
	if err != nil {
		return nil, xerrors.Errorf("create docker client failed: %w", err)
	}
	defer dockerClient.Close()

	dp := DockerProcess{
		docker: dockerClient,
		mounts: mounts,
		id:     "",
	}
	dp.containerConfig, dp.hostConfig = defaultDockerContainerConfigs(dockerConfig)
	dp.containerConfig.Cmd = command
	dp.containerConfig.Tty = true // need for not multiplex stdout log
	dp.hostConfig.AutoRemove = false
	err = dp.Start(ctx)
	if err != nil {
		return nil, xerrors.Errorf("start container: %w", err)
	}
	ctx = log.WithLogger(ctx, log.G(ctx).With(zap.String("container_id", dp.id)))

	defer func() {
		removeCtx, removeCtxCancel := context.WithTimeout(context.Background(), removeContainerTimeout)
		removeCtx = log.WithLogger(removeCtx, log.G(ctx))
		err := dp.RemoveContainer(removeCtx)
		log.DebugErrorCtx(removeCtx, err, "remove container")
		removeCtxCancel()
	}()

	waitError := dp.Wait(ctx)
	log.G(ctx).Debug("Wait container stop", zap.Error(waitError))

	reader, logErr := dockerClient.ContainerLogs(ctx, dp.id, types.ContainerLogsOptions{ShowStdout: true})
	log.DebugErrorCtx(ctx, logErr, "Got container logs")
	if logErr != nil {
		return nil, xerrors.Errorf("get container output: %w", logErr)
	}

	limitedReader := io.LimitReader(reader, maxStdoutSize+1)
	stdout, readLogErr := ioutil.ReadAll(limitedReader)
	if readLogErr != nil {
		return nil, xerrors.Errorf("read stdout from container: %w", readLogErr)
	}
	if len(stdout) > maxStdoutSize {
		return nil, xerrors.Errorf("stdoutsize more then limit. Limit = %v MB: %w", maxStdoutSize/misc.MB, misc.ErrTooLongOuput)
	}
	if waitError != nil {
		outLog := shortenMessage((stdout), 1000)
		log.G(ctx).Debug("Container output", zap.ByteString("stdout", outLog))
		return nil, xerrors.Errorf("Wait error: %w", waitError)
	}
	return stdout, nil
}

func ClearContainers(ctx context.Context) error {
	log.G(ctx).Info("Clear docker containers start")
	dclient, err := newDockerClient()
	if err != nil {
		return xerrors.Errorf("create docker client: %w", err)
	}

	defer dclient.Close()

	filter := filters.NewArgs()
	filter.Add("label", dockerLabel)
	list, err := dclient.ContainerList(ctx, types.ContainerListOptions{All: true, Filters: filter})
	if err != nil {
		return xerrors.Errorf("list containers: %w", err)
	}

	var firstErr error

	for _, c := range list {
		err := dclient.ContainerRemove(ctx, c.ID, types.ContainerRemoveOptions{Force: true})
		log.DebugErrorCtx(ctx, err, "Remove docker container", zap.String("docker_id", c.ID))
		if err != nil && firstErr == nil {
			firstErr = err
		}
	}
	return firstErr
}

// this client has to be closed after job is done
func newDockerClient() (*client.Client, error) {
	return client.NewClientWithOpts()
}

func defaultDockerContainerConfigs(config DockerConfig) (container.Config, container.HostConfig) {
	containerConfig := container.Config{
		Image:           dockerConfig.DockerImage,
		NetworkDisabled: true,
		Labels:          map[string]string{dockerLabel: ""},
		AttachStdout:    true,
		AttachStderr:    true,
		User:            strconv.Itoa(os.Getuid()),
	}
	hostConfig := container.HostConfig{
		NetworkMode:    "none",
		ReadonlyRootfs: true,
		AutoRemove:     true,
		SecurityOpt:    config.securityOptionsParsed,
		Resources: container.Resources{
			Memory:    dockerConfig.DockerMemoryMB * misc.MB,
			DiskQuota: dockerConfig.DockerDiskMB * misc.MB,
			PidsLimit: dockerConfig.DockerMaxPidCount,
			CPUPeriod: int64(time.Second / time.Microsecond),
			CPUQuota:  int64(float64(time.Second) / float64(time.Microsecond) * 100 / dockerConfig.DockerCPUQuotaPercent),
		},
	}
	return containerConfig, hostConfig
}

func shortenMessage(s []byte, maxLen int) []byte {
	if maxLen < 0 {
		panic("shortenMessage maxLen less then zero")
	}

	if len(s) <= maxLen {
		return s
	}

	middlePart := []byte(" ... ")
	if maxLen < 3*len(middlePart) {
		return s[:maxLen]
	}

	meaningLen := maxLen - len(middlePart)
	startLen := meaningLen / 2
	finishLen := startLen
	if meaningLen%2 != 0 {
		finishLen++
	}

	start := s[:startLen]
	finish := s[len(s)-finishLen:]
	result := bytes.Join([][]byte{start, middlePart, finish}, nil)
	return result
}
