package postgres_test

import (
	"context"
	"fmt"
	"testing"
	"time"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/cdn/cloud_api/pkg/storage"
	"a.yandex-team.ru/library/go/ptr"
)

func TestResource(t *testing.T) {
	ctx := context.Background()

	db, err := constructCleanStorage()
	if err != nil {
		t.Fatal(err)
	}

	groupEntity := originsGroupEntity(150, true, nil)
	groupEntity, errorResult := db.CreateOriginsGroup(ctx, groupEntity)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	// create entity
	entity := resourceEntity("entity_id", groupEntity.EntityID)
	entity.SecondaryHostnames = secondaryHostnames([]string{"hostname"})
	createResource, errorResult := db.CreateResource(ctx, entity)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}
	assert.NotZero(t, createResource.RowID)
	assert.NotZero(t, createResource.CreatedAt)
	assert.NotZero(t, createResource.UpdatedAt)
	assert.Nil(t, createResource.DeletedAt)
	assertEqualResources(t, entity, createResource)

	// get entities by folder id
	getAllResourcesByFolderID, errorResult := db.GetAllResourcesByFolderID(ctx, folderID, page)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}
	if assert.NotEmpty(t, getAllResourcesByFolderID) {
		assertEqualResources(t, createResource, getAllResourcesByFolderID[0])
	}

	// get entities
	getAllResources, errorResult := db.GetAllResources(ctx, page)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}
	if assert.NotEmpty(t, getAllResources) {
		assertEqualResources(t, createResource, getAllResources[0])
	}

	// update entity
	updateParams := updateResourceParams(createResource.EntityID, createResource.OriginsGroupEntityID)
	updateResource, errorResult := db.UpdateResource(ctx, updateParams)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}
	assert.Equal(t, createResource.EntityVersion+1, updateResource.EntityVersion)
	assert.Equal(t, false, updateResource.EntityActive)
	assert.Equal(t, createResource.FolderID, updateResource.FolderID)
	assert.Equal(t, false, updateResource.Active)
	assert.Equal(t, createResource.Name, updateResource.Name)
	assert.Equal(t, createResource.Cname, updateResource.Cname)
	assertSecondaryHostnames(t, []string{"secondary_name"}, updateResource.SecondaryHostnames)

	// get old entity
	getResource, errorResult := db.GetResourceByID(ctx, entity.EntityID)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}
	assertEqualResources(t, createResource, getResource)

	// activate entity
	errorResult = db.ActivateResource(ctx, entity.EntityID, updateResource.EntityVersion)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	// get new entity (updated)
	newResource, errorResult := db.GetResourceByID(ctx, entity.EntityID)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}
	assert.Equal(t, createResource.EntityVersion+1, newResource.EntityVersion)
	assert.Equal(t, createResource.FolderID, newResource.FolderID)
	assert.Equal(t, false, newResource.Active)
	assertSecondaryHostnames(t, []string{"secondary_name"}, updateResource.SecondaryHostnames)

	// delete entity
	errorResult = db.DeleteResourceByID(ctx, entity.EntityID)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	resources, errorResult := db.GetAllResources(ctx, page)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}
	assert.Empty(t, resources)
}

func TestGetAllResource(t *testing.T) {
	ctx := context.Background()

	db, err := constructCleanStorage()
	if err != nil {
		t.Fatal(err)
	}

	groupEntity := originsGroupEntity(150, true, nil)
	groupEntity, errorResult := db.CreateOriginsGroup(ctx, groupEntity)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	entity1 := resourceEntity("entity1", groupEntity.EntityID)
	entity1.FolderID = "1"
	entity2 := resourceEntity("entity2", groupEntity.EntityID)
	entity2.FolderID = "2"

	_, errorResult = db.CreateResource(ctx, entity1)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}
	_, errorResult = db.CreateResource(ctx, entity2)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	allResources, errorResult := db.GetAllResources(ctx, page)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}
	assert.Equal(t, 2, len(allResources))

	allResources, errorResult = db.GetAllResourcesByFolderID(ctx, "1", page)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}
	if assert.NotEmpty(t, allResources) {
		assert.Equal(t, "1", allResources[0].FolderID)
		assert.Equal(t, "entity1", allResources[0].EntityID)
	}

	allResources, errorResult = db.GetAllResourcesByFolderID(ctx, "2", page)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}
	if assert.NotEmpty(t, allResources) {
		assert.Equal(t, "2", allResources[0].FolderID)
		assert.Equal(t, "entity2", allResources[0].EntityID)
	}
}

