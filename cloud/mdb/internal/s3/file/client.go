package file

import (
	"bytes"
	"context"
	"encoding/json"
	"io"
	"io/ioutil"
	"os"
	"strings"
	"time"

	ints3 "a.yandex-team.ru/cloud/mdb/internal/s3"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/slices"
)

var errNotImplemented = semerr.NotImplemented("method not implemented in s3 file mock")

type object struct {
	Key          string
	LastModified int64
	Size         int64
	Body         json.RawMessage
}

type prefix struct {
	Prefix string
}

type Client struct {
	path string
}

func New(path string) *Client {
	return &Client{path: path}
}

func (cli *Client) readFile() ([]object, error) {
	data, err := ioutil.ReadFile(cli.path)
	if err != nil {
		return nil, cli.wrapError(err)
	}
	var parsed struct {
		Contents []object
	}
	err = json.Unmarshal(data, &parsed)
	if err != nil {
		return nil, err
	}

	return parsed.Contents, nil
}

func (cli *Client) ListBuckets(ctx context.Context) ([]ints3.Bucket, error) {
	return nil, errNotImplemented
}

func (cli *Client) ListObjects(ctx context.Context, bucket string, opts ints3.ListObjectsOpts) ([]ints3.Object, []ints3.Prefix, error) {
	fObjects, err := cli.readFile()
	if err != nil {
		return nil, nil, err
	}

	prefix := ""
	if opts.Prefix != nil {
		prefix = *opts.Prefix
	}

	delim := ""
	if opts.Delimiter != nil {
		delim = *opts.Delimiter
	}

	var objects []ints3.Object
	var prefixes []string
	for _, obj := range fObjects {
		if !strings.HasPrefix(obj.Key, prefix) {
			continue
		}

		if delim == "" || !strings.Contains(strings.TrimPrefix(obj.Key, prefix), delim) {
			objects = append(objects, ints3.Object{
				Key:          obj.Key,
				LastModified: time.Unix(obj.LastModified, 0),
				Size:         obj.Size,
			})
		} else {
			unprefixed := strings.TrimPrefix(obj.Key, prefix)
			prefixes = append(prefixes, prefix+unprefixed[:strings.Index(unprefixed, delim)]+delim)
		}
	}

	prefixes = slices.DedupStrings(prefixes)
	s3Prefixes := make([]ints3.Prefix, 0, len(prefixes))
	for _, p := range prefixes {
		s3Prefixes = append(s3Prefixes, ints3.Prefix{Prefix: p})
	}

	return objects, s3Prefixes, nil
}

func (cli *Client) CopyObject(ctx context.Context, srcBucket, srcKey, dstBucket, dstKey string) error {
	return errNotImplemented
}

func (cli *Client) GetObject(ctx context.Context, bucket, key string) (io.ReadCloser, error) {
	objects, err := cli.readFile()
	if err != nil {
		return nil, err
	}
	for _, obj := range objects {
		if obj.Key == key {
			return ioutil.NopCloser(bytes.NewReader(obj.Body)), nil
		}
	}
	return nil, ints3.ErrNotFound.Wrap(xerrors.Errorf("object with key %q was not found in S3", key))
}

func (cli *Client) PutObject(ctx context.Context, bucket, key string, body io.ReadSeeker, opts ints3.PutObjectOpts) error {
	return errNotImplemented
}

func (cli *Client) DeleteObject(ctx context.Context, bucket, key string) error {
	return errNotImplemented
}

func (cli *Client) wrapError(err error) error {
	if os.IsNotExist(err) {
		return ints3.ErrNotFound.Wrap(err)
	}
	return err
}
