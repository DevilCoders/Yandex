package disklock

import (
	"context"
	"crypto/rand"
	"fmt"
	"io/ioutil"
	"os"
	"os/exec"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const (
	cryptsetupBinPath = "/sbin/cryptsetup"
)

func (d *Disklock) Format() error {
	d.BaseApp.L().Debug("format disks")
	for _, disk := range d.disks {
		ctx := ctxWithDisk(d.BaseApp.ShutdownContext(), disk)
		key, ok := d.keys[disk.Key]
		if !ok {
			return xerrors.Errorf("there is no key with name %s", disk.Key)
		}

		passphrase := make([]byte, d.cfg.KeyLength)
		_, err := rand.Read(passphrase)
		if err != nil {
			return xerrors.Errorf("create random key: %w", err)
		}

		if err = key.EncryptKey(ctx, passphrase); err != nil {
			return xerrors.Errorf("encrypt key %s: %w", key.Name(), err)
		}

		if err = d.formatDisk(ctx, disk, passphrase); err != nil {
			return xerrors.Errorf("format disk %s: %w", disk.Name, err)
		}
	}
	d.BaseApp.L().Debug("all disks are formatted")
	return nil
}

func (d *Disklock) formatDisk(ctx context.Context, disk Disk, passphrase []byte) error {
	l := d.BaseApp.L()
	ctxlog.Debug(ctx, l, "format disk")

	decryptedKey, err := ioutil.TempFile("", fmt.Sprintf("disklock-%s", disk.Name))
	if err != nil {
		return xerrors.Errorf("create temporary file: %w", err)
	}
	defer func() { _ = os.Remove(decryptedKey.Name()) }()

	if _, err = decryptedKey.Write(passphrase); err != nil {
		return xerrors.Errorf("write passphrase to temporary file: %w", err)
	}

	if err = decryptedKey.Close(); err != nil {
		return xerrors.Errorf("close temporary file: %w", err)
	}

	args := []string{"luksFormat", disk.ID, decryptedKey.Name()}
	ctxlog.Debug(ctx, l, "run cryptsetup", log.Strings("args", args))
	cmd := exec.CommandContext(ctx, cryptsetupBinPath, args...)
	out, err := cmd.CombinedOutput()
	if err != nil {
		ctxlog.Error(ctx, l, "got an error", log.ByteString("output", out), log.Error(err))
		return xerrors.Errorf("run cryptsetup: %w", err)
	}

	ctxlog.Debug(ctx, l, "got an output", log.ByteString("output", out))
	ctxlog.Debug(ctx, l, "disk is formatted")

	return nil
}