func TestUpdateResource_UseOldOriginsGroupID_ifEmpty(t *testing.T) {
	ctx := context.Background()

	db, err := constructCleanStorage()
	if err != nil {
		t.Fatal(err)
	}

	groupEntity := originsGroupEntity(150, true, nil)
	groupEntity, errorResult := db.CreateOriginsGroup(ctx, groupEntity)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	// create entity
	entity := resourceEntity("entity_id", groupEntity.EntityID)
	createResource, errorResult := db.CreateResource(ctx, entity)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	params := updateResourceParams(createResource.EntityID, 0)
	updateResource, errorResult := db.UpdateResource(ctx, params)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	assert.Equal(t, createResource.EntityID, updateResource.EntityID)
	assert.Equal(t, createResource.EntityVersion+1, updateResource.EntityVersion)
	assert.Equal(t, createResource.OriginsGroupEntityID, updateResource.OriginsGroupEntityID)
}

func TestResourceRepeatedActivate(t *testing.T) {
	ctx := context.Background()

	db, err := constructCleanStorage()
	if err != nil {
		t.Fatal(err)
	}

	groupEntity := originsGroupEntity(150, true, nil)
	groupEntity, errorResult := db.CreateOriginsGroup(ctx, groupEntity)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	// create entity
	entity := resourceEntity("entity_id", groupEntity.EntityID)
	entity.EntityActive = false
	createResource, errorResult := db.CreateResource(ctx, entity)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	_, errorResult = db.GetResourceByID(ctx, createResource.EntityID)
	assert.NotNil(t, errorResult)

	errorResult = db.ActivateResource(ctx, createResource.EntityID, createResource.EntityVersion)
	assert.Nil(t, errorResult)

	_, errorResult = db.GetResourceByID(ctx, createResource.EntityID)
	assert.Nil(t, errorResult)

	errorResult = db.ActivateResource(ctx, createResource.EntityID, createResource.EntityVersion)
	assert.Nil(t, errorResult)

	resourceByID, errorResult := db.GetResourceByID(ctx, createResource.EntityID)
	assert.Nil(t, errorResult)

	createResource.EntityActive = true
	assertEqualResources(t, createResource, resourceByID)
}

func TestOriginProtocolUpdate(t *testing.T) {
	ctx := context.Background()

	db, err := constructCleanStorage()
	if err != nil {
		t.Fatal(err)
	}

	groupEntity := originsGroupEntity(150, true, nil)
	groupEntity, errorResult := db.CreateOriginsGroup(ctx, groupEntity)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	// create entity
	entity := resourceEntity("entity_id", groupEntity.EntityID)
	entity.OriginProtocol = storage.OriginProtocolSame
	createResource, errorResult := db.CreateResource(ctx, entity)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	params := updateResourceParams(createResource.EntityID, createResource.OriginsGroupEntityID)
	params.OriginProtocol = nil
	updateResource, errorResult := db.UpdateResource(ctx, params)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	assert.Equal(t, createResource.OriginProtocol, updateResource.OriginProtocol)
}

func TestGetResourceVersions(t *testing.T) {
	ctx := context.Background()

	db, err := constructCleanStorage()
	if err != nil {
		t.Fatal(err)
	}

	groupEntity := originsGroupEntity(150, true, nil)
	groupEntity, errorResult := db.CreateOriginsGroup(ctx, groupEntity)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	entity := resourceEntity("entity_id", groupEntity.EntityID)
	entity.OriginProtocol = storage.OriginProtocolSame
	createResource, errorResult := db.CreateResource(ctx, entity)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	params := updateResourceParams(createResource.EntityID, createResource.OriginsGroupEntityID)
	params.OriginProtocol = nil
	_, errorResult = db.UpdateResource(ctx, params)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	resourceVersions, errorResult := db.GetResourceVersions(ctx, createResource.EntityID, nil)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	assert.Equal(t, 2, len(resourceVersions))
}

