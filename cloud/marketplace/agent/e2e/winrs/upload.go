package winrs

import (
	"crypto/md5"
	"encoding/base64"
	"encoding/hex"
	"errors"
	"fmt"
	"io"
	"os"
	"strings"

	"github.com/gofrs/uuid"
	"github.com/masterzen/winrm"
)

var errChckSum = errors.New("md5 checksum mismatch")

// compareFile compares remote and local file by calculating md5 checksum.
func (w *WinRS) compareFile(remote, local string) error {
	file, err := openFile(local)
	if err != nil {
		return err
	}

	hash := md5.New() //nolint:gosec
	if _, err = io.Copy(hash, file); err != nil {
		return err
	}
	localMD5 := hex.EncodeToString(hash.Sum(nil))

	var remoteMD5 string
	remoteMD5, err = w.getFileMD5(remote)
	if err != nil {
		return err
	}

	if localMD5 != remoteMD5 {
		return errChckSum
	}

	return nil
}

func (w *WinRS) getFileMD5(path string) (res string, err error) {
	c := fmt.Sprintf(`
		(Get-FileHash -Path "%s" -Algorithm:MD5 |Select-Object -ExpandProperty Hash).ToLower()`, path)
	var stdout string
	stdout, err = w.runCommandS(winrm.Powershell(c))
	if err != nil {
		return
	}

	res = strings.Trim(stdout, "\r\n")

	return
}

// winrmSetMaxEnvelope configures remote WinRM service to use maximum envelope size.
func (w *WinRS) winrmSetMaxEnvelope() error {
	return w.runCommand(winrm.Powershell(`Set-Item WSMan:\localhost\MaxEnvelopeSizekb -Value 8192`))
}

// winrmSetMaxOps configures remote WinRM service to use maximum concurrent operations per user.
func (w *WinRS) winrmSetMaxOps() error {
	c := `Set-Item WSMan:\localhost\Service\MaxConcurrentOperationsPerUser -Value 4294967295`

	return w.runCommand(winrm.Powershell(c))
}

// UploadFile copies file on remote windows instance via WinRM, by:
// * tweaking a bit remote WinRM service
// * reading local file
// * encoding chunks in base64 string
// * generating remote file line by line
// * restoring file
// * comparing md5 checksum.
func (w *WinRS) UploadFile(dst, src string) error {
	file, err := openFile(src)
	if err != nil {
		return err
	}

	var name uuid.UUID
	name, err = uuid.NewV4()
	if err != nil {
		return err
	}
	cmdTempFilePath := fmt.Sprintf("%%temp%%\\%s", name.String())
	pwshTempFilePath := fmt.Sprintf("$ENV:TEMP\\%s", name.String())

	err = w.winrmSetMaxEnvelope()
	if err != nil {
		return err
	}

	err = w.winrmSetMaxOps()
	if err != nil {
		return err
	}

	err = w.generateRestoreFile(cmdTempFilePath, file)
	if err != nil {
		return err
	}

	err = w.processRestoreFile(dst, pwshTempFilePath)
	if err != nil {
		return err
	}

	err = w.compareFile(dst, src)
	if err != nil {
		return err
	}

	return nil
}

var errUnexpDir = errors.New("provided directory, not file")

// openFile is a helper, which return error when you try to open directory, not file.
func openFile(path string) (*os.File, error) {
	file, err := os.Open(path)
	if err != nil {
		return nil, err
	}

	var fileStat os.FileInfo
	fileStat, err = file.Stat()
	if err != nil {
		return nil, err
	}

	if fileStat.IsDir() {
		return nil, errUnexpDir
	}

	return file, nil
}

// generateRestoreFile creates file on remote endpoint by filling it with base64 encoded strings of original file.
func (w *WinRS) generateRestoreFile(path string, f io.Reader) (err error) {
	// fi, err := os.Stat(path)
	// if err != nil {
	// 	return
	// }
	// fsize := fi.Size()

	// for command to fit envelope size we have in `winrm` package
	buffSize := ((8150 - len(path)) / 4) * 3 //nolint:gomnd
	buff := make([]byte, buffSize)

	// fchunks := fsize / int64(buffSize)
	var i int64
	for {
		var n int
		n, err = f.Read(buff)
		if errors.Is(err, io.EOF) {
			err = nil
		}
		if n == 0 {
			break
		}

		// we generate `cmd` commands to fill temporary file line by line
		b64str := base64.StdEncoding.EncodeToString(buff[:n])
		c := fmt.Sprintf(`echo %s >> "%s"`, b64str, path)
		if err = w.runCommand(c); err != nil {
			break
		}

		i++
		fmt.Fprintf(os.Stdout, "uploaded chunk %v, %v bytes total\n", i, i*int64(n))
		// fmt.Fprintf(os.Stdout, "uploaded encoded file: %v of %v chunks\n", i, fchunks)
	}

	return
}

// processRestoreFile restore by converting base64 encoded file into original one.
func (w *WinRS) processRestoreFile(dst, src string) error {
	// awful implementation, but to make it better
	// we must rewrite winrm package
	c := fmt.Sprintf(`
		$ErrorActionPreference = 'Stop'

		$tmp = "%s"
		$dst = "%s"

		if (Test-Path -Path $dst -PathType:Container) {
			Exit 1
		}

		Remove-Item $dst -ErrorAction:SilentlyContinue
		New-Item -Type:File -Path $dst -Force | Out-Null

		try {
			$r = [System.IO.File]::OpenText($tmp)
			$w = [System.IO.File]::OpenWrite($dst)

			for() {
				$l = $r.ReadLine()
				if ( [string]::IsNullOrEmpty($l) ) { break }
				$b = [System.Convert]::FromBase64String($l)
				$w.write($b, 0, $b.Length)
			}
		} catch {
			Remove-Item $dst -ErrorAction:SilentlyContinue
			throw $Error[0]
		} finally {
			Remove-Item $tmp -ErrorAction:SilentlyContinue
			$r.Close()
			$w.Close()
		}
	`, src, dst)

	fmt.Fprintf(os.Stdout, "restoring encoded file at remote machine\n")

	return w.runCommand(winrm.Powershell(c))
}
