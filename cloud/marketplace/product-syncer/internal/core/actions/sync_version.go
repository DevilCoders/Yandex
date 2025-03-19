package actions

import (
	"context"
	"fmt"
	"net/http"
	"strings"
	"time"

	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/aws/session"
	"github.com/aws/aws-sdk-go/service/s3"
	"github.com/aws/aws-sdk-go/service/s3/s3manager"
	"github.com/golang/protobuf/ptypes/any"
	"google.golang.org/protobuf/proto"
	"google.golang.org/protobuf/types/known/anypb"

	"a.yandex-team.ru/cloud/bitbucket/public-api/yandex/cloud/compute/v1"
	"a.yandex-team.ru/cloud/marketplace/pkg/clients/marketplace-private"
	"a.yandex-team.ru/cloud/marketplace/pkg/ctxtools"
	"a.yandex-team.ru/cloud/marketplace/pkg/tracing"
	"a.yandex-team.ru/cloud/marketplace/product-syncer/internal/core/model"
	"a.yandex-team.ru/cloud/marketplace/product-syncer/internal/services/env"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type SyncVersionAction struct {
	*env.Env
}

func NewSyncVersionAction(env *env.Env) *SyncVersionAction {
	return &SyncVersionAction{env}
}

type SyncVersionParams struct {
	Version         model.VersionYaml
	Session         *session.Session
	VersionBucket   string
	PendingFolderID string
	SyncSvc         *s3.S3
}

func (a *SyncVersionAction) Do(ctx context.Context, params SyncVersionParams) error {
	span, spanCtx := tracing.Start(ctx, "SyncProductAction")
	defer span.Finish()

	scopedLogger := ctxtools.LoggerWith(spanCtx,
		log.String("version_id", params.Version.ID),
	)

	// check first if version exists. If not - create compute image before sync
	version, err := a.Adapters().Marketplace().GetVersionByID(spanCtx, params.Version.PublisherID, params.Version.ProductID, params.Version.ID)
	if xerrors.Is(err, marketplace.ErrNotFound) {
		scopedLogger.Info("version does not exist")
	} else if err != nil {
		scopedLogger.Debug("error retrieving version")
		return err
	}
	syncParams := createVersionSyncRequest(params.Version)

	if version == nil {
		// version does not exist, prepare Compute image here and send it into version.source.compute.image_id
		// Get signed S3 link
		req, err := prepareCreateImageRequest(params)
		if err != nil {
			scopedLogger.Debug("error preparing createImage params")
			return err
		}

		op, err := a.Backends().ComputeImage().Create(spanCtx, req)
		if err != nil {
			scopedLogger.Debug("error creating compute image for new version")
			return err
		}
		scopedLogger.Debug("polling create image operation")
		op, err = a.Backends().Operation().WaitOperation(spanCtx, op.Id)
		if err != nil {
			scopedLogger.Debug("error polling create image operation")
			return err
		}

		syncParams.Source.ComputeImage.ImageID, err = UnmarshalCreateImageMetadata(op.Metadata)
		if err != nil {
			scopedLogger.Debug("error unmarshalling create image operation metadata")
			return err
		}
		//	TODO: version does not exist, prepare Compute image here and send it into version.source.compute.image_id
	}

	scopedLogger.Debug("syncing tariff")
	// TODO: sync tariff. For now - ignore tariff in yaml and get it from product
	tariffID, err := a.Adapters().Marketplace().GetFreeProductTariff(spanCtx, params.Version.PublisherID, params.Version.ProductID)
	if err != nil {
		scopedLogger.Debug("error retrieving tariff")
		return err
	}

	// TODO: remove sleep. But without it we get weird read: connection reset by peer errors from python app
	time.Sleep(time.Second * 3)

	scopedLogger.Debug("syncing logo")
	err = uploadVersionLogo(params.Session, params.Version.ID, params.VersionBucket, params.Version.MarketingInfo.LogoSource.Link)
	if err != nil {
		scopedLogger.Debug("error uploading logo to s3", log.Error(err))
		return err
	}
	// TODO: remove sleep. But without it we get weird read: connection reset by peer errors from python app
	time.Sleep(time.Second * 3)

	scopedLogger.Debug("resolving category IDs from names")
	var categoryIDs []string
	for _, cat := range params.Version.CategoryNames {
		category, err := a.Adapters().Marketplace().GetCategoryByID(spanCtx, cat)
		if xerrors.Is(err, marketplace.ErrNotFound) {
			scopedLogger.Info(fmt.Sprintf("category %s does not exist, skipping", cat))
			continue
		} else if err != nil {
			scopedLogger.Debug("error resolving category name", log.Error(err))
			return err
		}
		categoryIDs = append(categoryIDs, category.ID)
		// TODO: remove sleep. But without it we get weird read: connection reset by peer errors from python app
		time.Sleep(time.Second * 3)
	}

	syncParams.CategoryIDs = categoryIDs
	// prepare complex version sync request
	syncParams.TariffID = *tariffID
	syncParams.MarketingInfo.LogoSource = marketplace.LogoSource{
		StorageObject: &marketplace.StorageObject{
			BucketName: params.VersionBucket,
			ObjectName: params.Version.ID,
		},
		VersionID: nil,
		Content:   nil,
	}

	scopedLogger.Debug("syncing version")
	op, err := a.Adapters().Marketplace().SyncVersion(spanCtx, syncParams)
	if err != nil {
		return err
	}
	scopedLogger.Debug("sync version action completed: ", log.String("Operation", op.ID))

	return nil
}

func prepareCreateImageRequest(params SyncVersionParams) (*compute.CreateImageRequest, error) {
	keys := strings.Split(params.Version.Source.ComputeImage.S3Link, "/")
	if len(keys) < 2 {
		return nil, fmt.Errorf("incorrect s3 link in version source %s", params.Version.Source.ComputeImage.S3Link)
	}
	s3req, _ := params.SyncSvc.GetObjectRequest(&s3.GetObjectInput{
		Bucket: aws.String(keys[0]),
		Key:    aws.String(keys[1]),
	})
	urlStr, err := s3req.Presign(3 * time.Hour)
	if err != nil {
		return nil, err
	}
	fmt.Printf("Uri s3 image link: %s\n", urlStr)
	var imgOs compute.Os_Type
	switch params.Version.Payload.ComputeImage.PackageInfo.Os.Family {
	case "linux":
		imgOs = compute.Os_LINUX
	case "windows":
		imgOs = compute.Os_WINDOWS
	default:
		imgOs = compute.Os_LINUX
	}

	return &compute.CreateImageRequest{
		FolderId:    params.PendingFolderID,
		Name:        fmt.Sprintf("%s-%d", params.Version.Payload.ComputeImage.Family, time.Now().Unix()),
		Description: "",
		Family:      params.Version.Payload.ComputeImage.Family,
		ProductIds:  nil,
		Source:      &compute.CreateImageRequest_Uri{Uri: urlStr},
		Os:          &compute.Os{Type: imgOs},
		Pooled:      false,
	}, nil
}

func UnmarshalCreateImageMetadata(data *any.Any) (string, error) {
	var m compute.CreateImageMetadata
	err := anypb.UnmarshalTo(data, &m, proto.UnmarshalOptions{})
	return m.ImageId, err
}

func uploadVersionLogo(sess *session.Session, versionID, versionsBucket, link string) error {
	resp, err := http.Get(link)
	if err != nil {
		return err
	}
	defer resp.Body.Close()

	if resp.StatusCode != http.StatusOK {
		return fmt.Errorf("error downloading source logo: %s", resp.Status)
	}

	uploader := s3manager.NewUploader(sess)
	_, err = uploader.Upload(&s3manager.UploadInput{
		Body:        resp.Body,
		Bucket:      aws.String(versionsBucket),
		ContentType: aws.String("image/svg+xml"),
		Key:         aws.String(versionID + ".svg"),
	})
	if err != nil {
		return err
	}
	return nil
}

func createVersionSyncRequest(params model.VersionYaml) marketplace.SyncVersionParams {
	var interfaces *marketplace.BoundedValue
	var gpus *marketplace.BoundedValue

	// sadly json 'omitempty` has no effect on nested structs
	// so we land some if's to NOT send empty networkInterfaces in request
	if params.Payload.ComputeImage.ResourceSpec.NetworkInterfaces != nil {
		interfaces = &marketplace.BoundedValue{
			Min:         params.Payload.ComputeImage.ResourceSpec.NetworkInterfaces.GetMin(),
			Recommended: params.Payload.ComputeImage.ResourceSpec.NetworkInterfaces.GetRecommended(),
			Max:         params.Payload.ComputeImage.ResourceSpec.NetworkInterfaces.GetMax(),
		}
	}

	if params.Payload.ComputeImage.ResourceSpec.GPU != nil {
		gpus = &marketplace.BoundedValue{
			Min:         params.Payload.ComputeImage.ResourceSpec.GPU.GetMin(),
			Recommended: params.Payload.ComputeImage.ResourceSpec.GPU.GetRecommended(),
			Max:         params.Payload.ComputeImage.ResourceSpec.GPU.GetMax(),
		}
	}

	return marketplace.SyncVersionParams{
		ID:             params.ID,
		ProductID:      params.ProductID,
		PublisherID:    params.PublisherID,
		MarketingInfo:  model.MarketingInfoYamlToRequest(params.MarketingInfo),
		TermsOfService: model.TermsOfServiceYamlToRequest(params),
		Payload: marketplace.Payload{ComputeImage: marketplace.ComputeImagePayload{
			PackageInfo: marketplace.PackageInfo{
				OS: marketplace.Os{
					Version: params.Payload.ComputeImage.PackageInfo.Os.Version,
					Family:  params.Payload.ComputeImage.PackageInfo.Os.Family,
					Name:    params.Payload.ComputeImage.PackageInfo.Os.Name,
				},
				PackageContents: model.PackageInfoYamlToRequest(params.Payload.ComputeImage.PackageInfo.PackageContents),
			},
			ResourceSpec: marketplace.ResourceSpec{
				NetworkInterfaces: interfaces,
				Memory: marketplace.BoundedValue{
					Min:         params.Payload.ComputeImage.ResourceSpec.Memory.GetMin(),
					Recommended: params.Payload.ComputeImage.ResourceSpec.Memory.GetRecommended(),
					Max:         params.Payload.ComputeImage.ResourceSpec.Memory.GetMax(),
				},
				CPU: marketplace.BoundedValue{
					Min:         params.Payload.ComputeImage.ResourceSpec.CPU.GetMin(),
					Recommended: params.Payload.ComputeImage.ResourceSpec.CPU.GetRecommended(),
					Max:         params.Payload.ComputeImage.ResourceSpec.CPU.GetMax(),
				},
				DiskSize: marketplace.BoundedValue{
					Min:         params.Payload.ComputeImage.ResourceSpec.DiskSize.GetMin(),
					Recommended: params.Payload.ComputeImage.ResourceSpec.DiskSize.GetRecommended(),
					Max:         params.Payload.ComputeImage.ResourceSpec.DiskSize.GetMax(),
				},
				ComputePlatforms: params.Payload.ComputeImage.ResourceSpec.ComputePlatforms,
				GPU:              gpus,
				CPUFraction: marketplace.BoundedValue{
					Min:         params.Payload.ComputeImage.ResourceSpec.CPUFraction.GetMin(),
					Recommended: params.Payload.ComputeImage.ResourceSpec.CPUFraction.GetRecommended(),
					Max:         params.Payload.ComputeImage.ResourceSpec.CPUFraction.GetMax(),
				},
			},
			FormID: params.Payload.ComputeImage.FormID,
		}},
		Source: marketplace.Source{ComputeImage: marketplace.ComputeImageSource{
			ImageID: params.Source.ComputeImage.ImageID,
		}},
		Tags: params.Tags,
	}
}
