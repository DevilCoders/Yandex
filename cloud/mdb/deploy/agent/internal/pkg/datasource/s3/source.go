package s3

import (
	"context"
	"strings"
	"time"

	"a.yandex-team.ru/cloud/mdb/deploy/agent/internal/pkg/commander"
	"a.yandex-team.ru/cloud/mdb/deploy/agent/internal/pkg/datasource"
	"a.yandex-team.ru/cloud/mdb/internal/s3"
	"a.yandex-team.ru/cloud/mdb/internal/s3/http"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/ptr"
)

type Config struct {
	Client http.Config `json:"client" yaml:"client"`
	Bucket string      `json:"bucket" yaml:"bucket"`
}

func DefaultConfig() Config {
	return Config{
		Client: http.DefaultConfig(),
		Bucket: "mdb-salt-images",
	}
}

type Source struct {
	client s3.Client
	cfg    Config
	l      log.Logger
}

var _ datasource.DataSource = &Source{}

func New(client s3.Client, cfg Config, l log.Logger) *Source {
	return &Source{client: client, cfg: cfg, l: l}
}

func NewFromConfig(cfg Config, l log.Logger) (*Source, error) {
	client, err := http.New(cfg.Client, l)
	if err != nil {
		return nil, xerrors.Errorf("init s3 client: %w", err)
	}
	return &Source{client: client, cfg: cfg, l: l}, nil
}

func (s *Source) getAllObjects(ctx context.Context) ([]s3.Object, error) {
	objects, _, err := s.client.ListObjects(
		ctx,
		s.cfg.Bucket,
		s3.ListObjectsOpts{Prefix: ptr.String(commander.ImagePrefix)},
	)
	if err != nil {
		return nil, xerrors.Errorf("get bucket objects: %w", err)
	}
	if len(objects) == 0 {
		return nil, xerrors.Errorf("there are no object in s3://%s/%s", s.cfg.Bucket, commander.ImagePrefix)
	}
	return objects, nil
}

func (s *Source) LatestVersion(ctx context.Context) (string, error) {
	objects, err := s.getAllObjects(ctx)
	if err != nil {
		return "", err
	}
	maxKey := objects[0].Key
	for _, o := range objects {
		if o.Key > maxKey {
			maxKey = o.Key
		}
	}
	return commander.VersionFromFromName(maxKey), nil
}

func (s *Source) ResolveVersion(ctx context.Context, version string) (string, error) {
	objects, err := s.getAllObjects(ctx)
	if err != nil {
		return "", err
	}
	var matched []string
	for _, o := range objects {
		if strings.HasSuffix(commander.VersionFromFromName(o.Key), version) {
			matched = append(matched, o.Key)
		}
	}
	switch len(matched) {
	case 0:
		return "", xerrors.Errorf(
			"no objects found for the version '%s' in 's3://%s/%s'. This version doesn't yet exist or has already been removed",
			version, s.cfg.Bucket, commander.ImagePrefix,
		)
	case 1:
		// Not an ideal solution, cause currently we have a lot of images with same revision
		// (We rebuild /srv image at least one per ten minutes if revision is not changed).
		// But for initial logic it's okay.
		return commander.VersionFromFromName(matched[0]), nil
	default:
		return "", xerrors.Errorf(
			"found more than one object that matches version '%s', keys: %s in 's3://%s/%s'",
			version, strings.Join(matched, ", "), s.cfg.Bucket, commander.ImagePrefix,
		)
	}
}

func (s *Source) Fetch(ctx context.Context, version string) (datasource.NamedReadCloser, error) {
	startAt := time.Now()
	var key string
	objects, _, err := s.client.ListObjects(ctx, s.cfg.Bucket, s3.ListObjectsOpts{
		Prefix: ptr.String(commander.ImagePrefix + version),
	})
	if err != nil {
		return nil, xerrors.Errorf("resolving version '%s' to key: %w", version, err)
	}
	switch len(objects) {
	case 0:
		return nil, xerrors.Errorf("unable to find objects for version '%s' and prefix '%s'", version, commander.ImagePrefix)
	case 1:
		key = objects[0].Key
	default:
		allKeys := make([]string, 0, len(objects))
		for _, o := range objects {
			allKeys = append(allKeys, o.Key)
		}
		return nil, xerrors.Errorf("find more then one object that match version '%s' keys: %s", version, strings.Join(allKeys, ", "))
	}

	obj, err := s.client.GetObject(ctx, s.cfg.Bucket, key)
	if err != nil {
		return nil, xerrors.Errorf("get '%s' object: %w", key, err)
	}
	s.l.Infof("successfully fetch image archive in %s", time.Since(startAt))
	return &image{obj: obj, name: key}, nil
}
