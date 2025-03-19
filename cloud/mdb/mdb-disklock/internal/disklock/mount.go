package disklock

import (
	"context"
	"os"
	"strings"
	"time"

	"golang.org/x/sys/unix"

	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const (
	procMounts = "/proc/mounts"
)

func (d *Disklock) Mount() error {
	d.BaseApp.L().Debug("mount disks")
	for _, disk := range d.disks {
		ctx := ctxWithDisk(d.BaseApp.ShutdownContext(), disk)

		if err := d.mountDisk(ctx, disk); err != nil {
			return xerrors.Errorf("mount disk %s: %w", disk.Name, err)
		}
	}
	d.BaseApp.L().Debug("all disks are mounted")
	return nil
}

func (d *Disklock) mountDisk(ctx context.Context, disk Disk) error {
	l := d.BaseApp.L()
	if isMounted, err := d.isMounted(disk); err != nil {
		return xerrors.Errorf("isMounted: %w", err)
	} else if isMounted {
		ctxlog.Debug(ctx, l, "disk is already mounted")
		return nil
	}

	if err := d.waitDiskIsPresent(ctx, disk); err != nil {
		return xerrors.Errorf("waitDiskIsPresent: %w", err)
	}

	ctxlog.Debug(ctx, l, "mount disk")
	opts := uintptr(unix.MS_NOATIME | unix.MS_NODIRATIME | unix.MS_MGC_VAL)
	if err := unix.Mount(disk.Mapper(), disk.Mount, "ext4", opts, "errors=remount-ro"); err != nil {
		return xerrors.Errorf("mount: %w", err)
	}
	ctxlog.Debug(ctx, l, "disk is mounted")
	return nil
}

func (d *Disklock) isMounted(disk Disk) (bool, error) {
	data, err := os.ReadFile(procMounts)
	if err != nil {
		return false, xerrors.Errorf("open %s: %w", procMounts, err)
	}
	if strings.Contains(string(data), disk.Mapper()) {
		return true, nil
	} else {
		return false, nil
	}
}

func (d *Disklock) waitDiskIsPresent(ctx context.Context, disk Disk) error {
	ctx, cancel := context.WithTimeout(ctx, d.mountWaitTimeout)
	defer cancel()

	ctxlog.Debug(ctx, d.BaseApp.L(), "wait while disk is present")
	ticker := time.NewTicker(100 * time.Millisecond)
	defer ticker.Stop()
	for {
		if _, err := os.Stat(disk.Mapper()); err != nil {
			if xerrors.Is(err, os.ErrNotExist) {
				select {
				case <-ctx.Done():
					return xerrors.Errorf("timeout exceeded: %w", ctx.Err())
				case <-ticker.C:
					continue
				}
			}
			return xerrors.Errorf("stat %s: %w", disk.Mapper(), err)
		}
		return nil
	}
}
