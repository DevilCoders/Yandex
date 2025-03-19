package nbd

import (
	"io/ioutil"
	"os"
	"path/filepath"

	"golang.org/x/xerrors"

	"go.uber.org/zap"
	"golang.org/x/net/context"

	log "a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"
	"a.yandex-team.ru/cloud/compute/snapshot/internal/dockerprocess"
)

const (
	// QemuAddressSpace is memory limit for qemu utils
	QemuAddressSpace = 1 * 1024 * 1024 * 1024

	closedDeviceErrorText        = "nbd: read on closed device"
	containerConnectDir          = "/connect"
	qemuNbdSocketPathInContainer = "/connect/qemu-nbd"
)

// IsClosedDeviceError check if error about nbd device is closed
func IsClosedDeviceError(err error) bool {
	if err == nil {
		return false
	}
	return err.Error() == closedDeviceErrorText
}

// Image is a Qemu image abstraction
type Image interface {
	GetQemuBlockdev(ctx context.Context) (string, error)
	GetChunkSize() int
	GetReaderCount(maxCount int) int
	GetFormat() string
}

// Device is a NetworkBlockDevice
type Device interface {
	ReadAt(p []byte, off int64) (n int, err error) // io.ReadAt
	Size() int64
	Close(ctx context.Context) error
}

// Registry controls created NBDs
type Registry interface {
	// MountNBD returns new Device backed by NBD.
	// It's enough to call Device.Close to dispose a Device.
	MountNBD(ctx context.Context, img Image) (Device, error)
	// Check runs monitoring checks.
	Check(ctx context.Context) error
	// Close closes the registry.
	Close(ctx context.Context)
}

type nbdRegistry struct {
	dir             string
	proxySocketPath string
}

type deviceTmpDirWrapper struct {
	Device
	dir string
}

func (w deviceTmpDirWrapper) Close(ctx context.Context) error {
	errClose := w.Device.Close(ctx)
	log.DebugErrorCtx(ctx, errClose, "Close wrapped device")
	errRemove := os.RemoveAll(w.dir)
	log.DebugErrorCtx(ctx, errRemove, "Remove tmp dir", zap.String("dir", w.dir))
	if errClose == nil {
		return errRemove
	} else {
		return errClose
	}
}

// NewRegistry returns new NBDs controller that holds metadata inside dir
func NewRegistry(ctx context.Context, proxySocketPath, tmpDir string) (Registry, error) {
	_, err := os.Stat(tmpDir)
	if err != nil {
		return nil, xerrors.Errorf("stat nbd registry tmp dir: %w", err)
	}

	r := &nbdRegistry{
		dir:             tmpDir,
		proxySocketPath: proxySocketPath,
	}

	// Clear all devices left from the previous launch
	if err := r.clear(ctx); err != nil {
		return nil, err
	}

	err = r.init(ctx)
	if err != nil {
		return nil, err
	}

	return r, nil
}

func (r *nbdRegistry) getConnectTmpDir() string {
	return filepath.Join(r.dir, "connections")
}

func (r *nbdRegistry) MountNBD(ctx context.Context, img Image) (device Device, resErr error) {
	logger := log.G(ctx)

	dir, err := ioutil.TempDir(r.getConnectTmpDir(), "nbd")
	defer func() {
		if err != nil {
			_ = os.RemoveAll(dir)
		}
	}()

	log.DebugErrorCtx(ctx, err, "Create tmp socket dir", zap.String("dir", dir))
	if err != nil {
		return nil, xerrors.Errorf("create tmp socket dir: %w", err)
	}
	qemuNbdSocketPath := filepath.Join(dir, filepath.Base(qemuNbdSocketPathInContainer))

	blkdev, err := img.GetQemuBlockdev(ctx)
	log.DebugError(logger, err, "Got GetQemuBlockdev")
	if err != nil {
		return nil, err
	}

	mounts := map[string]string{
		r.proxySocketPath: dockerprocess.ProxySocketInContainer,
		dir:               containerConnectDir,
	}

	command := []string{
		"qemu-nbd",
		"--read-only",
		"-k", qemuNbdSocketPathInContainer,
	}
	imageFormat := img.GetFormat()
	if imageFormat != "" {
		command = append(command, "-f", imageFormat)
	}
	command = append(command, blkdev)

	var dp = wrapExecToStartQemuNbdFunc(
		nbdStartTimeout, dir, filepath.Base(dockerprocess.ProxySocketInContainer),
		func(ctx context.Context) error {
			result, err := dockerprocess.Exec(ctx, command, mounts)
			log.DebugErrorCtx(ctx, err, "Qemu nbd finished", zap.ByteString("output", result))
			return err
		})

	n, err := NewNbdConnector(ctx, dp, qemuNbdSocketPath)
	if err != nil {
		log.G(ctx).Error("Can't mount ndb device", zap.Error(err))
		return nil, ErrNbdInternal
	}

	w := deviceTmpDirWrapper{Device: n, dir: dir}

	return w, nil
}

func (r *nbdRegistry) Check(ctx context.Context) error {
	log.G(ctx).Info("Need implement nbd registry check")
	return nil
}

func (r *nbdRegistry) Close(ctx context.Context) {
	_ = r.clear(ctx)
}

func (r *nbdRegistry) clear(ctx context.Context) error {
	var firstErr error
	firstErr = dockerprocess.ClearContainers(ctx)
	log.DebugErrorCtx(ctx, firstErr, "Clear docker containers finished")

	err := os.RemoveAll(r.getConnectTmpDir())
	log.DebugErrorCtx(ctx, err, "Remove previous connections dir")
	if err == nil && firstErr != nil {
		firstErr = err
	}

	if firstErr == nil {
		return nil
	}

	return xerrors.Errorf("clear nbd registry: %w", firstErr)
}

func (r *nbdRegistry) init(ctx context.Context) error {
	err := os.Mkdir(r.getConnectTmpDir(), 0700)
	log.DebugErrorCtx(ctx, err, "Create tmp dir for connections")
	return err
}
