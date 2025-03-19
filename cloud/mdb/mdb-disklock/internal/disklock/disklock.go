package disklock

import (
	"context"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/app"
	"a.yandex-team.ru/cloud/mdb/mdb-disklock/internal/keys"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Disklock struct {
	BaseApp          *app.App
	keys             map[string]keys.Key
	disks            []Disk
	mountWaitTimeout time.Duration
	cfg              Config
}

func NewDisklock(cfg Config, baseApp *app.App) (*Disklock, error) {
	res := &Disklock{
		keys:             make(map[string]keys.Key),
		disks:            cfg.Disks,
		BaseApp:          baseApp,
		mountWaitTimeout: cfg.MountWaitTimeout.Duration,
		cfg:              cfg,
	}

	for _, key := range cfg.Keys {
		k, err := keys.NewKey(key, baseApp.L(), cfg.AWSTransport)
		if err != nil {
			return nil, xerrors.Errorf("new key %s: %w", key.Name, err)
		}
		res.keys[key.Name] = k
	}

	return res, nil
}

func ctxWithDisk(ctx context.Context, disk Disk) context.Context {
	return ctxlog.WithFields(ctx,
		log.String("diskID", disk.ID),
		log.String("diskName", disk.Name),
		log.String("diskKey", disk.Key),
		log.String("diskMount", disk.Mount),
	)
}
