package sync

import (
	"context"
	"fmt"

	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/aws/credentials"
	"github.com/aws/aws-sdk-go/aws/session"
	"github.com/aws/aws-sdk-go/service/s3"
	"gopkg.in/yaml.v2"

	"a.yandex-team.ru/cloud/marketplace/pkg/ctxtools"
	"a.yandex-team.ru/cloud/marketplace/pkg/logging"
	"a.yandex-team.ru/cloud/marketplace/product-syncer/internal/app/config"
	"a.yandex-team.ru/cloud/marketplace/product-syncer/internal/core/actions"
	"a.yandex-team.ru/cloud/marketplace/product-syncer/internal/core/model"
	"a.yandex-team.ru/cloud/marketplace/product-syncer/internal/services/env"
	xlog "a.yandex-team.ru/library/go/core/log"
)

type Service struct {
	*env.Env
	SyncS3Sess *session.Session
	LogoS3Sess *session.Session

	SyncBucket      string
	LogoBucket      string
	Stand           string
	PendingFolderID string
}

func NewService(env *env.Env, cfg *config.ServiceConfig) (*Service, error) {
	logging.Logger().Debug("initializing sync service")

	if err := cfg.Validate(); err != nil {
		return nil, err
	}

	syncSession, err := session.NewSession(&aws.Config{
		Region: aws.String(cfg.S3SyncConfig.Region),
		Credentials: credentials.NewStaticCredentials(
			cfg.S3SyncConfig.AccessKey,
			cfg.S3SyncConfig.SecretKey,
			""),
		Endpoint:         aws.String(cfg.S3SyncConfig.Endpoint),
		S3ForcePathStyle: aws.Bool(true),
	})
	if err != nil {
		return nil, fmt.Errorf("error configuring sync s3 session")
	}

	logoSession, err := session.NewSession(&aws.Config{
		Region: aws.String(cfg.S3LogoConfig.Region),
		Credentials: credentials.NewStaticCredentials(
			cfg.S3LogoConfig.AccessKey,
			cfg.S3LogoConfig.SecretKey,
			""),
		Endpoint: aws.String(cfg.S3LogoConfig.Endpoint),
	})
	if err != nil {
		return nil, fmt.Errorf("error configuring logo s3 session")
	}

	srv := &Service{
		Env:             env,
		SyncS3Sess:      syncSession,
		LogoS3Sess:      logoSession,
		SyncBucket:      cfg.S3SyncConfig.Bucket,
		LogoBucket:      cfg.S3LogoConfig.Bucket,
		Stand:           cfg.S3SyncConfig.StandPrefix,
		PendingFolderID: cfg.MarketplaceClient.MktPendingImagesFolderID,
	}

	return srv, nil
}

func (s *Service) Run(runCtx context.Context) error {
	scoppedLogger := ctxtools.Logger(runCtx)
	scoppedLogger.Debug("entered sync service")

	syncSvc := s3.New(s.SyncS3Sess)
	if syncSvc == nil {
		return fmt.Errorf("error creating sync s3 svc")
	}

	scoppedLogger.Debug("created s3 service")

	for {
		// - while true:
		//    - read s3 by stand prefix
		//  - for product file in s3:
		//    - sync product
		//    - delete file
		//  - for version file in s3:
		//    - create free tariff if not exist
		//    - sync version
		//    - delete file

		err := s.syncProducts(runCtx, syncSvc)
		if err != nil {
			return err
		}

		err = s.syncVersions(runCtx, syncSvc, s.LogoS3Sess)
		if err != nil {
			return err
		}

		//just temp exit for tests, remove when ready
		scoppedLogger.Debug("nothing to do, exiting")
		//nolint:SA4004
		return nil
	}

}

// should split errors here into critical and acceptable
// i.e. yaml parse error in case of 1 incorrect product is not the end of the world
// but continued 500s from MKT API is something critical
func (s *Service) syncProducts(runCtx context.Context, syncSvc *s3.S3) error {
	scoppedLogger := ctxtools.Logger(runCtx)
	scoppedLogger.Debug("creating input")

	input := &s3.ListObjectsV2Input{
		Bucket: aws.String(s.SyncBucket),
		Prefix: aws.String(fmt.Sprintf("%s/%s", s.Stand, "products")),
	}

	scoppedLogger.Debug("listing objects in products")
	resp, err := syncSvc.ListObjectsV2(input)

	if err != nil {
		scoppedLogger.Debug("error listing objects in products", xlog.Error(err))
		return err
	}

	// sync each product
	for _, item := range resp.Contents {
		result, err := syncSvc.GetObject(&s3.GetObjectInput{
			Key:    item.Key,
			Bucket: aws.String(s.SyncBucket),
		})
		if err != nil {
			scoppedLogger.Debug("error getting object from products", xlog.Error(err))
			return err
		}

		var productYaml model.ProductYaml
		err = yaml.NewDecoder(result.Body).Decode(&productYaml)
		if err != nil {
			// TODO: maybe we should delete incorrect yaml in s3 aswell
			scoppedLogger.Debug("error parsing product yaml", xlog.Error(err))
			return err
		}

		err = actions.NewSyncProductAction(s.Env).Do(runCtx, productYaml)
		if err != nil {
			return err
		}

	}
	return nil
}

func (s *Service) syncVersions(runCtx context.Context, syncSvc *s3.S3, logoSess *session.Session) error {
	scoppedLogger := ctxtools.Logger(runCtx)
	scoppedLogger.Debug("creating s3 versions input")

	input := &s3.ListObjectsV2Input{
		Bucket: aws.String(s.SyncBucket),
		Prefix: aws.String(fmt.Sprintf("%s/%s", s.Stand, "versions")),
	}

	scoppedLogger.Debug("listing objects in versions")
	resp, err := syncSvc.ListObjectsV2(input)

	if err != nil {
		scoppedLogger.Debug("error listing objects in versions", xlog.Error(err))
		return err
	}

	// sync each version
	for _, item := range resp.Contents {
		result, err := syncSvc.GetObject(&s3.GetObjectInput{
			Key:    item.Key,
			Bucket: aws.String(s.SyncBucket),
		})
		if err != nil {
			scoppedLogger.Debug("error getting object from versions", xlog.Error(err))
			return err
		}

		defer result.Body.Close()

		var versionYaml model.VersionYaml
		err = yaml.NewDecoder(result.Body).Decode(&versionYaml)

		if err != nil {
			// TODO: maybe we should delete incorrect yaml in s3 aswell
			scoppedLogger.Debug("error parsing version yaml", xlog.Error(err))
			continue
		}
		err = versionYaml.Validate()
		if err != nil {
			scoppedLogger.Debug("error validating version yaml", xlog.Error(err))
			continue
		}

		if versionYaml.ID == "" {
			scoppedLogger.Error("some weird error parsing yaml, skip this version: ", xlog.String("s3 key", *item.Key))
			continue
		}

		err = actions.NewSyncVersionAction(s.Env).Do(runCtx, actions.SyncVersionParams{
			Version:         versionYaml,
			Session:         logoSess,
			VersionBucket:   s.LogoBucket,
			PendingFolderID: s.PendingFolderID,
			SyncSvc:         syncSvc,
		})
		if err != nil {
			scoppedLogger.Error("error syncing version: ", xlog.Error(err))
			return err
		}

	}
	return nil
}
