package images

import (
	"context"
	"time"

	"a.yandex-team.ru/cloud/mdb/image-releaser/pkg/checker"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var ErrNothingReleased = xerrors.NewSentinel("nothing to release")
var ErrUnstable = xerrors.NewSentinel("image is not stable yet")
var ErrLastNotFound = xerrors.NewSentinel("last image not found")

type PortoImages interface {
	Release(ctx context.Context, imageName string, checks []checker.Checker) error
	DestinationAge(ctx context.Context, imageName string, now time.Time) (time.Duration, error)
}

type ComputeImages interface {
	Release(ctx context.Context, imageName string, os OS, productIDs []string, poolSize int, destinationFolder string, checks []checker.Checker) error
	ReleaseDataproc(ctx context.Context, imageID string, version string, versionPrefix string, imageFamily string, isForce bool, folderID string) (string, error)
	DestinationAge(ctx context.Context, imageName string, destinationFolder string, now time.Time) (time.Duration, error)
	Cleanup(ctx context.Context, imageName string, keepImages int, destinationFolder string) error
}
