package compute

import (
	"context"
	"fmt"
	"strings"
	"time"

	"a.yandex-team.ru/cloud/mdb/image-releaser/internal/images"
	"a.yandex-team.ru/cloud/mdb/internal/compute/compute"
	ints3 "a.yandex-team.ru/cloud/mdb/internal/s3"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/ptr"
)

type lastImageProvider interface {
	last(ctx context.Context, imageName string) (image, error)
}

type computeProvider struct {
	preprodImgAPI compute.ImageService
	preprodFolder string
}

func (c *computeProvider) last(ctx context.Context, imageName string) (image, error) {
	img, err := latestInFolder(ctx, c.preprodImgAPI, c.preprodFolder, imageName)
	if err != nil {
		return image{}, err
	}
	return image{
		ID:          img.ID,
		CreatedAt:   img.CreatedAt,
		Name:        img.Name,
		Description: img.Description,
		Labels:      img.Labels,
		Family:      img.Family,
		MinDiskSize: img.MinDiskSize,
		ProductIDs:  img.ProductIDs,
	}, nil
}

type s3Provider struct {
	s3           ints3.Client
	bucket       string
	imagePattern string
}

func (s *s3Provider) last(ctx context.Context, imageName string) (image, error) {
	prefix := fmt.Sprintf(s.imagePattern, imageName)
	objects, _, err := s.s3.ListObjects(ctx, s.bucket, ints3.ListObjectsOpts{
		Prefix: ptr.String(prefix),
	})
	if err != nil {
		return image{}, err
	}
	img := image{Family: imageName}
	for _, imgObj := range objects {
		if imgObj.LastModified.After(img.CreatedAt) {
			img.CreatedAt = imgObj.LastModified
			name := strings.TrimSuffix(imgObj.Key, ".img")
			img.Name = name
			img.Description = name
		}
	}
	if img.Name == "" {
		return image{}, images.ErrLastNotFound.Wrap(xerrors.Errorf("there are no images with prefix %s in bucket %s", prefix, s.bucket))
	}

	return img, nil
}

type image struct {
	ID          string
	CreatedAt   time.Time
	Name        string
	Description string
	Labels      map[string]string
	Family      string
	MinDiskSize int64
	ProductIDs  []string
}
