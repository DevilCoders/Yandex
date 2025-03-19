package compute

import (
	"context"
	"fmt"
	"sort"
	"strings"
	"time"

	"github.com/blang/semver/v4"

	"a.yandex-team.ru/cloud/mdb/image-releaser/internal/images"
	"a.yandex-team.ru/cloud/mdb/image-releaser/pkg/checker"
	"a.yandex-team.ru/cloud/mdb/internal/compute/compute"
	"a.yandex-team.ru/cloud/mdb/internal/httputil"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const computePrefixTemplate = "dbaas-%s-image"
const userAgent = "mdb-image-releaser"
const createOperationName = "create compute image"
const (
	readyStatus    = "READY"
	errorStatus    = "ERROR"
	creatingStatus = "CREATING"
)

type Compute struct {
	s3urlPrefix             string
	destinationImgAPI       compute.ImageService
	destinationOperationAPI compute.OperationService
	srcImg                  lastImageProvider
	l                       log.Logger
}

func (c *Compute) DestinationAge(ctx context.Context, imageName string, destinationFolder string, now time.Time) (time.Duration, error) {
	img, err := latestInFolder(ctx, c.destinationImgAPI, destinationFolder, imageName)
	if err != nil {
		return 0, err
	}
	return now.Sub(img.CreatedAt), nil
}

func dbaasImagesInFolder(ctx context.Context, computeAPI compute.ImageService, folder, imageName string, withStatus ...string) ([]compute.Image, error) {
	imagePrefix := fmt.Sprintf(computePrefixTemplate, imageName)
	status := make(map[string]bool, len(withStatus))
	for _, s := range withStatus {
		status[s] = true
	}
	computeImages, err := allImagesInFolder(ctx, computeAPI, folder)
	if err != nil {
		return nil, err
	}
	return filterImages(computeImages, imagePrefix, status), nil
}

func allImagesInFolder(ctx context.Context, computeAPI compute.ImageService, folder string) ([]compute.Image, error) {
	imgs := make([]compute.Image, 0)
	var pageToken string
	for {
		resp, err := computeAPI.List(ctx, folder, compute.ListImagesOpts{
			PageToken: pageToken,
		})
		if err != nil {
			return nil, xerrors.Errorf("compute image listing: %w", err)
		}

		imgs = append(imgs, resp.Images...)
		pageToken = resp.NextPageToken
		if pageToken == "" {
			break
		}
	}
	return imgs, nil
}

func latestInFolder(ctx context.Context, computeAPI compute.ImageService, folder string, imageName string) (compute.Image, error) {
	imgs, err := dbaasImagesInFolder(ctx, computeAPI, folder, imageName, readyStatus)
	if err != nil {
		return compute.Image{}, err
	}
	latestImg, err := latest(imgs)
	if err != nil {
		return compute.Image{}, images.ErrLastNotFound.Wrap(xerrors.Errorf("get image for %q in folder %s: %w", imageName, folder, err))
	}
	return latestImg, nil
}

func latest(images []compute.Image) (compute.Image, error) {
	var img compute.Image

	for _, i := range images {
		if i.CreatedAt.After(img.CreatedAt) {
			img = i
		}
	}
	if img.ID == "" {
		return img, xerrors.New("no image")
	}
	return img, nil
}

func takeOldestAfterN(images []compute.Image, n int) []compute.Image {
	sort.Slice(images, func(i, j int) bool {
		return images[i].CreatedAt.After(images[j].CreatedAt)
	})
	var ret []compute.Image
	for i, img := range images {
		if i >= n {
			ret = append(ret, img)
		}
	}
	return ret
}

func filterImages(computeImages []compute.Image, filterPrefix string, withStatus map[string]bool) []compute.Image {
	imgs := make([]compute.Image, 0)
	for _, img := range computeImages {
		if (len(withStatus) == 0 || withStatus[img.Status]) && strings.HasPrefix(img.Name, filterPrefix) {
			imgs = append(imgs, img)
		}
	}
	return imgs
}

func getLatestVersion(computeImages []compute.Image, versionPrefix string, family string) semver.Version {
	var imageVersion semver.Version
	for _, computeImage := range computeImages {
		if family != "" && computeImage.Family != family {
			continue
		}
		if version, ok := computeImage.Labels["version"]; ok {
			if versionPrefix != "" && !strings.HasPrefix(version, versionPrefix) {
				continue
			}
			parsedVersion, err := semver.Make(version)
			if err != nil {
				continue
			}
			if parsedVersion.Compare(imageVersion) > 0 {
				imageVersion = parsedVersion
			}
		}
	}
	return imageVersion
}

func setImageVersionInLabel(ctx context.Context, imageService compute.ImageService, operationService compute.OperationService, imageID string, version string, isForce bool) error {
	computeImage, err := imageService.Get(ctx, imageID)
	if _, ok := computeImage.Labels["version"]; ok {
		if !isForce {
			return xerrors.Errorf("image %s already have assigned version", err)
		}
	}
	if computeImage.Labels == nil {
		computeImage.Labels = make(map[string]string)
	}
	computeImage.Labels["version"] = version
	resp, err := imageService.UpdateLabels(ctx, imageID, computeImage.Labels)
	if err != nil {
		return err
	}
	_, err = operationService.GetDone(ctx, resp.ID)
	return err
}

func (c *Compute) Release(ctx context.Context, imageName string, os images.OS, productIDs []string, poolSize int, destinationFolder string, checks []checker.Checker) error {
	testImg, err := c.srcImg.last(ctx, imageName)
	if err != nil {
		return xerrors.Errorf("get latest source image: %w", err)
	}
	c.l.Infof("latest source image is %s", testImg.Name)
	if err := c.performChecks(ctx, checks, testImg.CreatedAt); err != nil {
		return images.ErrUnstable.Wrap(err)
	}
	return c.imageReleased(ctx, testImg, poolSize, destinationFolder, os, productIDs)
}

func (c *Compute) ReleaseDataproc(ctx context.Context, imageID string, version string, versionPrefix string, imageFamily string, isForce bool, folderID string) (string, error) {
	computeImages, err := allImagesInFolder(ctx, c.destinationImgAPI, folderID)
	if err != nil || len(computeImages) == 0 {
		return "", xerrors.Errorf("Can not find images in folder %s: %w", folderID, err)
	}

	if version == "" {
		latestVersion := getLatestVersion(computeImages, versionPrefix, imageFamily)
		if latestVersion.Major == 0 && latestVersion.Minor == 0 && latestVersion.Patch == 0 {
			return "", xerrors.Errorf("Can not find last Dataproc image version in folder %s", folderID)
		}
		err = latestVersion.IncrementPatch()
		if err != nil {
			return "", err
		}
		version = latestVersion.String()
	}
	err = setImageVersionInLabel(ctx, c.destinationImgAPI, c.destinationOperationAPI, imageID, version, isForce)
	if err != nil {
		return "", err
	}

	return version, nil
}

func (c *Compute) performChecks(ctx context.Context, checks []checker.Checker, since time.Time) error {
	for _, check := range checks {
		err := check.IsStable(ctx, since, time.Now())
		if err != nil {
			return err
		}
	}
	return nil
}

func (c *Compute) imageReleased(ctx context.Context, testImg image, poolSize int, destinationFolder string, os images.OS, productIDs []string) error {
	imgID, err := c.imagePresent(ctx, testImg, destinationFolder, os, productIDs)
	if err != nil {
		return err
	}
	c.l.Infof("%s is present: %s", testImg.Name, imgID)
	return nil
}

func (c *Compute) imagePresent(ctx context.Context, srcImg image, destinationFolder string, os images.OS, productIDs []string) (string, error) {
	img, err := c.destinationImgWithName(ctx, srcImg.Name, destinationFolder)
	if err == nil {
		switch img.Status {
		case readyStatus:
			return img.ID, nil
		case creatingStatus:
			c.l.Info("image is already creating, waiting to finish")
			waitErr := c.waitCreating(ctx, img.ID)
			if waitErr != nil {
				return "", waitErr
			}
			return img.ID, nil
		default:
			return "", xerrors.Errorf("image is not created, operation status: %s, %s", img.Status, img.Description)
		}
	}
	if !xerrors.Is(err, errNoImageWithName) {
		return "", err
	}

	return c.createImage(ctx, srcImg, destinationFolder, os, productIDs)
}

func (c *Compute) createImage(ctx context.Context, testImg image, destinationFolder string, os images.OS, productIDs []string) (string, error) {
	imageName := testImg.Name
	c.l.Infof("creating image %s", imageName)
	uri, err := httputil.JoinURL(c.s3urlPrefix, fmt.Sprintf("%s.img", imageName))
	if err != nil {
		return "", xerrors.Errorf("join s3 url: %w", err)
	}
	if len(productIDs) == 0 {
		productIDs = testImg.ProductIDs
	}
	op, err := c.destinationImgAPI.Create(ctx, destinationFolder, imageName, compute.CreateImageSourceURI(uri), os.ComputeOS(), compute.CreateImageOpts{
		Description: testImg.Description,
		Labels:      testImg.Labels,
		Family:      testImg.Family,
		MinDiskSize: testImg.MinDiskSize,
		ProductIds:  productIDs,
	})
	if err != nil {
		return "", xerrors.Errorf("image create: %w", err)
	}

	if err := c.operationSucceeded(ctx, op.ID, createOperationName); err != nil {
		return "", err
	}
	prodImgID, err := c.destinationImgWithName(ctx, imageName, destinationFolder)
	if err != nil {
		return "", xerrors.Errorf("operation has succeeded, but there is no image with name %s: %w", imageName, err)
	}
	return prodImgID.ID, nil
}

func (c *Compute) waitCreating(ctx context.Context, imgID string) error {
	ops, err := c.destinationImgAPI.ListOperations(ctx, compute.ListOperationsOpts{ImageID: imgID})
	if err != nil {
		return xerrors.Errorf("get CREATING operation for %s image: %w", imgID, err)
	}
	if len(ops) < 1 {
		return xerrors.Errorf("CREATING operation for %s image not found", imgID)
	}
	return c.operationSucceeded(ctx, ops[0].ID, createOperationName)
}

func (c *Compute) operationSucceeded(ctx context.Context, operationID string, operationName string) error {
	c.l.Infof("waiting for %s operation %s to complete", operationName, operationID)
	doneOp, err := c.destinationOperationAPI.GetDone(ctx, operationID)
	if err != nil {
		return xerrors.Errorf("get done %s operation: %w", operationName, err)
	}
	if doneOp.Error != nil {
		return xerrors.Errorf("%s operation failed: %w", operationName, doneOp.Error)
	}
	c.l.Infof("%s operation finished with result: %s", operationName, doneOp.Result)
	return nil
}

var errNoImageWithName = xerrors.NewSentinel("no image with name")

func (c *Compute) destinationImgWithName(ctx context.Context, imgName string, destinationFolder string) (compute.Image, error) {
	listImagesResponse, err := c.destinationImgAPI.List(ctx, destinationFolder, compute.ListImagesOpts{
		Name: imgName,
	})
	if err != nil {
		return compute.Image{}, xerrors.Errorf("list new image: %w", err)
	}
	if len(listImagesResponse.Images) < 1 {
		return compute.Image{}, errNoImageWithName.Wrap(xerrors.Errorf("image %s not found", imgName))
	}
	return listImagesResponse.Images[0], nil
}

func (c *Compute) deleteImage(ctx context.Context, id string) error {
	op, err := c.destinationImgAPI.Delete(ctx, id)
	if err != nil {
		return xerrors.Errorf("%q image delete failed: %w", id, err)
	}
	if op.Error != nil {
		return xerrors.Errorf("%q image delete operation failed: %s", id, err)
	}
	return nil
}

func (c *Compute) Cleanup(ctx context.Context, imageName string, keepImages int, destinationFolder string) error {
	imgs, err := dbaasImagesInFolder(ctx, c.destinationImgAPI, destinationFolder, imageName)
	var ready []compute.Image
	var forDelete []compute.Image
	if err != nil {
		return xerrors.Errorf("get images for cleanup failed: %w", err)
	}
	for _, img := range imgs {
		switch img.Status {
		case errorStatus:
			forDelete = append(forDelete, img)
		case readyStatus:
			ready = append(ready, img)
		}
	}
	if len(ready) == 0 {
		c.l.Warnf("there are no ready %s images", imageName)
	}
	c.l.Debugf("there are %d images for delete", len(forDelete))
	var deleteError error
	for _, img := range append(forDelete, takeOldestAfterN(ready, keepImages)...) {
		if err := c.deleteImage(ctx, img.ID); err != nil {
			c.l.Errorf("image delete failed: %s", err)
			deleteError = err
			continue
		}
		c.l.Debugf("image %+v deleted successfully", img)
	}
	return deleteError
}
