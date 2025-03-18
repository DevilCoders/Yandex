package postgres_test

import (
	"context"
	"testing"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cdn/cloud_api/pkg/storage"
)

func TestSecondaryHostnames(t *testing.T) {
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
	entity.SecondaryHostnames = secondaryHostnames([]string{"1", "2"})
	_, errorResult = db.CreateResource(ctx, entity)
	require.Nil(t, errorResult)

	entity = resourceEntity("entity_id2", groupEntity.EntityID)
	entity.SecondaryHostnames = secondaryHostnames([]string{"3", "4"})
	_, errorResult = db.CreateResource(ctx, entity)
	require.Nil(t, errorResult)

	entity = resourceEntity("entity_id3", groupEntity.EntityID)
	entity.SecondaryHostnames = secondaryHostnames([]string{"1", "2"})
	_, errorResult = db.CreateResource(ctx, entity)
	require.NotNil(t, errorResult)
}

func TestSecondaryHostnames_UpdateResource(t *testing.T) {
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
	entity.SecondaryHostnames = secondaryHostnames([]string{"1", "2"})
	createdResource, errorResult := db.CreateResource(ctx, entity)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	params := updateResourceParams(createdResource.EntityID, createdResource.OriginsGroupEntityID)
	params.SecondaryHostnames = ptrStringArray([]string{"1", "2"})
	updatedResource, errorResult := db.UpdateResource(ctx, params)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}
	assertSecondaryHostnameEntities(t, createdResource.SecondaryHostnames, updatedResource.SecondaryHostnames)

	errorResult = db.ActivateResource(ctx, updatedResource.EntityID, updatedResource.EntityVersion)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}
}

func TestSecondaryHostnames_TransferHostnameBetweenResources(t *testing.T) {
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

	entity1 := resourceEntity("entity_id", groupEntity.EntityID)
	entity1.SecondaryHostnames = secondaryHostnames([]string{"1", "2"})
	resource1, errorResult := db.CreateResource(ctx, entity1)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	entity2 := resourceEntity("entity_id", groupEntity.EntityID)
	entity2.SecondaryHostnames = secondaryHostnames([]string{"2"})
	_, errorResult = db.CreateResource(ctx, entity2)
	require.NotNil(t, errorResult)

	params := updateResourceParams(resource1.EntityID, resource1.OriginsGroupEntityID)
	params.SecondaryHostnames = ptrStringArray([]string{"1"}) // delete "2"
	resource1Update, errorResult := db.UpdateResource(ctx, params)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	errorResult = db.ActivateResource(ctx, resource1Update.EntityID, resource1Update.EntityVersion)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	entity2 = resourceEntity("entity_id2", groupEntity.EntityID)
	entity2.SecondaryHostnames = secondaryHostnames([]string{"2"})
	resource2, errorResult := db.CreateResource(ctx, entity2)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	resource1, errorResult = db.GetResourceByID(ctx, resource1.EntityID)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	if assert.Len(t, resource1.SecondaryHostnames, 1) {
		require.Equal(t, "1", resource1.SecondaryHostnames[0].Hostname)
	}
	if assert.Len(t, resource2.SecondaryHostnames, 1) {
		require.Equal(t, "2", resource2.SecondaryHostnames[0].Hostname)
	}
}

func TestSecondaryHostnames_EmptyHostname(t *testing.T) {
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
	entity.SecondaryHostnames = nil
	createResource, errorResult := db.CreateResource(ctx, entity)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}
	require.Empty(t, createResource.SecondaryHostnames)

	params := updateResourceParams(createResource.EntityID, createResource.OriginsGroupEntityID)
	params.SecondaryHostnames = nil
	updateResource, errorResult := db.UpdateResource(ctx, params)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}
	require.Empty(t, updateResource.SecondaryHostnames)

	errorResult = db.ActivateResource(ctx, updateResource.EntityID, updateResource.EntityVersion)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	resourceByID, errorResult := db.GetResourceByID(ctx, updateResource.EntityID)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}
	require.Empty(t, resourceByID.SecondaryHostnames)
}

func TestSecondaryHostnames_EmptyUpdate(t *testing.T) {
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
	entity.SecondaryHostnames = secondaryHostnames([]string{"hostname"})
	createResource, errorResult := db.CreateResource(ctx, entity)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	params := updateResourceParams(createResource.EntityID, createResource.OriginsGroupEntityID)
	params.SecondaryHostnames = nil
	updateResource, errorResult := db.UpdateResource(ctx, params)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	errorResult = db.ActivateResource(ctx, updateResource.EntityID, updateResource.EntityVersion)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	assertSecondaryHostnameEntities(t, createResource.SecondaryHostnames, updateResource.SecondaryHostnames)
}

func TestSecondaryHostnames_DeleteHostnamesWhenDeleteResource(t *testing.T) {
	ctx := context.Background()

	db, err := constructCleanStorage()
	if err != nil {
		t.Fatal(err)
	}

	// create group
	groupEntity := originsGroupEntity(150, true, nil)
	groupEntity, errorResult := db.CreateOriginsGroup(ctx, groupEntity)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	// create entity
	entity := resourceEntity("entity_id", groupEntity.EntityID)
	entity.SecondaryHostnames = secondaryHostnames([]string{"1", "2"})
	_, errorResult = db.CreateResource(ctx, entity)
	require.Nil(t, errorResult)

	errorResult = db.DeleteResourceByID(ctx, entity.EntityID)
	require.Nil(t, errorResult)

	entity2 := resourceEntity("entity_id2", groupEntity.EntityID)
	entity2.SecondaryHostnames = secondaryHostnames([]string{"1", "2"})
	_, errorResult = db.CreateResource(ctx, entity2)
	require.Nil(t, errorResult)
}

func assertSecondaryHostnameEntities(t *testing.T, expected []*storage.SecondaryHostnameEntity, actual []*storage.SecondaryHostnameEntity) {
	var expectedStr []string
	for _, hostname := range expected {
		expectedStr = append(expectedStr, hostname.Hostname)
	}
	var actualStr []string
	for _, hostname := range actual {
		actualStr = append(actualStr, hostname.Hostname)
	}

	require.Equal(t, expectedStr, actualStr)
}

func secondaryHostnames(hostnames []string) []*storage.SecondaryHostnameEntity {
	result := make([]*storage.SecondaryHostnameEntity, 0, len(hostnames))
	for _, hostname := range hostnames {
		result = append(result, &storage.SecondaryHostnameEntity{
			RowID:                 0,
			ResourceEntityID:      "",
			ResourceEntityVersion: 0,
			Hostname:              hostname,
		})
	}

	return result
}

func ptrStringArray(arr []string) *[]string {
	return &arr
}
