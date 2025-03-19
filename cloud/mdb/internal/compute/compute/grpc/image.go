package grpc

import (
	"context"

	"github.com/golang/protobuf/ptypes"
	"google.golang.org/genproto/protobuf/field_mask"
	"google.golang.org/grpc/credentials"

	cloudCompute "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/compute/v1"
	internalCompute "a.yandex-team.ru/cloud/mdb/internal/compute/compute"
	"a.yandex-team.ru/cloud/mdb/internal/compute/operations"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var _ internalCompute.ImageService = &ImageServiceClient{}

type ImageServiceClient struct {
	imageAPI cloudCompute.ImageServiceClient
}

func NewImageServiceClient(ctx context.Context, target, userAgent string, cfg grpcutil.ClientConfig, creds credentials.PerRPCCredentials, l log.Logger) (*ImageServiceClient, error) {
	conn, err := grpcutil.NewConn(ctx, target, userAgent, cfg, l, grpcutil.WithClientCredentials(creds))
	if err != nil {
		return nil, xerrors.Errorf("connecting to image API at %q: %w", target, err)
	}

	return &ImageServiceClient{
		imageAPI: cloudCompute.NewImageServiceClient(conn),
	}, nil
}

func (i *ImageServiceClient) Get(ctx context.Context, imageID string) (internalCompute.Image, error) {
	resp, err := i.imageAPI.Get(ctx, &cloudCompute.GetImageRequest{
		ImageId: imageID,
	})
	if err != nil {
		return internalCompute.Image{}, err
	}
	return imageFromGRPC(resp)
}

func (i *ImageServiceClient) ListOperations(ctx context.Context, opts internalCompute.ListOperationsOpts) ([]operations.Operation, error) {
	resp, err := i.imageAPI.ListOperations(ctx, &cloudCompute.ListImageOperationsRequest{
		ImageId: opts.ImageID,
	})
	if err != nil {
		return nil, err
	}
	var result []operations.Operation
	for _, o := range resp.Operations {
		resOp, err := operations.FromGRPC(o)
		if err != nil {
			return nil, err
		}
		result = append(result, resOp)
	}
	return result, nil
}

func imageFromGRPC(image *cloudCompute.Image) (internalCompute.Image, error) {
	resI := internalCompute.Image{
		ID:          image.GetId(),
		FolderID:    image.GetFolderId(),
		Name:        image.GetName(),
		Description: image.GetDescription(),
		Labels:      image.GetLabels(),
		Family:      image.GetFamily(),
		StorageSize: image.GetStorageSize(),
		MinDiskSize: image.GetMinDiskSize(),
		ProductIDs:  image.GetProductIds(),
		Status:      image.GetStatus().String(),
	}
	switch image.GetOs().GetType() {
	case cloudCompute.Os_WINDOWS:
		resI.OS = internalCompute.OSWindows
	case cloudCompute.Os_LINUX:
		resI.OS = internalCompute.OSLinux
	}
	if createdAt, err := ptypes.Timestamp(image.GetCreatedAt()); err != nil {
		return resI, xerrors.Errorf("createdAt: %w", err)
	} else {
		resI.CreatedAt = createdAt
	}
	return resI, nil
}

func (i *ImageServiceClient) List(ctx context.Context, folderID string, opts internalCompute.ListImagesOpts) (internalCompute.ListImagesResponse, error) {
	resp, err := i.imageAPI.List(ctx, &cloudCompute.ListImagesRequest{
		FolderId:  folderID,
		PageSize:  opts.PageSize,
		PageToken: opts.PageToken,
		Filter:    opts.Filter,
		Name:      opts.Name,
	})
	if err != nil {
		return internalCompute.ListImagesResponse{}, err
	}
	result := internalCompute.ListImagesResponse{}
	result.NextPageToken = resp.NextPageToken
	for _, i := range resp.Images {
		intImage, err := imageFromGRPC(i)
		if err != nil {
			return result, err
		}
		result.Images = append(result.Images, intImage)
	}
	return result, nil
}

func (i *ImageServiceClient) Create(ctx context.Context, folderID, name string, source internalCompute.CreateImageSource, os internalCompute.OS, opts internalCompute.CreateImageOpts) (operations.Operation, error) {
	req := &cloudCompute.CreateImageRequest{
		FolderId:    folderID,
		Name:        name,
		Description: opts.Description,
		Labels:      opts.Labels,
		Family:      opts.Family,
		MinDiskSize: opts.MinDiskSize,
		ProductIds:  opts.ProductIds,
		Pooled:      true,
		Os: &cloudCompute.Os{
			Type: cloudCompute.Os_LINUX,
		},
	}
	if os == internalCompute.OSWindows {
		req.Os.Type = cloudCompute.Os_WINDOWS
	}
	switch v := source.(type) {
	case internalCompute.CreateImageSourceDiskID:
		req.Source = &cloudCompute.CreateImageRequest_DiskId{DiskId: string(v)}
	case internalCompute.CreateImageSourceImageID:
		req.Source = &cloudCompute.CreateImageRequest_ImageId{ImageId: string(v)}
	case internalCompute.CreateImageSourceSnapshotID:
		req.Source = &cloudCompute.CreateImageRequest_SnapshotId{SnapshotId: string(v)}
	case internalCompute.CreateImageSourceURI:
		req.Source = &cloudCompute.CreateImageRequest_Uri{Uri: string(v)}
	}

	cloudOp, err := i.imageAPI.Create(ctx, req)
	if err != nil {
		return operations.Operation{}, err
	}
	op, err := operations.FromGRPC(cloudOp)
	if err != nil {
		return operations.Operation{}, xerrors.Errorf("parse operation: %w", err)
	}
	return op, nil
}

func (i *ImageServiceClient) UpdateLabels(ctx context.Context, imageID string, labels map[string]string) (operations.Operation, error) {
	req := &cloudCompute.UpdateImageRequest{
		ImageId: imageID,
		Labels:  labels,
		UpdateMask: &field_mask.FieldMask{
			Paths: []string{
				"labels",
			}},
	}

	cloudOp, err := i.imageAPI.Update(ctx, req)
	if err != nil {
		return operations.Operation{}, err
	}
	op, err := operations.FromGRPC(cloudOp)
	if err != nil {
		return operations.Operation{}, xerrors.Errorf("parse operation: %w", err)
	}
	return op, nil
}

func (i *ImageServiceClient) Delete(ctx context.Context, imageID string) (operations.Operation, error) {
	req := &cloudCompute.DeleteImageRequest{
		ImageId: imageID,
	}
	cloudOp, err := i.imageAPI.Delete(ctx, req)
	if err != nil {
		return operations.Operation{}, xerrors.Errorf("request image %q delete", imageID)
	}
	op, err := operations.FromGRPC(cloudOp)
	if err != nil {
		return operations.Operation{}, xerrors.Errorf("parse operation: %w", err)
	}
	return op, nil
}
