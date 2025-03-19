package convert

import (
	"go.uber.org/zap"
	"golang.org/x/net/context"
	"golang.org/x/xerrors"

	log "a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/config"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/misc"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/nbd"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/storage"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/logging"
)

const (
	qemuChunkSize = 2 * 64 * 1024

	qemuCPUTime = 10
)

// Converter is a base class for image-conversion-via-nbd stuff.
type Converter struct {
	Registry     nbd.Registry
	workersCount int
}

// NewConverter returns a new Converter instance.
func NewConverter(ctx context.Context, conf *config.Config) (*Converter, error) {
	ctx = log.WithLogger(ctx, log.G(ctx).With(logging.Function("NewConverter")))
	registry, err := nbd.NewRegistry(ctx, conf.QemuDockerProxy.SocketPath, conf.General.Tmpdir)
	if err != nil {
		log.G(ctx).Error("Can't get ndb registry", zap.Error(err))
		return nil, err
	}
	log.G(ctx).Info("Convert workers.", zap.Int("count", conf.Performance.ConvertWorkers))

	return &Converter{
		Registry:     registry,
		workersCount: conf.Performance.ConvertWorkers,
	}, nil
}

// GetNbdParams returns params for NbdMoveSrc device.
func (c *Converter) GetNbdParams(ctx context.Context, img nbd.Image) (params NbdParams, err error) {
	ctx = log.WithLogger(ctx, log.G(ctx).With(logging.Function("Converter.GetNbdParams")))
	device, err := c.Registry.MountNBD(ctx, img)
	if err != nil {
		log.G(ctx).Error("Can't mount nbd", zap.Error(err))
		return params, xerrors.Errorf("can't mount nbd image %v: %w", img, err)
	}

	return NbdParams{
		Size:         misc.Ceil(device.Size(), storage.DefaultChunkSize),
		BlockSize:    storage.DefaultChunkSize,
		ChunkSize:    img.GetChunkSize(),
		WorkersCount: img.GetReaderCount(c.workersCount),
		Device:       device,
	}, nil
}

// Close closes the Converter.
func (c *Converter) Close(ctx context.Context) {
	c.Registry.Close(ctx)
}

// NbdParams are params for NbdMoveSrc device.
type NbdParams struct {
	Size         int64      // Rounded up source size
	BlockSize    int64      // Read api block size (default)
	ChunkSize    int        // File read block size (depends on source)
	WorkersCount int        // Optimal number of readers
	Device       nbd.Device // NBD device
}
