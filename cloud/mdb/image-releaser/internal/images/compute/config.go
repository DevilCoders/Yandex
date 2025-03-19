package compute

import (
	"context"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/app"
	"a.yandex-team.ru/cloud/mdb/internal/compute/compute"
	"a.yandex-team.ru/cloud/mdb/internal/compute/compute/grpc"
	"a.yandex-team.ru/cloud/mdb/internal/compute/iam"
	iamgrpc "a.yandex-team.ru/cloud/mdb/internal/compute/iam/grpc"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil"
	"a.yandex-team.ru/cloud/mdb/internal/retry"
	s3http "a.yandex-team.ru/cloud/mdb/internal/s3/http"
	"a.yandex-team.ru/cloud/mdb/internal/secret"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Config struct {
	Destination DestinationConf `json:"destination" yaml:"destination"`
	Source      SourceConf      `json:"source" yaml:"source"`
	// Use S3 when you don't have access to testing compute api
	S3 S3Conf `json:"s3" yaml:"s3"`
	// OAuthToken is optional and is useful if you are running locally
	OAuthToken  secret.String `json:"oauth,omitempty" yaml:"oauth,omitempty"`
	S3URLPrefix string        `json:"s3_url_prefix" yaml:"s3_url_prefix"`
}

type DestinationConf struct {
	// ServiceAccount in not used when OAuth is provided
	ServiceAccount   app.ServiceAccountConfig `json:"service_account" yaml:"service_account"`
	OperationBackoff retry.Config             `json:"operation_backoff" yaml:"operation_backoff"`

	ComputeURL  string                `json:"compute_url" yaml:"compute_url"`
	ComputeGRPC grpcutil.ClientConfig `json:"compute_client_conf" yaml:"compute_client_conf"`

	TokenServiceURL  string                `json:"token_service_url" yaml:"token_service_url"`
	TokenServiceGRPC grpcutil.ClientConfig `json:"token_client_conf" yaml:"token_client_conf"`
}

type SourceConf struct {
	// ServiceAccount in not used when OAuth is provided
	ServiceAccount app.ServiceAccountConfig `json:"service_account" yaml:"service_account"`
	FolderID       string                   `json:"folder" yaml:"folder"`

	ComputeURL  string                `json:"compute_url" yaml:"compute_url"`
	ComputeGRPC grpcutil.ClientConfig `json:"compute_client_conf" yaml:"compute_client_conf"`

	TokenServiceURL  string                `json:"token_service_url" yaml:"token_service_url"`
	TokenServiceGRPC grpcutil.ClientConfig `json:"token_client_conf" yaml:"token_client_conf"`
}

type S3Conf struct {
	S3           s3http.Config `json:"client" yaml:"client"`
	Bucket       string        `json:"bucket" yaml:"bucket"`
	ImagePattern string        `json:"image_pattern" yaml:"image_pattern"`
}

func DefaultConfig() Config {
	return Config{
		S3URLPrefix: "https://storage.yandexcloud.net/a2bd1a42-69fd-451a-8dee-47ba04fbacbf",

		Destination: DestinationConf{
			ComputeGRPC:      grpcutil.DefaultClientConfig(),
			TokenServiceGRPC: grpcutil.DefaultClientConfig(),
			OperationBackoff: retry.Config{
				InitialInterval: 10 * time.Second,
				MaxElapsedTime:  25 * time.Minute,
			},
		},
		Source: SourceConf{
			ComputeGRPC:      grpcutil.DefaultClientConfig(),
			TokenServiceGRPC: grpcutil.DefaultClientConfig(),
		},
	}
}

func NewCompute(conf Config, l log.Logger) (*Compute, error) {
	oauth := conf.OAuthToken.Unmask()

	if !conf.Destination.ServiceAccount.FromEnv("DESTINATION_SA") {
		l.Info("DESTINATION_SA_PRIVATE_KEY is not set in env")
	}
	prodCreds, err := credentials(conf.Destination.ServiceAccount, conf.Destination.TokenServiceURL, conf.Destination.TokenServiceGRPC, oauth, l)
	if err != nil {
		return nil, xerrors.Errorf("destination credentials: %w", err)
	}

	prodImgAPI, err := imageAPI(conf.Destination.ComputeURL, conf.Destination.ComputeGRPC, prodCreds, l)
	if err != nil {
		return nil, xerrors.Errorf("destination image client init: %w", err)
	}
	prodOperationAPI, err := operationAPI(conf.Destination.ComputeURL, conf.Destination.ComputeGRPC, prodCreds, conf.Destination.OperationBackoff, l)
	if err != nil {
		return nil, xerrors.Errorf("destination compute operations client init: %w", err)
	}

	var lastImageProvider lastImageProvider
	if conf.Source.FolderID != "" {
		preprodCreds, err := credentials(conf.Source.ServiceAccount, conf.Source.TokenServiceURL, conf.Source.TokenServiceGRPC, oauth, l)
		if err != nil {
			return nil, xerrors.Errorf("source credentials: %w", err)
		}
		preprodImgAPI, err := imageAPI(conf.Source.ComputeURL, conf.Source.ComputeGRPC, preprodCreds, l)
		if err != nil {
			return nil, xerrors.Errorf("source image client init: %w", err)
		}
		lastImageProvider = &computeProvider{
			preprodImgAPI: preprodImgAPI,
			preprodFolder: conf.Source.FolderID,
		}
	} else if conf.S3.Bucket != "" {
		if !conf.S3.S3.SecretKey.FromEnv("S3_SECRET_KEY") {
			l.Infof("%s is empty", "S3_SECRET_KEY")
		}
		s3, err := s3http.New(conf.S3.S3, l)
		if err != nil {
			return nil, xerrors.Errorf("S3 client init: %w", err)
		}
		lastImageProvider = &s3Provider{
			s3:           s3,
			bucket:       conf.S3.Bucket,
			imagePattern: conf.S3.ImagePattern,
		}
	} else {
		return nil, xerrors.New("invalid config: no testing image provider")
	}

	c := &Compute{
		srcImg:                  lastImageProvider,
		destinationImgAPI:       prodImgAPI,
		destinationOperationAPI: prodOperationAPI,
		l:                       l,
		s3urlPrefix:             conf.S3URLPrefix,
	}
	return c, nil
}

func credentials(serviceAccount app.ServiceAccountConfig, tokenServiceURL string, tokenServiceGRPC grpcutil.ClientConfig, oauth string, l log.Logger) (iam.CredentialsService, error) {
	tokenService, err := iamgrpc.NewTokenServiceClient(context.Background(), tokenServiceURL, userAgent, tokenServiceGRPC, &grpcutil.PerRPCCredentialsStatic{}, l)
	if err != nil {
		return nil, err
	}
	var creds iam.CredentialsService
	if oauth != "" {
		creds = tokenService.FromOauthCredentials(oauth)
	} else {
		creds = tokenService.ServiceAccountCredentials(iam.ServiceAccount{
			ID:    serviceAccount.ID,
			KeyID: serviceAccount.KeyID.Unmask(),
			Token: []byte(serviceAccount.PrivateKey.Unmask()),
		})
	}
	return creds, nil
}

func imageAPI(computeURL string, computeGRPCCOnf grpcutil.ClientConfig, creds iam.CredentialsService, l log.Logger) (compute.ImageService, error) {
	imageAPI, err := grpc.NewImageServiceClient(context.Background(), computeURL, userAgent, computeGRPCCOnf, creds, l)
	if err != nil {
		return nil, err
	}
	return imageAPI, nil
}

func operationAPI(computeURL string, computeGRPCCOnf grpcutil.ClientConfig, creds iam.CredentialsService, retryConf retry.Config, l log.Logger) (compute.OperationService, error) {
	operationAPI, err := grpc.NewOperationServiceClient(context.Background(), computeURL, userAgent, computeGRPCCOnf, creds, retryConf, l)
	if err != nil {
		return nil, err
	}
	return operationAPI, nil
}
