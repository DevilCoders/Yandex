package main

import (
	"compress/gzip"
	"context"
	"flag"
	"fmt"
	"io"
	"os"

	"go.uber.org/zap"

	log "a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"
	"a.yandex-team.ru/cloud/compute/snapshot/pkg/version"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/client"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/globalauth"
)

const (
	imageFileMode   = 0600
	compressionGzip = "gzip"
)

var fmtVersion = fmt.Sprintf("version=%s build=%s hash=%s tag=%s", version.Version, version.Build, version.GitHash, version.GitTag)

func main() {
	flag.Usage = usage
	flag.Parse()

	if *showVersionParam {
		fmt.Println(fmtVersion)
		return
	}

	logger, err := zap.NewDevelopment()
	if err != nil {
		_, _ = os.Stderr.WriteString("Can't create logger")
		os.Exit(1)
	}
	ctx := log.WithLogger(context.Background(), logger)

	if flag.NArg() != 1 {
		log.G(ctx).Fatal("Need exact one positional argument: snapshot id")
	}
	snapshotID := flag.Arg(0)

	globalauth.InitCredentials(ctx, globalauth.MetadataTokenPolicy)

	snapshotClient, err := client.NewClient(ctx, *snapshotServerParam, false, globalauth.GetCredentials())
	if err != nil {
		log.G(ctx).Fatal("Can't create snapshot client.", zap.Error(err))
	}

	err = snapshotClient.Ping(ctx)
	if err != nil {
		log.G(ctx).Fatal("Can't ping snapshot server", zap.Error(err))
	}

	snapshotReader, err := snapshotClient.OpenSnapshot(ctx, snapshotID)
	if err != nil {
		log.G(ctx).Fatal("Can't open snapshot", zap.String("snapshotID", snapshotID), zap.Error(err))
	}

	log.G(ctx).Debug("Snapshot checksum", zap.String("snapshotID", snapshotID),
		zap.String("Checksum", snapshotReader.GetSnapshotChecksum()))

	writer, err := createImageWriter(ctx, snapshotID)
	if err != nil {
		log.G(ctx).Fatal("Can't open snapshot destination.", zap.String("snapshotID", snapshotID), zap.Error(err))
	}

	err = writeSnapshot(ctx, writer, snapshotReader)
	if err != nil {
		log.G(ctx).Fatal("Can't write snapshot", zap.String("snapshotID", snapshotID), zap.Error(err))
	}
}

func usage() {
	flag.CommandLine.SetOutput(os.Stdout)
	fmt.Printf(`Usage:
%s [options] <SnapshotID>
`, os.Args[0])
	flag.PrintDefaults()
}

type gzipCloser struct {
	*gzip.Writer
	file *os.File
}

func (gz gzipCloser) Close() error {
	err := gz.Writer.Close()
	if err != nil {
		return err
	}
	return gz.file.Close()
}

func createImageWriter(ctx context.Context, snapshotID string) (io.WriteCloser, error) {
	if *outFileparam == "-" {
		return os.Stdout, nil
	}

	var fileName string
	if *outFileparam == "" {
		fileName = snapshotID + ".raw"
		if *compressionParam == compressionGzip {
			fileName += ".gz"
		}
	} else {
		fileName = *outFileparam
	}

	flags := os.O_WRONLY | os.O_TRUNC | os.O_CREATE

	if *overwritePatam {
		flags |= os.O_TRUNC
	} else {
		flags |= os.O_EXCL
	}

	file, err := os.OpenFile(fileName, flags, imageFileMode)
	if err != nil {
		log.G(ctx).Error("Can't open snapshot file.", zap.String("FileName", fileName), zap.Error(err))
		return nil, err
	}

	switch *compressionParam {
	case "":
		return file, nil
	case compressionGzip:
		gzipStream := gzip.NewWriter(file)
		return gzipCloser{
			Writer: gzipStream,
			file:   file,
		}, nil
	default:
		_ = file.Close()
		_ = os.Remove(fileName)
		return nil, fmt.Errorf("doesn't support compression format: '%s'", *compressionParam)
	}
}

// Writer can be file, stdout, compressed stream
func writeSnapshot(ctx context.Context, writer io.WriteCloser, reader client.SnapshotReader) (err error) {
	size := reader.GetSize()

	var writerAt io.WriterAt

	if tmpWriterAt, ok := writer.(io.WriterAt); ok && size > 0 {
		// Some files can't random write. For example /dev/stderr /dev/stdout
		// Test real work of WriteAt without errors
		// And set logical size of file by write last byte
		if _, err := tmpWriterAt.WriteAt([]byte{0}, size-1); err == nil {
			writerAt = tmpWriterAt
		}
	}

	defer func() {
		closeErr := writer.Close()
		if closeErr != nil {
			log.G(ctx).Error("Can't close snapshot dest.")
		}
		if err == nil && closeErr != nil {
			err = closeErr
		}
	}()

	log.G(ctx).Debug("Start snapshot write.")

	chunkSize := reader.GetChunkSize()
	blockBuffer := make([]byte, chunkSize)
	for offset := int64(0); offset < size; offset += chunkSize {
		if offset+chunkSize > size {
			blockBuffer = blockBuffer[:size-offset]
		}

		isZeroBlock, err := reader.IsZeroChunk(chunkSize, offset)
		if err != nil {
			log.G(ctx).Error("Can't get zero status of chunk", zap.String("snapshot_id", reader.GetID()),
				zap.Error(err))
			isZeroBlock = false
		}

		if isZeroBlock && writerAt != nil {
			// Can skip zero blocks for random-write files.
			continue
		}

		read, err := reader.ReadAt(ctx, blockBuffer, offset)
		if err != nil {
			log.G(ctx).Error("Can't read snapshot.", zap.Int64("offset", offset), zap.Error(err))
			return err
		}
		if read != len(blockBuffer) {
			log.G(ctx).Error("Read not full block", zap.Int64("offset", offset),
				zap.Int("blocksize", len(blockBuffer)), zap.Int("read", read))
			return fmt.Errorf("read not full block")
		}
		if writerAt == nil {
			_, err = writer.Write(blockBuffer)
		} else {
			_, err = writerAt.WriteAt(blockBuffer, offset)
		}
		if err != nil {
			log.G(ctx).Error("Can't write to snapshot destination", zap.Error(err))
			return err
		}

		if offset/chunkSize%100 == 0 {
			log.G(ctx).Debug("Snapshot download progress.", zap.Int64("current", offset),
				zap.Int64("total", size), zap.Float64("%", float64(offset)/float64(size)*100))
		}
	}

	log.G(ctx).Debug("Start snapshot write complete.")

	return nil
}
