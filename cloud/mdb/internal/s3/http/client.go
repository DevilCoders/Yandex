package http

import (
	"context"
	"io"
	"path"

	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/aws/awserr"
	"github.com/aws/aws-sdk-go/aws/credentials"
	"github.com/aws/aws-sdk-go/aws/credentials/stscreds"
	"github.com/aws/aws-sdk-go/aws/defaults"
	"github.com/aws/aws-sdk-go/aws/request"
	"github.com/aws/aws-sdk-go/aws/session"
	"github.com/aws/aws-sdk-go/service/s3"

	"a.yandex-team.ru/cloud/mdb/internal/httputil"
	ints3 "a.yandex-team.ru/cloud/mdb/internal/s3"
	"a.yandex-team.ru/cloud/mdb/internal/secret"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/ptr"
)

type Transport struct {
	TLS     httputil.TLSConfig     `json:"tls" yaml:"tls"`
	Logging httputil.LoggingConfig `json:"logging" yaml:"logging"`
}

type Config struct {
	Host               string        `json:"host" yaml:"host"`
	Region             string        `json:"region" yaml:"region"`
	AccessKey          secret.String `json:"access_key" yaml:"access_key"`
	SecretKey          secret.String `json:"secret_key" yaml:"secret_key"`
	Anonymous          bool          `json:"anonymous" yaml:"anonymous"`
	UseYCMetadataToken bool          `json:"use_yc_metadata_token" yaml:"use_yc_metadata_token"`
	Role               string        `json:"role" yaml:"role"`
	ForcePathStyle     bool          `json:"force_path_style" yaml:"force_path_style"`
	Transport          Transport     `json:"transport" yaml:"transport"`
}

func DefaultConfig() Config {
	return Config{
		Region: "RU",
	}
}

type Client struct {
	s3cli *s3.S3
}

func New(config Config, l log.Logger) (*Client, error) {
	client, err := httputil.NewClient(
		httputil.ClientConfig{
			Name: "s3",
			Transport: httputil.TransportConfig{
				TLS:     config.Transport.TLS,
				Logging: config.Transport.Logging,
			},
		},
		l,
	)
	if err != nil {
		return nil, err
	}

	disableTLS := false
	if config.Transport.TLS.Insecure {
		disableTLS = true
	}
	awsCfg := aws.NewConfig().
		WithEndpoint(config.Host).
		WithRegion(config.Region).
		WithHTTPClient(client.Client).
		WithDisableSSL(disableTLS).
		WithS3ForcePathStyle(config.ForcePathStyle)

	ses, err := session.NewSession(awsCfg)
	if err != nil {
		return nil, err
	}

	switch {
	case config.Anonymous:
		ses.Config.WithCredentials(credentials.AnonymousCredentials)
	case config.UseYCMetadataToken:
		// Yandex.Cloud mimics metadata service, so we can use AWS credentials, but set token to another header.
		// Using a service account token is not a public API, but it is considered normal for service clouds.
		credsConfig := aws.NewConfig().
			WithRegion(config.Region).
			WithHTTPClient(client.Client)
		cred := credentials.NewCredentials(defaults.RemoteCredProvider(*credsConfig, defaults.Handlers()))

		ses.Config.WithCredentials(cred)
		ses.Handlers.Send.PushFront(func(r *request.Request) {
			token := r.HTTPRequest.Header.Get("X-Amz-Security-Token")
			r.HTTPRequest.Header.Add("X-YaCloud-SubjectToken", token)
		})
	default:
		ses.Config.WithCredentials(credentials.NewStaticCredentials(config.AccessKey.Unmask(), config.SecretKey.Unmask(), ""))
	}

	if config.Role == "" {
		return &Client{s3cli: s3.New(ses)}, nil
	} else {
		return &Client{s3cli: s3.New(ses, &aws.Config{Credentials: stscreds.NewCredentials(ses, config.Role)})}, nil
	}
}

func (cli *Client) ListBuckets(ctx context.Context) ([]ints3.Bucket, error) {
	buckets, err := cli.s3cli.ListBucketsWithContext(ctx, &s3.ListBucketsInput{})
	if err != nil {
		return nil, cli.wrapError(err)
	}
	res := make([]ints3.Bucket, 0, len(buckets.Buckets))
	for _, b := range buckets.Buckets {
		if err := notNilRequired(b.Name, "Name"); err != nil {
			return nil, err
		}
		if err := notNilRequired(b.CreationDate, "CreationDate"); err != nil {
			return nil, err
		}
		res = append(res, ints3.Bucket{
			CreationDate: *b.CreationDate,
			Name:         *b.Name,
		})
	}
	return res, err
}

