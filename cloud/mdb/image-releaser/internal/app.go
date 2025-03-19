package internal

import (
	"context"
	"fmt"
	"time"

	"a.yandex-team.ru/cloud/mdb/image-releaser/internal/images"
	"a.yandex-team.ru/cloud/mdb/image-releaser/internal/images/compute"
	"a.yandex-team.ru/cloud/mdb/image-releaser/internal/images/porto"
	"a.yandex-team.ru/cloud/mdb/image-releaser/pkg/checker"
	jugCheck "a.yandex-team.ru/cloud/mdb/image-releaser/pkg/checker/juggler"
	"a.yandex-team.ru/cloud/mdb/internal/app"
	"a.yandex-team.ru/cloud/mdb/internal/juggler"
	jugglerclient "a.yandex-team.ru/cloud/mdb/internal/juggler/http"
	"a.yandex-team.ru/cloud/mdb/internal/sentry"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type ReleaserApp struct {
	*app.App
	portoIm    func() (images.PortoImages, error)
	computeIm  func() (images.ComputeImages, error)
	jugglerAPI juggler.API
}

type Config struct {
	App     app.Config           `json:"app" yaml:"app"`
	Juggler jugglerclient.Config `json:"juggler" yaml:"juggler"`

	Porto   porto.Config   `json:"porto,omitempty" yaml:"porto,omitempty"`
	Compute compute.Config `json:"compute,omitempty" yaml:"compute,omitempty"`
}

func (c *Config) AppConfig() *app.Config {
	return &c.App
}

func DefaultConfig() Config {
	return Config{
		Juggler: jugglerclient.DefaultConfig(),
		App:     app.DefaultConfig(),
		Compute: compute.DefaultConfig(),
		Porto:   porto.Config{},
	}
}

func NewApp(conf Config, baseApp *app.App) *ReleaserApp {
	l := baseApp.L()

	jugAPI, err := jugglerclient.NewClient(conf.Juggler, l)
	if err != nil {
		l.Fatalf("failed to initialize juggler client: %s", err.Error())
	}

	var imagC images.ComputeImages
	compImagesF := func() (images.ComputeImages, error) {
		if imagC == nil {
			imagC, err = compute.NewCompute(conf.Compute, l)
			if err != nil {
				return nil, xerrors.Errorf("create compute images: %s", err.Error())
			}
		}
		return imagC, nil
	}

	var imagP images.PortoImages
	portoImagesF := func() (images.PortoImages, error) {
		if imagP == nil {
			imagP, err = porto.NewPorto(conf.Porto, l)
			if err != nil {
				return nil, xerrors.Errorf("create compute images: %s", err.Error())
			}
		}
		return imagP, nil
	}
	go baseApp.WaitForStop()
	return &ReleaserApp{App: baseApp, jugglerAPI: jugAPI, computeIm: compImagesF, portoIm: portoImagesF}
}

func (ra *ReleaserApp) ReleaseCompute(ctx context.Context,
	imageName string, os string, productIDs []string, poolsize int, folderID string,
	noCheck bool, checkHost, checkService string, reqStability time.Duration,
) {
	im, err := ra.computeIm()
	if err != nil {
		ra.L().Fatalf("%s", err.Error())
		return
	}
	parsedOS, err := images.ParseOS(os)
	if err != nil {
		ra.L().Fatalf("release compute: %s", err.Error())
		return
	}
	checkers := checks(noCheck, ra.jugglerAPI, checkHost, checkService, reqStability)
	releaseRes := im.Release(ctx, imageName, parsedOS, productIDs, poolsize, folderID, checkers)
	logResult(ctx, ra.L(), releaseRes, imageName)
}

func (ra *ReleaserApp) ReleaseDataproc(ctx context.Context,
	imageID string, version string, versionPrefix string, imageFamily string, isForce bool, folderID string) {
	im, err := ra.computeIm()
	if err != nil {
		ra.L().Fatalf("%s", err.Error())
		return
	}
	assignedVersion, err := im.ReleaseDataproc(ctx, imageID, version, versionPrefix, imageFamily, isForce, folderID)
	fmt.Printf("Assigned version: %s\n", assignedVersion)
	if err != nil {
		ra.L().Fatalf("%s", err.Error())
		return
	}
}

func (ra *ReleaserApp) CleanupComputeImages(ctx context.Context, imageName string, keepImages int, folderID string) {
	im, err := ra.computeIm()
	if err != nil {
		ra.L().Fatalf("%s", err.Error())
		return
	}
	if err := im.Cleanup(ctx, imageName, keepImages, folderID); err != nil {
		ra.L().Errorf("cleanup failed because: %s", err)
		sentry.GlobalClient().CaptureErrorAndWait(ctx, err, nil)
	} else {
		ra.L().Infof("cleanup succeeded for %q, keep images: %d, folder: %q", imageName, keepImages, folderID)
	}
}

func (ra *ReleaserApp) ReleasePorto(ctx context.Context, imageName string, noCheck bool, checkHost, checkService string, reqStability time.Duration) {
	im, err := ra.portoIm()
	if err != nil {
		ra.L().Fatalf("%s", err.Error())
		return
	}
	checkers := checks(noCheck, ra.jugglerAPI, checkHost, checkService, reqStability)
	releaseRes := im.Release(ctx, imageName, checkers)
	logResult(ctx, ra.L(), releaseRes, imageName)
}

func (ra *ReleaserApp) AgePorto(ctx context.Context, imageName string) (time.Duration, error) {
	im, err := ra.portoIm()
	if err != nil {
		return 0, err
	}
	return im.DestinationAge(ctx, imageName, time.Now())
}

func (ra *ReleaserApp) AgeCompute(ctx context.Context, imageName string, folderID string) (time.Duration, error) {
	im, err := ra.computeIm()
	if err != nil {
		return 0, err
	}
	return im.DestinationAge(ctx, imageName, folderID, time.Now())
}

func logResult(ctx context.Context, l log.Logger, releaseRes error, imageName string) {
	switch {
	case xerrors.Is(releaseRes, images.ErrNothingReleased):
		l.Infof("nothing to release")
	case xerrors.Is(releaseRes, images.ErrUnstable):
		l.Infof("%s not released because: %s", imageName, releaseRes.Error())
	case xerrors.Is(releaseRes, images.ErrLastNotFound):
		l.Errorf("failed to release because: %s", releaseRes.Error())
	case releaseRes == nil:
		l.Infof("successfully released: %s", imageName)
	default:
		l.Errorf("failed to release because: %s", releaseRes.Error())
		sentry.GlobalClient().CaptureErrorAndWait(ctx, releaseRes, nil)
	}
}

func checks(noCheck bool, jugAPI juggler.API, checkHost string, checkService string, reqStability time.Duration) []checker.Checker {
	var checks []checker.Checker
	if !noCheck {
		checks = []checker.Checker{jugCheck.NewJugglerChecker(jugAPI, checkHost, checkService, reqStability)}
	}
	return checks
}
