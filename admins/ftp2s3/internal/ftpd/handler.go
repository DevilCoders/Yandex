package ftpd

import (
	"os"

	"github.com/fclairamb/ftpserver/server"

	s3driver "a.yandex-team.ru/admins/ftp2s3/internal/s3-driver"
	"a.yandex-team.ru/library/go/core/metrics"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type ftpHandler struct {
	driver  *s3driver.S3Driver
	metrics metrics.Registry
}

func (h *ftpHandler) CanAllocate(server.ClientContext, int) (bool, error) {
	return true, nil
}

func (h *ftpHandler) ChmodFile(server.ClientContext, string, os.FileMode) error {
	return nil
}

func (h *ftpHandler) ChangeDirectory(server.ClientContext, string) error {
	return nil
}

func (h *ftpHandler) MakeDirectory(cc server.ClientContext, directory string) error {
	h.metrics.Counter("makeDirectory").Inc()
	return h.driver.MakeDirectory(directory)
}

func (h *ftpHandler) ListFiles(cc server.ClientContext, directory string) ([]os.FileInfo, error) {
	h.metrics.Counter("listFiles").Inc()
	return h.driver.ListFiles(directory)
}

func (h *ftpHandler) GetFileInfo(cc server.ClientContext, path string) (os.FileInfo, error) {
	h.metrics.Counter("getFileInfo").Inc()
	return h.driver.GetFileInfo(path)
}

func (h *ftpHandler) OpenFile(cc server.ClientContext, path string, flag int) (server.FileStream, error) {
	h.metrics.Counter("openFile").Inc()
	return h.driver.OpenFile(path, flag, h.metrics)
}

func (h *ftpHandler) DeleteFile(cc server.ClientContext, path string) error {
	h.metrics.Counter("deleteFile").Inc()
	return h.driver.DeleteFile(path)
}

func (h *ftpHandler) RenameFile(server.ClientContext, string, string) error {
	h.metrics.Counter("renameFile").Inc()
	return xerrors.New("RNTO is prohibited on this server")
}
