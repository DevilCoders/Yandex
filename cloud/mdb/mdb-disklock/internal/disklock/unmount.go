package disklock

import (
	"context"

	"golang.org/x/sys/unix"

	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (d *Disklock) Unmount() error {
	d.BaseApp.L().Debug("unmount disks")
	for _, disk := range d.disks {
		ctx := ctxWithDisk(d.BaseApp.ShutdownContext(), disk)

		if err := d.unmountDisk(ctx, disk); err != nil {
			return xerrors.Errorf("unmount disk %s: %w", disk.Name, err)
		}
	}
	d.BaseApp.L().Debug("all disks are unmounted")
	return nil
}

func (d *Disklock) unmountDisk(ctx context.Context, disk Disk) error {
	l := d.BaseApp.L()
	if isMounted, err := d.isMounted(disk); err != nil {
		return xerrors.Errorf("isMounted: %w", err)
	} else if !isMounted {
		ctxlog.Debug(ctx, l, "disk is already unmounted")
		return nil
	}

	ctxlog.Debug(ctx, l, "unmount disk")
	if err := unix.Unmount(disk.Mount, unix.MNT_DETACH); err != nil {
		return xerrors.Errorf("unmount: %w", err)
	}
	ctxlog.Debug(ctx, l, "disk is unmounted")
	return nil
}
