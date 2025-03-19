package logbroker

import (
	"bytes"
	"compress/gzip"
	"io"
	"testing"

	"github.com/klauspost/compress/zstd"
	"github.com/stretchr/testify/suite"
)

type readersTestSuite struct {
	suite.Suite
}

func TestReaders(t *testing.T) {
	suite.Run(t, new(readersTestSuite))
}

func (suite *readersTestSuite) TestRaw() {
	data := "some message data"

	r := rawReader{}
	r.reader.Reset([]byte(data))

	suite.Equal(len(data), r.RawSize())

	got, err := io.ReadAll(&r)
	suite.Require().NoError(err)
	suite.EqualValues(data, got)

	suite.Zero(r.RawSize())
	suite.NoError(r.Close())
}

func (suite *readersTestSuite) TestGzip() {
	data := "some message data"
	buf := &bytes.Buffer{}
	{
		gw := gzip.NewWriter(buf)
		_, err := gw.Write([]byte(data))
		suite.Require().NoError(err)
		suite.Require().NoError(gw.Close())
	}
	compressed := buf.Bytes()

	r := gzipReader{}
	r.reader.Reset(compressed)

	suite.Equal(len(compressed), r.RawSize())

	got, err := io.ReadAll(&r)
	suite.Require().NoError(err)
	suite.EqualValues(data, got)

	suite.Zero(r.RawSize())
	suite.NoError(r.Close())
}

func (suite *readersTestSuite) TestZstd() {
	data := "some message data"
	buf := &bytes.Buffer{}
	{
		zw, err := zstd.NewWriter(buf)
		suite.Require().NoError(err)
		_, err = zw.Write([]byte(data))
		suite.Require().NoError(err)
		suite.Require().NoError(zw.Close())
	}
	compressed := buf.Bytes()

	r := zstdReader{}
	r.reader.Reset(compressed)

	suite.Equal(len(compressed), r.RawSize())

	got, err := io.ReadAll(&r)
	suite.Require().NoError(err)
	suite.EqualValues(data, got)

	suite.Zero(r.RawSize())
	suite.NoError(r.Close())
}
