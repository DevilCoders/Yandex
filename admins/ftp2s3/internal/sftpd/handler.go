package sftpd

import (
	"io"
	"os"

	"github.com/pkg/sftp"

	s3driver "a.yandex-team.ru/admins/ftp2s3/internal/s3-driver"
	"a.yandex-team.ru/library/go/core/metrics"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type sftpHandler struct {
	driver  *s3driver.S3Driver
	metrics metrics.Registry
}

func newS3Handler(s3api s3driver.S3API, username string, m metrics.Registry) sftp.Handlers {
	handler := &sftpHandler{
		driver: &s3driver.S3Driver{
			Service: s3api,
			Prefix:  "/" + username,
		},
		metrics: m.WithTags(map[string]string{"user": username}),
	}
	return sftp.Handlers{
		FileGet:  handler,
		FilePut:  handler,
		FileCmd:  handler,
		FileList: handler,
	}
}

func (h *sftpHandler) Fileread(r *sftp.Request) (io.ReaderAt, error) {
	h.metrics.Counter("file.read").Inc()

	log.Debugf("Fileread %s on %s", r.Method, r.Filepath)
	reader, err := h.newReaderAt(r.Filepath)
	if err != nil {
		h.metrics.Counter("file.read.fail").Inc()
		log.Errorf("Fileread %s: %v", r.Filepath, err)
	}
	return reader, err
}

func (h *sftpHandler) Filewrite(r *sftp.Request) (io.WriterAt, error) {
	h.metrics.Counter("file.write").Inc()

	pflags := r.Pflags()
	log.Debugf("Filewrite %s on %s, pflags: %+v", r.Method, r.Filepath, pflags)

	w, err := h.newWriterAt(r.Filepath, pflags)
	if err != nil {
		h.metrics.Counter("file.write.fail").Inc()
		log.Errorf("Filewrite %s: %v", r.Filepath, err)
	}
	return w, err

}

func (h *sftpHandler) Filecmd(r *sftp.Request) error {
	h.metrics.Counter("file." + r.Method).Inc()

	log.Debugf("Filecmd %s on %s", r.Method, r.Filepath)
	var err error
	switch r.Method {
	case "Setstat":
		return nil
	case "Rename":
		err = xerrors.New("Rename is prohibited on this server")
	case "Rmdir", "Remove":
		err = h.driver.DeleteFile(r.Filepath)
	case "Mkdir":
		err = h.driver.MakeDirectory(r.Filepath)
	case "Link", "Symlink":
		err = xerrors.New("Link/Symlink are prohibited on this server")
	}
	if err != nil {
		h.metrics.Counter("file.cmd.fail").Inc()
		log.Errorf("Filecmd %s on %s: err=%v", r.Method, r.Filepath, err)
	}
	return err
}

func (h *sftpHandler) Filelist(r *sftp.Request) (sftp.ListerAt, error) {
	h.metrics.Counter("file." + r.Method).Inc()

	log.Debugf("Filelist %s on %s", r.Method, r.Filepath)
	var files []os.FileInfo
	var err error
	switch r.Method {
	case "List":
		files, err = h.driver.ListFiles(r.Filepath)
	case "Stat", "Readlink":
		var info os.FileInfo
		info, err = h.driver.GetFileInfo(r.Filepath)
		files = []os.FileInfo{info}
	}
	if err != nil {
		h.metrics.Counter("file.list.fail").Inc()
		log.Errorf("Filelist %s on %s: err=%v", r.Method, r.Filepath, err)
	}
	return listerat(files), err
}

type listerat []os.FileInfo

func (f listerat) ListAt(ls []os.FileInfo, offset int64) (int, error) {
	var n int
	if offset >= int64(len(f)) {
		return 0, io.EOF
	}
	n = copy(ls, f[offset:])
	if n < len(ls) {
		return n, io.EOF
	}
	return n, nil
}
