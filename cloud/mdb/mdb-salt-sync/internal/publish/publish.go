package publish

import (
	"context"
	"fmt"
	"os"
	"path"
	"regexp"
	"sort"
	"sync"
	"time"

	"golang.org/x/sync/errgroup"

	"a.yandex-team.ru/cloud/mdb/internal/encodingutil"
	"a.yandex-team.ru/cloud/mdb/internal/s3"
	"a.yandex-team.ru/cloud/mdb/internal/s3/http"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/ptr"
)

type EnvConfig struct {
	Bucket              string      `json:"bucket" yaml:"bucket"`
	S3                  http.Config `json:"s3" yaml:"s3"`
	PossiblyUnavailable bool        `json:"possibly_unavailable" yaml:"possibly_unavailable"`
}

func (c EnvConfig) Name() string {
	return c.S3.Host + "/" + c.Bucket
}

type Config struct {
	KeepImages          int                   `json:"keep_images" yaml:"keep_images"`
	Envs                []EnvConfig           `json:"envs" yaml:"envs"`
	ListImagesTimeout   encodingutil.Duration `json:"list_images_timeout" yaml:"list_images_timeout"`
	PutImageTimeout     encodingutil.Duration `json:"put_image_timeout" yaml:"put_image_timeout"`
	RemoveImagesTimeout encodingutil.Duration `json:"remove_images_timeout" yaml:"remove_images_timeout"`
}

func (c Config) Valid() error {
	if len(c.Envs) == 0 {
		return xerrors.New(".Envs is empty")
	}
	presentEnvNames := make(map[string]bool)
	for _, e := range c.Envs {
		if presentEnvNames[e.Name()] {
			return xerrors.Errorf(".Env %s already defined", e.Name())
		}
		presentEnvNames[e.Name()] = true
	}
	if c.KeepImages <= 0 {
		return xerrors.Errorf(".KeepImages should be positive. Got: %d", c.KeepImages)
	}
	return nil
}

func DefaultConfig() Config {
	return Config{
		KeepImages:          64,
		ListImagesTimeout:   encodingutil.FromDuration(time.Second * 5),
		PutImageTimeout:     encodingutil.FromDuration(time.Second * 30),
		RemoveImagesTimeout: encodingutil.FromDuration(time.Second * 30),
	}
}

const imagePrefix = "image-"

func genImageKey(imageFile, imageVersion string) string {
	return fmt.Sprintf(
		"%s%d-r%s%s", imagePrefix, time.Now().Unix(), imageVersion, path.Ext(imageFile))
}

var revReg *regexp.Regexp

func init() {
	revReg = regexp.MustCompile(`^` + imagePrefix + `\d+-r(\w+)`)
}

func getRevFromImageKey(imageKey string) string {
	matches := revReg.FindStringSubmatch(imageKey)
	if len(matches) < 2 {
		return ""
	}
	return matches[1]
}

type Image struct {
	Version  string
	Uploaded time.Time
}

type Env struct {
	bucket              string
	possiblyUnavailable bool
	s3cli               s3.Client
}

func NewEnv(bucket string, s3cli s3.Client, possiblyUnavailable bool) Env {
	return Env{bucket: bucket, s3cli: s3cli, possiblyUnavailable: possiblyUnavailable}
}

type Publisher struct {
	config          Config
	envs            map[string]Env
	l               log.Logger
	unavailableEnvs map[string]bool
}

func New(config Config, envs map[string]Env, l log.Logger) *Publisher {
	return &Publisher{
		config:          config,
		l:               l,
		envs:            envs,
		unavailableEnvs: make(map[string]bool),
	}
}