func TestCopyResource(t *testing.T) {
	ctx := context.Background()

	db, err := constructCleanStorage()
	if err != nil {
		t.Fatal(err)
	}

	groupEntity := originsGroupEntity(150, true, nil)
	groupEntity, errorResult := db.CreateOriginsGroup(ctx, groupEntity)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	entity := resourceEntity("entity_id", groupEntity.EntityID)
	createResource, errorResult := db.CreateResource(ctx, entity)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	rule, errorResult := db.CreateResourceRule(ctx, &storage.ResourceRuleEntity{
		ResourceEntityID:      createResource.EntityID,
		ResourceEntityVersion: int64(createResource.EntityVersion),
	})
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	newResource, ruleIDs, errorResult := db.CopyResource(ctx, createResource.EntityID)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	assertEqualResourceData(t, createResource, newResource)
	assert.EqualValues(t, createResource.EntityVersion+1, newResource.EntityVersion)

	assert.Equal(t, rule.EntityID+1, ruleIDs[rule.EntityID])
}

func assertEqualResourceData(t *testing.T, expected, actual *storage.ResourceEntity) {
	assert.Equal(t, expected.EntityID, actual.EntityID)
	assert.Equal(t, expected.OriginsGroupEntityID, actual.OriginsGroupEntityID)
	assert.Equal(t, expected.FolderID, actual.FolderID)
	assert.Equal(t, expected.Active, actual.Active)
	assert.Equal(t, expected.Name, actual.Name)
	assert.Equal(t, expected.Cname, actual.Cname)
	assert.Equal(t, len(expected.SecondaryHostnames), len(actual.SecondaryHostnames))
	if expected.SecondaryHostnames != nil && actual.SecondaryHostnames != nil {
		assert.Equal(t, expected.SecondaryHostnames, actual.SecondaryHostnames)
	}
}

func TestUpdatedEntityCreatedTime(t *testing.T) {
	ctx := context.Background()

	db, err := constructCleanStorage()
	if err != nil {
		t.Fatal(err)
	}

	groupEntity := originsGroupEntity(150, true, nil)
	groupEntity, errorResult := db.CreateOriginsGroup(ctx, groupEntity)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	// create entity
	entity := resourceEntity("entity_id", groupEntity.EntityID)
	createResource, errorResult := db.CreateResource(ctx, entity)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	// update entity
	updateParams := updateResourceParams(createResource.EntityID, createResource.OriginsGroupEntityID)
	updateResource, errorResult := db.UpdateResource(ctx, updateParams)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	assertTime(t, createResource.CreatedAt, updateResource.CreatedAt)
}

func TestCreateResource(t *testing.T) {
	ctx := context.Background()

	db, err := constructCleanStorage()
	if err != nil {
		t.Fatal(err)
	}

	groupEntity := originsGroupEntity(150, true, nil)
	groupEntity, errorResult := db.CreateOriginsGroup(ctx, groupEntity)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	// create entity
	entity := &storage.ResourceEntity{
		RowID:                storage.AutoGenID,
		EntityID:             "resource",
		EntityVersion:        1,
		EntityActive:         true,
		OriginsGroupEntityID: groupEntity.EntityID,
		OriginProtocol:       storage.OriginProtocolHTTP,
		Options: &storage.ResourceOptions{
			CustomHost:      ptr.String("host"),
			CustomSNI:       ptr.String("sni"),
			RedirectToHTTPS: ptr.Bool(true),
			CORS: &storage.CORSOptions{
				Enabled:        ptr.Bool(true),
				EnableTiming:   ptr.Bool(true),
				AllowedOrigins: []string{"1", "2"},
				AllowedMethods: []storage.AllowedMethod{0, 1, 2},
				AllowedHeaders: []string{"1", "2"},
				MaxAge:         ptr.Int64(1),
				ExposeHeaders:  []string{"1", "2"},
			},
			BrowserCacheOptions: &storage.BrowserCacheOptions{
				Enabled: ptr.Bool(true),
				MaxAge:  ptr.Int64(1),
			},
			EdgeCacheOptions: &storage.EdgeCacheOptions{
				Enabled:      ptr.Bool(true),
				UseRedirects: ptr.Bool(false),
				TTL:          ptr.Int64(1),
				Override:     ptr.Bool(true),
			},
			ServeStaleOptions: &storage.ServeStaleOptions{
				Enabled: ptr.Bool(true),
			},
			CompressionOptions: &storage.CompressionOptions{
				Variant: storage.CompressionVariant{
					FetchCompressed: ptr.Bool(true),
				},
			},
			RewriteOptions: &storage.RewriteOptions{
				Enabled:     ptr.Bool(true),
				Regex:       ptr.String("123"),
				Replacement: ptr.String("12444"),
			},
		},
	}
	_, errorResult = db.CreateResource(ctx, entity)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}
}

