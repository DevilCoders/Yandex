package resourceservice

import (
	"context"
	"fmt"
	"math/rand"
	"regexp"

	"a.yandex-team.ru/cdn/cloud_api/pkg/errors"
	"a.yandex-team.ru/cdn/cloud_api/pkg/model"
	"a.yandex-team.ru/cdn/cloud_api/pkg/storage"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

func (s *Service) CreateResource(ctx context.Context, params *model.CreateResourceParams) (*model.Resource, errors.ErrorResult) {
	ctxlog.Info(ctx, s.Logger, "create resource", log.Sprintf("params", "%+v", params))

	params.Options = model.SetResourceOptionsDefaults(params.Options)

	err := params.Validate()
	if err != nil {
		return nil, errors.WrapError("validate params", errors.ValidationError, err)
	}

	var originsGroup *storage.OriginsGroupEntity
	var resultError errors.ErrorResult

	// TODO: transaction

	variant := params.OriginVariant
	switch {
	case variant.GroupID != 0:
		// get group
		originsGroup, resultError = s.Storage.GetOriginsGroupByID(ctx, variant.GroupID, false)
		if resultError != nil {
			return nil, resultError.Wrap("get origins group")
		}
	case variant.Source != nil:
		// create group
		entity := originsGroupEntityFromSource(params.FolderID, variant.Source)
		originsGroup, resultError = s.Storage.CreateOriginsGroup(ctx, entity)
		if resultError != nil {
			return nil, resultError.Wrap("create origins group")
		}
	}

	resourceID, err := s.ResourceGenerator.ResourceID()
	if err != nil {
		return nil, errors.WrapError("generate resource id", errors.InternalError, err)
	}
	cname := s.ResourceGenerator.Cname(resourceID)

	entity := resourceEntityForSave(resourceID, originsGroup.EntityID, cname, params)

	createdEntity, resultError := s.Storage.CreateResource(ctx, entity)
	if resultError != nil {
		return nil, resultError.Wrap("create resource entity")
	}

	if s.AutoActivateEntities {
		resultError = s.Storage.ActivateOriginsGroup(ctx, originsGroup.EntityID, originsGroup.EntityVersion)
		if resultError != nil {
			return nil, resultError.Wrap("activate origins group")
		}

		resultError = s.Storage.ActivateResource(ctx, createdEntity.EntityID, createdEntity.EntityVersion)
		if resultError != nil {
			return nil, resultError.Wrap("activate resource")
		}
	}

	resource, err := makeResource(createdEntity)
	if err != nil {
		return nil, errors.WrapError("make resource", errors.InternalError, err)
	}

	return resource, nil
}

func (s *Service) GetResource(ctx context.Context, params *model.GetResourceParams) (*model.Resource, errors.ErrorResult) {
	ctxlog.Info(ctx, s.Logger, "get resource", log.Sprintf("params", "%+v", params))

	err := params.Validate()
	if err != nil {
		return nil, errors.WrapError("validate params", errors.ValidationError, err)
	}

	entity, resultError := s.Storage.GetResourceByID(ctx, params.ResourceID)
	if resultError != nil {
		return nil, resultError.Wrap("get resource entity")
	}

	resource, err := makeResource(entity)
	if err != nil {
		return nil, errors.WrapError("make resource", errors.InternalError, err)
	}

	return resource, nil
}

// TODO: refactor
func (s *Service) GetAllResources(ctx context.Context, params *model.GetAllResourceParams) ([]*model.Resource, errors.ErrorResult) {
	ctxlog.Info(ctx, s.Logger, "get all resources", log.Sprintf("params", "%+v", params))

	var entities []*storage.ResourceEntity
	var errorResult errors.ErrorResult
	if params.FolderID == nil {
		entities, errorResult = s.Storage.GetAllResources(ctx, makePage(params.Page))
	} else {
		entities, errorResult = s.Storage.GetAllResourcesByFolderID(ctx, *params.FolderID, makePage(params.Page))
	}
	if errorResult != nil {
		return nil, errorResult.Wrap("get all resource entities")
	}

	resources, err := makeResources(entities)
	if err != nil {
		return nil, errors.WrapError("make resources", errors.InternalError, err)
	}

	return resources, nil
}

func (s *Service) UpdateResource(ctx context.Context, params *model.UpdateResourceParams) (*model.Resource, errors.ErrorResult) {
	ctxlog.Info(ctx, s.Logger, "update resource", log.Sprintf("params", "%+v", params))

	err := params.Validate()
	if err != nil {
		return nil, errors.WrapError("validate params", errors.ValidationError, err)
	}

	updateParams := resourceParamsForUpdate(params)

	updatedEntity, errorResult := s.Storage.UpdateResource(ctx, updateParams)
	if errorResult != nil {
		return nil, errorResult.Wrap("update resource entity")
	}

	if s.AutoActivateEntities {
		errorResult = s.Storage.ActivateResource(ctx, updatedEntity.EntityID, updatedEntity.EntityVersion)
		if errorResult != nil {
			return nil, errorResult.Wrap("activate resource")
		}
	}

	resource, err := makeResource(updatedEntity)
	if err != nil {
		return nil, errors.WrapError("make resource", errors.InternalError, err)
	}

	return resource, nil
}

func (s *Service) DeleteResource(ctx context.Context, params *model.DeleteResourceParams) errors.ErrorResult {
	ctxlog.Info(ctx, s.Logger, "delete resource", log.Sprintf("params", "%+v", params))

	err := params.Validate()
	if err != nil {
		return errors.WrapError("validate params", errors.ValidationError, err)
	}

	resultError := s.Storage.DeleteResourceByID(ctx, params.ResourceID)
	if resultError != nil {
		return resultError.Wrap("delete resource entity")
	}

	return nil
}

func (s *Service) ActivateResource(ctx context.Context, params *model.ActivateResourceParams) errors.ErrorResult {
	ctxlog.Info(ctx, s.Logger, "activate resource", log.Sprintf("params", "%+v", params))

	err := params.Validate()
	if err != nil {
		return errors.WrapError("validate params", errors.ValidationError, err)
	}

	errorResult := s.Storage.ActivateResource(ctx, params.ResourceID, storage.EntityVersion(params.Version))
	if errorResult != nil {
		return errorResult.Wrap("activate resource")
	}

	return nil
}

func (s *Service) ListResourceVersions(ctx context.Context, params *model.ListResourceVersionsParams) ([]*model.Resource, errors.ErrorResult) {
	ctxlog.Info(ctx, s.Logger, "list resource versions", log.Sprintf("params", "%+v", params))

	err := params.Validate()
	if err != nil {
		return nil, errors.WrapError("validate params", errors.ValidationError, err)
	}

	resourceVersions, errorResult := s.Storage.GetResourceVersions(ctx, params.ResourceID, params.Versions)
	if errorResult != nil {
		return nil, errorResult.Wrap("get resource versions")
	}

	resources, err := makeResources(resourceVersions)
	if err != nil {
		return nil, errors.WrapError("make resources", errors.InternalError, err)
	}

	return resources, nil
}

func (s *Service) CountActiveResourcesByFolderID(ctx context.Context, params *model.CountActiveResourcesParams) (int64, errors.ErrorResult) {
	ctxlog.Info(ctx, s.Logger, "count active resources", log.Sprintf("params", "%+v", params))

	err := params.Validate()
	if err != nil {
		return 0, errors.WrapError("validate params", errors.ValidationError, err)
	}

	count, errorResult := s.Storage.CountActiveResourcesByFolderID(ctx, params.FolderID)
	if errorResult != nil {
		return 0, errorResult.Wrap("count active resources")
	}

	return count, nil
}

var reNonWord = regexp.MustCompile(`[^a-z0-9_-]`)

func normalizeName(name string) string {
	return reNonWord.ReplaceAllString(name, "-")
}

func generateSourceName(source *model.OriginVariantSource) string {
	switch source.Type {
	case model.OriginTypeCommon:
		return "common-" + normalizeName(source.Source)
	case model.OriginTypeBucket:
		return "s3-" + normalizeName(source.Source)
	case model.OriginTypeWebsite:
		return "website-" + normalizeName(source.Source)
	default:
		return normalizeName(source.Source)
	}
}

func originsGroupEntityFromSource(folderID string, source *model.OriginVariantSource) *storage.OriginsGroupEntity {
	if source == nil {
		return nil
	}

	groupID := rand.Int63()

	return &storage.OriginsGroupEntity{
		RowID:         storage.AutoGenID,
		EntityID:      groupID,
		EntityVersion: 1,
		EntityActive:  false,
		FolderID:      folderID,
		Name:          generateSourceName(source),
		UseNext:       true,
		Origins: []*storage.OriginEntity{
			{
				EntityID:                  storage.AutoGenID,
				OriginsGroupEntityID:      groupID,
				OriginsGroupEntityVersion: 0,
				FolderID:                  folderID,
				Source:                    source.Source,
				Enabled:                   true,
				Backup:                    false,
				Type:                      storage.OriginType(source.Type),
				CreatedAt:                 storage.AutoGenTime,
				UpdatedAt:                 storage.AutoGenTime,
			},
		},
		CreatedAt: storage.AutoGenTime,
		UpdatedAt: storage.AutoGenTime,
		DeletedAt: nil,
	}
}

func resourceEntityForSave(
	resourceID string,
	originsGroupID int64,
	cname string,
	params *model.CreateResourceParams,
) *storage.ResourceEntity {
	if params == nil {
		return nil
	}

	return &storage.ResourceEntity{
		RowID:                storage.AutoGenID,
		EntityID:             resourceID,
		EntityVersion:        1,
		EntityActive:         false,
		OriginsGroupEntityID: originsGroupID,
		FolderID:             params.FolderID,
		Active:               params.Active,
		Name:                 params.Name,
		Cname:                cname,
		SecondaryHostnames:   secondaryHostnames(params.SecondaryHostnames),
		OriginProtocol:       storage.OriginProtocol(params.OriginProtocol),
		Options:              resourceEntityOptions(params.Options),
		CreatedAt:            storage.AutoGenTime,
		UpdatedAt:            storage.AutoGenTime,
		DeletedAt:            nil,
	}
}

func secondaryHostnames(hostnames []string) []*storage.SecondaryHostnameEntity {
	result := make([]*storage.SecondaryHostnameEntity, 0, len(hostnames))
	for _, hostname := range hostnames {
		result = append(result, &storage.SecondaryHostnameEntity{
			RowID:                 storage.AutoGenID,
			ResourceEntityID:      "",
			ResourceEntityVersion: 0,
			ResourceEntityActive:  false,
			Hostname:              hostname,
		})
	}

	return result
}

func resourceEntityOptions(options *model.ResourceOptions) *storage.ResourceOptions {
	if options == nil {
		return nil
	}

	return &storage.ResourceOptions{
		CustomHost:              options.CustomHost,
		CustomSNI:               options.CustomSNI,
		RedirectToHTTPS:         options.RedirectToHTTPS,
		AllowedMethods:          makeEntityAllowedMethods(options.AllowedMethods),
		CORS:                    makeEntityCORSOptions(options.CORS),
		BrowserCacheOptions:     makeEntityBrowserCacheOptions(options.BrowserCacheOptions),
		EdgeCacheOptions:        makeEntityEdgeCacheOptions(options.EdgeCacheOptions),
		ServeStaleOptions:       makeEntityServeStaleOptions(options.ServeStaleOptions),
		NormalizeRequestOptions: makeEntityNormalizeRequestOptions(options.NormalizeRequestOptions),
		CompressionOptions:      makeEntityCompressionOptions(options.CompressionOptions),
		StaticHeadersOptions:    makeEntityStaticHeadersOptions(options.StaticHeadersOptions),
		RewriteOptions:          makeEntityRewriteOptions(options.RewriteOptions),
	}
}

func resourceParamsForUpdate(params *model.UpdateResourceParams) *storage.UpdateResourceParams {
	if params == nil {
		return nil
	}

	return &storage.UpdateResourceParams{
		ID:                 params.ResourceID,
		OriginsGroupID:     params.OriginsGroupID,
		Active:             params.Active,
		SecondaryHostnames: params.SecondaryHostnames,
		OriginProtocol:     resourceEntityOriginProtocol(params.OriginProtocol),
		Options:            resourceEntityOptions(params.Options),
	}
}

func resourceEntityOriginProtocol(protocol *model.OriginProtocol) *storage.OriginProtocol {
	if protocol != nil {
		storageProtocol := storage.OriginProtocol(*protocol)
		return &storageProtocol
	}
	return nil
}

func makeResources(entities []*storage.ResourceEntity) ([]*model.Resource, error) {
	result := make([]*model.Resource, 0, len(entities))
	for _, entity := range entities {
		resource, err := makeResource(entity)
		if err != nil {
			return nil, err
		}
		result = append(result, resource)
	}

	return result, nil
}

func makeResource(entity *storage.ResourceEntity) (*model.Resource, error) {
	options, err := makeResourceOptions(entity.Options)
	if err != nil {
		return nil, fmt.Errorf("make resource options: %w", err)
	}

	return &model.Resource{
		ID:                 entity.EntityID,
		FolderID:           entity.FolderID,
		OriginsGroupID:     entity.OriginsGroupEntityID,
		Active:             entity.Active,
		Name:               entity.Name,
		Cname:              entity.Cname,
		SecondaryHostnames: makeSecondaryHostnames(entity.SecondaryHostnames),
		OriginProtocol:     model.OriginProtocol(entity.OriginProtocol),
		Options:            options,
		CreateAt:           entity.CreatedAt,
		UpdatedAt:          entity.UpdatedAt,
		Meta: &model.VersionMeta{
			Version: int64(entity.EntityVersion),
			Active:  entity.EntityActive,
		},
	}, nil
}

func makeSecondaryHostnames(hostnames []*storage.SecondaryHostnameEntity) []string {
	result := make([]string, 0, len(hostnames))
	for _, hostname := range hostnames {
		result = append(result, hostname.Hostname)
	}

	return result
}

func makeResourceOptions(options *storage.ResourceOptions) (*model.ResourceOptions, error) {
	if options == nil {
		return nil, nil
	}

	return &model.ResourceOptions{
		CustomHost:              options.CustomHost,
		CustomSNI:               options.CustomSNI,
		RedirectToHTTPS:         options.RedirectToHTTPS,
		AllowedMethods:          makeAllowedMethods(options.AllowedMethods),
		CORS:                    makeCORSOptions(options.CORS),
		BrowserCacheOptions:     makeBrowserCacheOptions(options.BrowserCacheOptions),
		EdgeCacheOptions:        makeEdgeCacheOptions(options.EdgeCacheOptions),
		ServeStaleOptions:       makeServeStaleOptions(options.ServeStaleOptions),
		NormalizeRequestOptions: makeNormalizeRequestOptions(options.NormalizeRequestOptions),
		CompressionOptions:      makeCompressionOptions(options.CompressionOptions),
		StaticHeadersOptions:    makeStaticHeadersOptions(options.StaticHeadersOptions),
		RewriteOptions:          makeRewriteOptions(options.RewriteOptions),
	}, nil
}

func makeCORSOptions(options *storage.CORSOptions) *model.CORSOptions {
	if options == nil {
		return nil
	}

	return &model.CORSOptions{
		Enabled:      options.Enabled,
		EnableTiming: options.EnableTiming,
		Mode: func() *model.CORSMode {
			if options.Mode == nil {
				return nil
			}
			mode := model.CORSMode(*options.Mode)
			return &mode
		}(),
		AllowedOrigins: options.AllowedOrigins,
		AllowedMethods: makeAllowedMethods(options.AllowedMethods),
		AllowedHeaders: options.AllowedHeaders,
		MaxAge:         options.MaxAge,
		ExposeHeaders:  options.ExposeHeaders,
	}
}

func makeBrowserCacheOptions(options *storage.BrowserCacheOptions) *model.BrowserCacheOptions {
	if options == nil {
		return nil
	}

	return &model.BrowserCacheOptions{
		Enabled: options.Enabled,
		MaxAge:  options.MaxAge,
	}
}

func makeEdgeCacheOptions(options *storage.EdgeCacheOptions) *model.EdgeCacheOptions {
	if options == nil {
		return nil
	}

	return &model.EdgeCacheOptions{
		Enabled:          options.Enabled,
		UseRedirects:     options.UseRedirects,
		TTL:              options.TTL,
		Override:         options.Override,
		OverrideTTLCodes: makeOverrideTTLCodes(options.OverrideTTLCodes),
	}
}

func makeServeStaleOptions(options *storage.ServeStaleOptions) *model.ServeStaleOptions {
	if options == nil {
		return nil
	}

	return &model.ServeStaleOptions{
		Enabled: options.Enabled,
		Errors:  makeServeStaleErrors(options.Errors),
	}
}

func makeNormalizeRequestOptions(options *storage.NormalizeRequestOptions) *model.NormalizeRequestOptions {
	if options == nil {
		return nil
	}

	return &model.NormalizeRequestOptions{
		Cookies: model.NormalizeRequestCookies{
			Ignore: options.Cookies.Ignore,
		},
		QueryString: model.NormalizeRequestQueryString{
			Ignore:    options.QueryString.Ignore,
			Whitelist: options.QueryString.Whitelist,
			Blacklist: options.QueryString.Blacklist,
		},
	}
}

func makeCompressionOptions(options *storage.CompressionOptions) *model.CompressionOptions {
	if options == nil {
		return nil
	}

	return &model.CompressionOptions{
		Variant: model.CompressionVariant{
			FetchCompressed: options.Variant.FetchCompressed,
			Compress:        makeCompress(options.Variant.Compress),
		},
	}
}

func makeStaticHeadersOptions(options *storage.StaticHeadersOptions) *model.StaticHeadersOptions {
	if options == nil {
		return nil
	}

	return &model.StaticHeadersOptions{
		Request:  makeHeaderOptions(options.Request),
		Response: makeHeaderOptions(options.Response),
	}
}

func makeRewriteOptions(options *storage.RewriteOptions) *model.RewriteOptions {
	if options == nil {
		return nil
	}

	return &model.RewriteOptions{
		Enabled:     options.Enabled,
		Regex:       options.Regex,
		Replacement: options.Replacement,
		Flag: func() *model.RewriteFlag {
			if options.Flag == nil {
				return nil
			}
			flag := model.RewriteFlag(*options.Flag)
			return &flag
		}(),
	}
}

func makeAllowedMethods(methods storage.AllowedMethodArray) []model.AllowedMethod {
	result := make([]model.AllowedMethod, 0, len(methods))
	for _, method := range methods {
		result = append(result, model.AllowedMethod(method))
	}

	return result
}

func makeOverrideTTLCodes(codes storage.OverrideTTLCodeArray) []model.OverrideTTLCode {
	result := make([]model.OverrideTTLCode, 0, len(codes))
	for _, code := range codes {
		result = append(result, makeOverrideTTLCode(code))
	}

	return result
}

func makeOverrideTTLCode(code storage.OverrideTTLCode) model.OverrideTTLCode {
	return model.OverrideTTLCode{
		Code: code.Code,
		TTL:  code.TTL,
	}
}

func makeServeStaleErrors(serveStaleErrors []storage.ServeStaleErrorType) []model.ServeStaleErrorType {
	result := make([]model.ServeStaleErrorType, 0, len(serveStaleErrors))
	for _, serveStaleError := range serveStaleErrors {
		result = append(result, model.ServeStaleErrorType(serveStaleError))
	}

	return result
}

func makeCompress(compress *storage.Compress) *model.Compress {
	if compress == nil {
		return nil
	}

	return &model.Compress{
		Compress: compress.Compress,
		Codecs:   makeCompressCodecs(compress.Codecs),
		Types:    compress.Types,
	}
}

func makeCompressCodecs(codecs []storage.CompressCodec) []model.CompressCodec {
	result := make([]model.CompressCodec, 0, len(codecs))
	for _, codec := range codecs {
		result = append(result, model.CompressCodec(codec))
	}

	return result
}

func makeHeaderOptions(headers []storage.HeaderOption) []model.HeaderOption {
	result := make([]model.HeaderOption, 0, len(headers))
	for _, header := range headers {
		result = append(result, model.HeaderOption{
			Name:   header.Name,
			Action: model.HeaderAction(header.Action),
			Value:  header.Value,
		})
	}

	return result
}

func makePage(page *model.Pagination) *storage.Pagination {
	if page == nil {
		return nil
	}

	return &storage.Pagination{
		Offset: page.Offset(),
		Limit:  page.Limit(),
	}
}