func NewFromConfig(config Config, l log.Logger) (*Publisher, error) {
	if err := config.Valid(); err != nil {
		return nil, xerrors.Errorf("config invalid: %w", err)
	}
	envs := make(map[string]Env, len(config.Envs))
	for _, e := range config.Envs {
		s3cli, err := http.New(e.S3, l)
		if err != nil {
			return nil, err
		}
		envs[e.Name()] = NewEnv(e.Bucket, s3cli, e.PossiblyUnavailable)
	}

	return New(config, envs, l), nil
}

type imageWithEnv struct {
	Uploaded    time.Time
	PresentEnvs map[string]struct{}
}

func (p *Publisher) fetchImages(ctx context.Context) (map[string]*imageWithEnv, error) {
	type objectsAtEnv struct {
		envName string
		err     error
		objects []s3.Object
	}
	fetched := make(chan objectsAtEnv)
	var wg sync.WaitGroup
	for envName := range p.envs {
		if p.unavailableEnvs[envName] {
			continue
		}
		wg.Add(1)
		go func(envName string) {
			defer wg.Done()
			env := p.envs[envName]

			listCtx, cancel := context.WithTimeout(ctx, p.config.ListImagesTimeout.Duration)
			defer cancel()
			objects, _, err := env.s3cli.ListObjects(listCtx, env.bucket, s3.ListObjectsOpts{
				Prefix: ptr.String(imagePrefix),
			})
			fetched <- objectsAtEnv{
				envName: envName,
				objects: objects,
				err:     err,
			}
		}(envName)
	}

	go func() {
		wg.Wait()
		close(fetched)
	}()

	imagesWithEnvs := make(map[string]*imageWithEnv)
	for fr := range fetched {
		if fr.err != nil {
			if p.envs[fr.envName].possiblyUnavailable {
				p.l.Warn(
					"failed to list objects. Ignore it, cause that env marked as possibly unavailable",
					log.String("s3", fr.envName),
					log.Error(fr.err),
				)
				p.unavailableEnvs[fr.envName] = true
				continue
			}
			return nil, xerrors.Errorf("failed to list objects in %s: %w", fr.envName, fr.err)
		}
		for _, obj := range fr.objects {
			if img, ok := imagesWithEnvs[obj.Key]; ok {
				// treat least LastModified as Updated
				if img.Uploaded.After(obj.LastModified) {
					img.Uploaded = obj.LastModified
				}
				img.PresentEnvs[fr.envName] = struct{}{}
				continue
			}
			imagesWithEnvs[obj.Key] = &imageWithEnv{
				Uploaded:    obj.LastModified,
				PresentEnvs: map[string]struct{}{fr.envName: {}},
			}
		}
	}

	return imagesWithEnvs, nil
}

func (p *Publisher) ListImages(ctx context.Context) ([]Image, error) {
	imagesWithEnvs, err := p.fetchImages(ctx)
	if err != nil {
		return nil, err
	}
	images := make([]Image, 0, len(imagesWithEnvs))
	for imageKey, imgMeta := range imagesWithEnvs {
		imageVersion := getRevFromImageKey(imageKey)
		if imageVersion == "" {
			p.l.Warn("failed get revision from s3 key", log.String("key", imageKey))
			continue
		}

		var notPresentIn []string
		for e := range p.envs {
			if p.unavailableEnvs[e] {
				continue
			}
			if _, ok := imgMeta.PresentEnvs[e]; !ok {
				notPresentIn = append(notPresentIn, e)
			}
		}
		if len(notPresentIn) > 0 {
			p.l.Warn(
				"ignoring image, cause it not present in some envs",
				log.Array("notPresentIn", notPresentIn),
			)
			continue
		}

		images = append(images, Image{
			Version:  imageVersion,
			Uploaded: imgMeta.Uploaded,
		})
	}
	return images, nil
}