func (cli *Client) ListObjects(ctx context.Context, bucket string, opts ints3.ListObjectsOpts) ([]ints3.Object, []ints3.Prefix, error) {
	var objects []ints3.Object
	var prefixes []ints3.Prefix

	if err := opts.Validate(); err != nil {
		return nil, nil, err
	}

	truncated := true
	var continuationToken *string
	for truncated {
		objResp, err := cli.s3cli.ListObjectsV2WithContext(ctx, &s3.ListObjectsV2Input{
			Bucket:            &bucket,
			MaxKeys:           opts.MaxKeys,
			Prefix:            opts.Prefix,
			Delimiter:         opts.Delimiter,
			ContinuationToken: continuationToken,
		})

		if err != nil {
			return nil, nil, cli.wrapError(err)
		}
		for _, o := range objResp.Contents {
			if err := notNilRequired(o.Key, "Key"); err != nil {
				return nil, nil, err
			}
			if err := notNilRequired(o.LastModified, "LastModified"); err != nil {
				return nil, nil, err
			}
			if err := notNilRequired(o.Size, "Size"); err != nil {
				return nil, nil, err
			}
			objects = append(objects, ints3.Object{
				Key:          *o.Key,
				LastModified: *o.LastModified,
				Size:         *o.Size,
			})
		}

		for _, p := range objResp.CommonPrefixes {
			if err := notNilRequired(p.Prefix, "Prefix"); err != nil {
				return nil, nil, err
			}
			prefixes = append(prefixes, ints3.Prefix{
				Prefix: *p.Prefix,
			})
		}
		if err := notNilRequired(objResp.IsTruncated, "IsTruncated"); err != nil {
			return nil, nil, err
		}
		truncated = *objResp.IsTruncated
		continuationToken = objResp.NextContinuationToken

		if opts.MaxKeys != nil {
			break
		}
	}

	return objects, prefixes, nil
}

func (cli *Client) CopyObject(ctx context.Context, srcBucket, srcKey, dstBucket, dstKey string) error {
	_, err := cli.s3cli.CopyObjectWithContext(ctx, &s3.CopyObjectInput{
		Bucket:     ptr.String(dstBucket),
		CopySource: ptr.String(path.Join(srcBucket, srcKey)),
		Key:        ptr.String(dstKey),
	})
	return cli.wrapError(err)
}

func (cli *Client) GetObject(ctx context.Context, bucket, key string) (io.ReadCloser, error) {
	out, err := cli.s3cli.GetObjectWithContext(ctx, &s3.GetObjectInput{
		Bucket: ptr.String(bucket),
		Key:    ptr.String(key),
	})
	if err != nil {
		return nil, cli.wrapError(err)
	}
	return out.Body, nil
}

func (cli *Client) PutObject(ctx context.Context, bucket, key string, body io.ReadSeeker, opts ints3.PutObjectOpts) error {
	_, err := cli.s3cli.PutObjectWithContext(ctx, &s3.PutObjectInput{
		Bucket:        ptr.String(bucket),
		Key:           ptr.String(key),
		Body:          body,
		ContentType:   opts.ContentType,
		ContentLength: opts.ContentLength,
	})
	return cli.wrapError(err)
}

func (cli *Client) DeleteObject(ctx context.Context, bucket, key string) error {
	_, err := cli.s3cli.DeleteObjectWithContext(ctx, &s3.DeleteObjectInput{
		Bucket: ptr.String(bucket),
		Key:    ptr.String(key),
	})
	return cli.wrapError(err)
}

func notNilRequired(v interface{}, name string) error {
	if v == nil {
		return xerrors.Errorf("invalid result: %s should not be nil", name)
	}
	return nil
}

func (cli *Client) wrapError(err error) error {
	if err2, ok := err.(awserr.RequestFailure); ok && err2.StatusCode() == 404 {
		return ints3.ErrNotFound.Wrap(err)
	}
	return err
}
