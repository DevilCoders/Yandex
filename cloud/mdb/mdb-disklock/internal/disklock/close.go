package disklock

import (
	"context"
	"fmt"
	"os"

	"github.com/anatol/luks.go"

	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (d *Disklock) Close() error {
	d.BaseApp.L().Debug("close disks")
	for _, disk := range d.disks {
		ctx := ctxWithDisk(d.BaseApp.ShutdownContext(), disk)
		if err := d.closeDisk(ctx, disk); err != nil {
			return xerrors.Errorf("close disk %s: %w", disk.Key, err)
		}
	}
	d.BaseApp.L().Debug("all disks are closed")
	return nil
}

func (d *Disklock) closeDisk(ctx context.Context, disk Disk) error {
	l := d.BaseApp.L()
	ctxlog.Debug(ctx, l, "close disk")

	mapper := fmt.Sprintf("/dev/mapper/%s", disk.Name)
	if _, err := os.Stat(mapper); err != nil && xerrors.Is(err, os.ErrNotExist) {
		ctxlog.Debug(ctx, l, "mapper doesn't exist, disk is closed")
		return nil
	}

	if err := luks.Lock(disk.Name); err != nil {
		return xerrors.Errorf("lock disk: %w", err)
	}

	ctxlog.Debug(ctx, l, "disk is closed")
	return nil
}