func (p *Publisher) PutImage(ctx context.Context, imageFile, imageVersion string) error {
	imageKey := genImageKey(imageFile, imageVersion)

	var putWg sync.WaitGroup
	type putResult struct {
		envName string
		err     error
	}
	putted := make(chan putResult, len(p.envs))
	for envName := range p.envs {
		if p.unavailableEnvs[envName] {
			p.l.Info("Don't attempt to put new image, cause that env is unavailable", log.String("env", envName))
			continue
		}
		putWg.Add(1)
		go func(envName string) {
			defer putWg.Done()
			putted <- putResult{
				envName: envName,
				err:     p.putImageToEnv(ctx, envName, imageKey, imageFile),
			}
		}(envName)
	}
	putWg.Wait()
	close(putted)

	for put := range putted {
		if put.err != nil {
			if p.envs[put.envName].possiblyUnavailable {
				p.l.Warn(
					"Failed to put image. Ignore it, cause that env is possibly unavailable",
					log.String("image file", imageFile),
					log.String("s3", put.envName),
					log.Error(put.err),
				)
				p.unavailableEnvs[put.envName] = true
				continue
			}
			return put.err
		}
	}

	var removeWg errgroup.Group
	for envName := range p.envs {
		if p.unavailableEnvs[envName] {
			p.l.Info("Don't attempt to remove old images, cause that env is unavailable", log.String("env", envName))
			continue
		}
		envName := envName
		removeWg.Go(
			func() error {
				return p.removeOldImagesFromEnv(ctx, imageKey, envName)
			},
		)
	}
	return removeWg.Wait()
}

func (p *Publisher) putImageToEnv(ctx context.Context, envName, imageKey, imageFile string) error {
	file, err := os.Open(imageFile)

	if err != nil {
		return err
	}
	defer func() {
		if err := file.Close(); err != nil {
			p.l.Warn("Failed to close a file", log.String("filename", imageFile), log.Error(err))
		}
	}()

	var putOptions s3.PutObjectOpts

	fi, err := file.Stat()
	if err != nil {
		return xerrors.Errorf("failed to get image file stat: %w", err)
	}
	imageSize := fi.Size()
	putOptions.ContentLength = &imageSize

	p.l.Info(
		"Uploading image to s3",
		log.String("image file", imageFile),
		log.String("s3", envName),
		log.String("key", imageKey),
	)
	putCtx, cancel := context.WithTimeout(ctx, p.config.PutImageTimeout.Duration)
	defer cancel()
	if err := p.envs[envName].s3cli.PutObject(putCtx, p.envs[envName].bucket, imageKey, file, putOptions); err != nil {
		return xerrors.Errorf("failed to put %s image to %s: %w", imageFile, envName, err)
	}

	return nil
}

func (p *Publisher) removeOldImagesFromEnv(ctx context.Context, newImageKey, envName string) error {
	env := p.envs[envName]
	removeCtx, cancel := context.WithTimeout(ctx, p.config.RemoveImagesTimeout.Duration)
	defer cancel()
	images, _, err := env.s3cli.ListObjects(removeCtx, env.bucket, s3.ListObjectsOpts{
		Prefix: ptr.String(imagePrefix),
	})

	if err != nil {
		p.l.Warn(
			"Failed to list images in bucket while attempt to remove old images",
			log.String("s3", envName),
			log.Error(err),
		)
		return nil
	}

	if len(images) > p.config.KeepImages {
		p.l.Infof("There are %d images in %s bucket. We should delete oldest", len(images), env.bucket)
		sort.Slice(images, func(i, j int) bool {
			return images[i].LastModified.Before(images[j].LastModified)
		})
		for i := 0; i < (len(images) - p.config.KeepImages); i++ {
			img := images[i]
			if img.Key == newImageKey {
				return xerrors.Errorf("Attempt to delete currently upload image: %s", newImageKey)
			}
			p.l.Debug("Deleting image", log.String("key", img.Key), log.Time("LastModified", img.LastModified))
			if err := env.s3cli.DeleteObject(removeCtx, env.bucket, img.Key); err != nil {
				p.l.Warn("Failed to delete old image", log.Error(err))
			}
		}
	}
	return nil
}