func TestDeleteAllResourceVersions(t *testing.T) {
	ctx := context.Background()

	db, err := constructCleanStorage()
	if err != nil {
		t.Fatal(err)
	}

	gorm, err := constructGorm()
	if err != nil {
		t.Fatal(err)
	}

	groupEntity := originsGroupEntity(150, true, nil)
	groupEntity, errorResult := db.CreateOriginsGroup(ctx, groupEntity)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	entity := resourceEntity("entity_id", groupEntity.EntityID)
	createResource, errorResult := db.CreateResource(ctx, entity)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	params := updateResourceParams(createResource.EntityID, createResource.OriginsGroupEntityID)
	_, errorResult = db.UpdateResource(ctx, params)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	errorResult = db.DeleteResourceByID(ctx, createResource.EntityID)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	result := gorm.Exec(fmt.Sprintf("SELECT * FROM %s WHERE deleted_at NOTNULL", storage.ResourceTable))
	if result.Error != nil {
		t.Fatal(result.Error)
	}

	assert.EqualValues(t, 2, result.RowsAffected)
}

func assertTime(t *testing.T, expected, actual time.Time) {
	expected = expected.Truncate(1 * time.Millisecond)
	actual = actual.Truncate(1 * time.Millisecond)
	assert.Equal(t, expected, actual)
}

func assertEqualResources(t *testing.T, expected, actual *storage.ResourceEntity) {
	assertEqualResourceData(t, expected, actual)
	assert.Equal(t, expected.RowID, actual.RowID)
	assert.Equal(t, expected.EntityActive, actual.EntityActive)
	assert.Equal(t, expected.EntityVersion, actual.EntityVersion)
}

func assertSecondaryHostnames(t *testing.T, expected []string, actual []*storage.SecondaryHostnameEntity) {
	var actualHostnames []string
	for _, hostname := range actual {
		actualHostnames = append(actualHostnames, hostname.Hostname)
	}
	assert.Equal(t, expected, actualHostnames)
}

func resourceEntity(id string, originsGroupID int64) *storage.ResourceEntity {
	return &storage.ResourceEntity{
		RowID:                storage.AutoGenID,
		EntityID:             id,
		EntityVersion:        1,
		EntityActive:         true,
		OriginsGroupEntityID: originsGroupID,
		FolderID:             folderID,
		Active:               true,
		Name:                 "name",
		Cname:                "cname",
		SecondaryHostnames:   nil,
		OriginProtocol:       storage.OriginProtocolHTTP,
		Options:              nil,
		CreatedAt:            storage.AutoGenTime,
		UpdatedAt:            storage.AutoGenTime,
		DeletedAt:            nil,
	}
}

func updateResourceParams(resourceID string, originsGroupID int64) *storage.UpdateResourceParams {
	protocol := storage.OriginProtocolHTTP
	hostnames := []string{"secondary_name"}
	return &storage.UpdateResourceParams{
		ID:                 resourceID,
		OriginsGroupID:     originsGroupID,
		Active:             false,
		SecondaryHostnames: &hostnames,
		OriginProtocol:     &protocol,
		Options:            nil,
	}
}

var page = &storage.Pagination{
	Offset: 0,
	Limit:  20,
}
