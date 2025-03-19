package app

import (
	"context"
	"fmt"
	"io/ioutil"
	"os"
	"os/exec"
	"path"
	"runtime"
	"strconv"
	"time"

	"gopkg.in/yaml.v2"

	"a.yandex-team.ru/cloud/mdb/internal/app"
	"a.yandex-team.ru/cloud/mdb/internal/encodingutil"
	"a.yandex-team.ru/cloud/mdb/internal/monrun"
	"a.yandex-team.ru/cloud/mdb/internal/retry"
	"a.yandex-team.ru/cloud/mdb/internal/sentry"
	"a.yandex-team.ru/cloud/mdb/mdb-salt-sync/internal/publish"
	"a.yandex-team.ru/cloud/mdb/mdb-salt-sync/internal/repos"
	"a.yandex-team.ru/cloud/mdb/mdb-salt-sync/internal/sync"
	"a.yandex-team.ru/cloud/mdb/mdb-salt-sync/internal/vcs"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const (
	DefaultReposConfigPath = "repos.yaml"
)

type Config struct {
	App                  app.Config            `yaml:"app"`
	OverrideVersionAfter encodingutil.Duration `yaml:"override_version_after"`
	VersionsPath         string                `yaml:"versions_path"`
	ReposPath            string                `yaml:"repos_path"`
	CheckoutsPath        string                `yaml:"checkouts_path"`
	ImagesDir            string                `yaml:"images_dir"`
	CheckoutsMaxAge      encodingutil.Duration `yaml:"checkouts_max_age"`
	RootRepo             repos.RepoConfig      `yaml:"root_repo"`
	Publish              publish.Config        `yaml:"publish"`
	GoMaxProcs           int                   `yaml:"go_max_procs"`
}

func DefaultConfig() Config {
	return Config{
		App:                  app.DefaultConfig(),
		OverrideVersionAfter: encodingutil.FromDuration(time.Minute * 10),
		VersionsPath:         "pillar/versions.sls",
		ReposPath:            "pillar/repos.sls",
		CheckoutsPath:        "/salt-sync/checkouts",
		CheckoutsMaxAge:      encodingutil.FromDuration(time.Hour * 3),
		Publish:              publish.DefaultConfig(),
		GoMaxProcs:           4,
	}
}

var _ app.AppConfig = &Config{}

func (c *Config) AppConfig() *app.Config {
	return &c.App
}

type App struct {
	base   *app.App
	config Config
}

func New() *App {
	config := DefaultConfig()
	baseApp, err := app.New(append(app.DefaultToolOptions(&config, DefaultReposConfigPath), app.WithSentry())...)
	if err != nil {
		fmt.Printf("initialization failed with: %s\n", err)
		os.Exit(1)
	}
	return &App{base: baseApp, config: config}
}

func (a *App) Run(buildAndUpload, updateImageAnyway bool) {
	runtime.GOMAXPROCS(a.config.GoMaxProcs)
	if err := a.run(buildAndUpload, updateImageAnyway); err != nil {
		sentry.GlobalClient().CaptureErrorAndWait(context.Background(), err, nil)
		a.base.L().Fatal("Sync failed", log.Error(err))
	}
}

func Monitor(ctx context.Context, warnSince, critSince time.Duration, l log.Logger) monrun.Result {
	monrunRetry := retry.New(retry.Config{
		MaxRetries: 5,
	})
	config := DefaultConfig()
	baseApp, err := app.New(
		app.WithConfig(&config),
		app.WithConfigLoad(DefaultReposConfigPath),
		app.WithLoggerConstructor(func(_ app.LoggingConfig) (log.Logger, error) {
			return l, nil
		}),
	)
	if err != nil {
		return monrun.Result{Code: monrun.WARN, Message: err.Error()}
	}
	publisher, err := publish.NewFromConfig(config.Publish, baseApp.L())
	if err != nil {
		return monrun.Warnf("failed to create publisher: %s", err)
	}
	images, err := publisher.ListImages(ctx)
	if err != nil {
		return monrun.Warnf("failed to list published images: %s", err)
	}

	saltvcs := (&vcs.Factory{}).NewProvider(config.RootRepo, "", baseApp.L())

	var head vcs.Head
	if err := monrunRetry.RetryWithLog(
		ctx,
		func() error {
			remoteHead, err := saltvcs.RemoteHead()
			head = remoteHead
			return err
		},
		"get remote head",
		l,
	); err != nil {
		return monrun.Result{Code: monrun.WARN, Message: err.Error()}
	}

	return monitorPublishedImages(head, images, warnSince, critSince, l)
}

func monitorPublishedImages(last vcs.Head, images []publish.Image, warnSince, critSince time.Duration, l log.Logger) monrun.Result {
	lastRev, err := strconv.ParseInt(last.Version, 10, 64)
	if err != nil {
		return monrun.Critf("failed to parse current version %q: %s", last.Version, err)
	}

	if len(images) == 0 {
		return monrun.Critf("there are no salt images in s3")
	}

	maxUploadedRev, maxUploadedDate := maxImage(images, l)

	if lastRev == maxUploadedRev {
		return monrun.Result{}
	}

	uploadDelta := last.Time.Sub(maxUploadedDate)
	if uploadDelta > warnSince {
		code := monrun.WARN
		if uploadDelta > critSince {
			code = monrun.CRIT
		}
		return monrun.Result{
			Code: code,
			Message: fmt.Sprintf(
				"Uploaded image too old (rev %d, date: %s). A newer change exists (rev: %s, date: %s)",
				maxUploadedRev, uploadDelta,
				last.Version, last.Time,
			),
		}
	}
	return monrun.Result{}
}

func loadYAMLConfig(p string, cfg interface{}) error {
	buf, err := ioutil.ReadFile(p)
	if err != nil {
		return xerrors.Errorf("failed to load config from %q: %w", p, err)
	}
	if err = yaml.Unmarshal(buf, cfg); err != nil {
		return xerrors.Errorf("failed to parse config from %q: %w", p, err)
	}
	return nil
}

func (a *App) pull(reposConfig repos.Config, vcsFactory *vcs.Factory) error {
	envrootvcs := vcsFactory.NewProvider(reposConfig.Fileroots, "", a.base.L())
	subvcs := make([]vcs.Provider, len(reposConfig.FilerootsSubrepos))
	for i, repo := range reposConfig.FilerootsSubrepos {
		subvcs[i] = vcsFactory.NewProvider(repo, reposConfig.FilerootsEnvsMountPathDevEnv, a.base.L())
	}
	a.base.L().Info("Updating repositories")
	for _, v := range append(subvcs, envrootvcs) {
		if err := v.Pull(); err != nil {
			if remerr := vcs.RemoveCheckouts(a.config.CheckoutsPath, a.base.L()); remerr != nil {
				a.base.L().Error("Error while removing vcs checkouts due to pull vcs checkout error", log.Error(remerr))
			}
			return xerrors.Errorf("failed to update trunk on %s vcs: %w", v, err)
		}
	}
	return nil
}

func (a *App) run(buildAndUpload, updateImageAnyway bool) error {
	ctx := context.Background()
	if err := vcs.InitCheckouts(a.config.CheckoutsPath, a.config.CheckoutsMaxAge.Duration, a.base.L()); err != nil {
		if remerr := vcs.RemoveCheckouts(a.config.CheckoutsPath, a.base.L()); remerr != nil {
			a.base.L().Error("Error while removing vcs checkouts due to init vcs checkout error", log.Error(remerr))
		}
		return xerrors.Errorf("failed to init vcs cache: %w", err)
	}

	vcsFactory := &vcs.Factory{CheckoutsPath: a.config.CheckoutsPath}
	saltvcs := vcsFactory.NewProvider(a.config.RootRepo, "", a.base.L())
	// First of all update salt - /srv, cause we need fresh repos.sls
	if err := saltvcs.Pull(); err != nil {
		if remerr := vcs.RemoveCheckouts(a.config.CheckoutsPath, a.base.L()); remerr != nil {
			a.base.L().Error("Error while removing salt vcs checkouts due to pull vcs checkout error", log.Error(remerr))
		}
		return xerrors.Errorf("failed to update trunk on %s vcs: %w", saltvcs, err)
	}

	reposConfig := repos.Config{}
	if err := loadYAMLConfig(path.Join(saltvcs.GetMountPath(), a.config.ReposPath), &reposConfig); err != nil {
		return xerrors.Errorf("failed to load repos.sls: %w", err)
	}

	if err := a.pull(reposConfig, vcsFactory); err != nil {
		return err
	}

	publisher, err := publish.NewFromConfig(a.config.Publish, a.base.L())
	if err != nil {
		return err
	}

	if buildAndUpload && !updateImageAnyway {
		uploadedImages, err := publisher.ListImages(ctx)
		if err != nil {
			return err
		}
		head, err := saltvcs.LocalHead()
		if err != nil {
			return xerrors.Errorf("failed to get current version: %w", err)
		}
		a.base.L().Debug(
			"Check should we build and upload or not",
			log.Reflect("head", head),
			log.Reflect("uploadedImages", uploadedImages),
		)
		should, err := shouldBuildImage(head.Version, uploadedImages, a.config.OverrideVersionAfter.Duration, a.base.L())
		if err != nil {
			return err
		}
		if !should {
			a.base.L().Info("We shouldn't update image right now")
			return nil
		}
	}

	devVCSFactory := vcsFactory
	if !buildAndUpload {
		// Checkouts without cache+rsync
		devVCSFactory = &vcs.Factory{}
	}
	if err := sync.Dev(
		devVCSFactory,
		reposConfig,
		a.config.RootRepo,
		buildAndUpload,
		log.With(a.base.L(), log.String("env", "dev")),
	); err != nil {
		return err
	}

	// Reload repos.sls cause it may change during the pull
	// We shouldn't do anything special if it changes, cause likely it's new repositories
	reposConfig = repos.Config{}
	if err := loadYAMLConfig(path.Join(saltvcs.GetMountPath(), a.config.ReposPath), &reposConfig); err != nil {
		return xerrors.Errorf("failed to reload repos.sls: %w", err)
	}
	// Versions are stored inside salt so we load that config after we checkout salt itself
	versions := make(map[string]map[string]string)
	if err := loadYAMLConfig(path.Join(saltvcs.GetMountPath(), a.config.VersionsPath), &versions); err != nil {
		return xerrors.Errorf("failed to load versions.sls: %w", err)
	}

	for env, envVersions := range versions {
		if err := sync.Env(
			vcsFactory,
			env,
			reposConfig,
			envVersions,
			log.With(a.base.L(), log.String("env", env)),
		); err != nil {
			return xerrors.Errorf("error while syncing %q env: %w", env, err)
		}
	}

	if !buildAndUpload {
		a.base.L().Info("Don't publish created image, cause it's personal salt update")
		return nil
	}

	imageVersion, err := saltvcs.LocalHead()
	if err != nil {
		return xerrors.Errorf("failed to get salt version: %w", err)
	}

	imageFile := path.Join(a.config.ImagesDir, "image.txz")
	if err := archive(ctx, a.config.RootRepo.Mount, imageFile, a.base.L()); err != nil {
		return err
	}

	if err := publisher.PutImage(ctx, imageFile, imageVersion.Version); err != nil {
		return err
	}
	return nil
}

func archive(ctx context.Context, imageDir, imageArchive string, l log.Logger) error {
	cmd := exec.CommandContext(ctx, "tar", "-cJvf", imageArchive, imageDir, "--owner=0", "--group=0")
	if _, err := vcs.Execute(cmd, l); err != nil {
		return xerrors.Errorf("Failed to archive image: %w", err)
	}
	return nil
}

func maxImage(images []publish.Image, l log.Logger) (int64, time.Time) {
	var maxUploadedRev int64
	var maxUploadedDate time.Time
	for _, img := range images {
		uploadedRev, err := strconv.ParseInt(img.Version, 10, 64)
		if err != nil {
			l.Warn("failed to parse published version. Ignore it", log.String("version", img.Version), log.Error(err))
			continue
		}
		if maxUploadedRev <= uploadedRev {
			maxUploadedRev = uploadedRev
			if maxUploadedDate.Before(img.Uploaded) {
				maxUploadedDate = img.Uploaded
			}
		}
	}
	return maxUploadedRev, maxUploadedDate
}

func shouldBuildImage(currentVersion string, uploaded []publish.Image, overrideVersionAfter time.Duration, l log.Logger) (bool, error) {
	if len(uploaded) == 0 {
		return true, nil
	}

	currentRev, err := strconv.ParseInt(currentVersion, 10, 64)
	if err != nil {
		return false, xerrors.Errorf("failed to parse current version %q: %w", currentVersion, err)
	}

	maxUploadedRev, maxUploadedDate := maxImage(uploaded, l)

	if currentRev > maxUploadedRev {
		return true, nil
	}

	if time.Since(maxUploadedDate) > overrideVersionAfter {
		return true, nil
	}

	return false, nil
}
