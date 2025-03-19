package porto

import (
	"context"
	"fmt"
	"time"

	"a.yandex-team.ru/cloud/mdb/image-releaser/internal/images"
	"a.yandex-team.ru/cloud/mdb/image-releaser/pkg/checker"
	ints3 "a.yandex-team.ru/cloud/mdb/internal/s3"
	s3http "a.yandex-team.ru/cloud/mdb/internal/s3/http"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/ptr"
)

type Porto struct {
	s3                ints3.Client
	sourceBucket      string
	destinationBucket string
	l                 log.Logger
}

type Config struct {
	S3                s3http.Config `json:"s3" yaml:"s3"`
	SourceBucket      string        `json:"source_bucket" yaml:"source_bucket"`
	DestinationBucket string        `json:"destination_bucket" yaml:"destination_bucket"`
}

type s3ImgVersion struct {
	objKey string
	date   time.Time
}

const s3PrefixTemplate = "%s-bionic"

func NewPorto(conf Config, l log.Logger) (*Porto, error) {
	s3, err := s3http.New(conf.S3, l)
	if err != nil {
		return nil, xerrors.Errorf("failed to initialize s3 client: %w", err)
	}
	return &Porto{
		s3:                s3,
		sourceBucket:      conf.SourceBucket,
		destinationBucket: conf.DestinationBucket,
		l:                 l,
	}, nil
}

func (p *Porto) Release(ctx context.Context, imageName string, checks []checker.Checker) error {
	lastTest, err := lastImage(ctx, p.s3, imageName, p.sourceBucket)
	if err != nil {
		return err
	}
	if err := p.performChecks(ctx, checks, lastTest.date); err != nil {
		return images.ErrUnstable.Wrap(err)
	}

	lastProd, err := lastImage(ctx, p.s3, imageName, p.destinationBucket)
	if err != nil && !xerrors.Is(err, images.ErrLastNotFound) {
		return xerrors.Errorf("failed to get last image in prod: %w", err)
	}
	if lastTest.date.After(lastProd.date) {
		return release(ctx, p.s3, lastTest, p.sourceBucket, p.destinationBucket)
	}
	return images.ErrNothingReleased
}

func (p *Porto) performChecks(ctx context.Context, checks []checker.Checker, since time.Time) error {
	for _, check := range checks {
		err := check.IsStable(ctx, since, time.Now())
		if err != nil {
			return err
		}
	}
	return nil
}

func release(ctx context.Context, cli ints3.Client, ver s3ImgVersion, fromBucket, toBucket string) error {
	err := cli.CopyObject(ctx, fromBucket, ver.objKey, toBucket, ver.objKey)
	if err != nil {
		return xerrors.Errorf("failed to release %s to %s: %w", ver.objKey, toBucket, err)
	}
	return nil
}

func (p *Porto) SrcAge(ctx context.Context, imageName string, now time.Time) (time.Duration, error) {
	img, err := lastImage(ctx, p.s3, imageName, p.sourceBucket)
	if err != nil {
		return 0, err
	}
	return now.Sub(img.date), nil
}

func (p *Porto) DestinationAge(ctx context.Context, imageName string, now time.Time) (time.Duration, error) {
	img, err := lastImage(ctx, p.s3, imageName, p.destinationBucket)
	if err != nil {
		return 0, err
	}
	return now.Sub(img.date), nil
}

func lastImage(ctx context.Context, cli ints3.Client, imageName string, bucket string) (s3ImgVersion, error) {
	var img s3ImgVersion

	prefix := fmt.Sprintf(s3PrefixTemplate, imageName)
	objects, _, err := cli.ListObjects(ctx, bucket, ints3.ListObjectsOpts{
		Prefix: ptr.String(prefix),
	})
	if err != nil {
		return img, err
	}
	for _, imgObj := range objects {
		modified := imgObj.LastModified
		if imgObj.LastModified.After(img.date) {
			img.date = modified
			img.objKey = imgObj.Key
		}
	}
	if img.objKey == "" {
		return img, images.ErrLastNotFound.Wrap(xerrors.Errorf("there are no images with prefix %s in bucket %s", prefix, bucket))
	}
	return img, nil
}
