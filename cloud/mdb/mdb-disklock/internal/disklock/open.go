package disklock

import (
	"context"
	"os"

	"github.com/anatol/luks.go"

	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (d *Disklock) Open() error {
	d.BaseApp.L().Debug("open disks")
	for _, disk := range d.disks {
		ctx := ctxWithDisk(d.BaseApp.ShutdownContext(), disk)
		key, ok := d.keys[disk.Key]
		if !ok {
			return xerrors.Errorf("there is no key with name %s", disk.Key)
		}

		passphrase, err := key.GetKey(ctx)
		if err != nil {
			return xerrors.Errorf("get key %s: %w", key.Name(), err)
		}

		if err = d.openDisk(ctx, disk, passphrase); err != nil {
			return xerrors.Errorf("open disk %s: %w", disk.Name, err)
		}
	}
	d.BaseApp.L().Debug("all disks are opened")
	return nil
}

func (d *Disklock) openDisk(ctx context.Context, disk Disk, passphrase []byte) error {
	l := d.BaseApp.L()
	ctxlog.Debug(ctx, l, "open disk")
	dev, err := luks.Open(disk.ID)
	if err != nil {
		return xerrors.Errorf("luks open: %w", err)
	}
	defer func() { _ = dev.Close() }()

	if _, err = os.Stat(disk.Mapper()); err == nil {
		ctxlog.Debug(ctx, l, "mapper already exists, disk is opened")
		return nil
	}

	ctxlog.Debug(ctx, l, "do unlock")
	err = dev.UnlockAny(passphrase, disk.Name)
	if err != nil {
		if xerrors.Is(err, luks.ErrPassphraseDoesNotMatch) {
			return xerrors.Errorf("incorrect key for disk: %w", err)
		}
		return xerrors.Errorf("unlock disk: %w", err)
	}

	ctxlog.Debug(ctx, l, "disk is opened")

	return nil
}
