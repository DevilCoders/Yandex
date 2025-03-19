package airflow

import (
	"reflect"
	"testing"

	"github.com/stretchr/testify/require"

	afv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/airflow/v1"
)

func TestClusterCreateHasMostFieldsFromClusterGet(t *testing.T) {
	internalCreatedFieldNames := map[string]bool{
		"Id":          true,
		"CreatedAt":   true,
		"Environment": true,
		"Status":      true,
		"Health":      true,
		"Monitoring":  true,
	}

	clusterType := reflect.TypeOf(afv1.Cluster{})
	createClusterType := reflect.TypeOf(afv1.CreateClusterRequest{})

	for i := 0; i < clusterType.NumField(); i++ {
		clusterField := clusterType.Field(i)
		createClusterField, usedInCreate := createClusterType.FieldByName(clusterField.Name)
		if internalCreatedFieldNames[clusterField.Name] {
			require.False(t, usedInCreate, "Create request has unexpected field: %s", clusterField.Name)
		} else {
			require.True(t, usedInCreate, "Create request doesn't have field: %s", clusterField.Name)
			require.Equal(t, clusterField.Type, createClusterField.Type, "Type mismatch for field: %s", clusterField.Name)
		}
	}
}

func TestClusterUpdateHasMostFieldsFromClusterCreate(t *testing.T) {
	updateUnsupportedFieldNames := map[string]bool{
		"FolderId": true,
		"Config":   true,
		"Network":  true,
	}

	updateClusterType := reflect.TypeOf(afv1.UpdateClusterRequest{})
	createClusterType := reflect.TypeOf(afv1.CreateClusterRequest{})

	for i := 0; i < createClusterType.NumField(); i++ {
		clusterField := createClusterType.Field(i)
		updateClusterField, updateSupported := updateClusterType.FieldByName(clusterField.Name)
		if updateUnsupportedFieldNames[clusterField.Name] {
			require.False(t, updateSupported, "Update request has unexpected field: %s", clusterField.Name)
		} else {
			require.True(t, updateSupported, "Update request doesn't have field: %s", clusterField.Name)
			require.Equal(t, clusterField.Type, updateClusterField.Type, "Type mismatch for field: %s", clusterField.Name)
		}
	}
}

func TestClusterUpdateHasExtraFieldsFromClusterCreate(t *testing.T) {
	updateExtraFieldNames := map[string]bool{
		"UpdateMask":  true,
		"ClusterId":   true,
		"ConfigSpec":  true,
		"NetworkSpec": true,
	}

	updateClusterType := reflect.TypeOf(afv1.UpdateClusterRequest{})
	createClusterType := reflect.TypeOf(afv1.CreateClusterRequest{})

	for i := 0; i < updateClusterType.NumField(); i++ {
		updateField := updateClusterType.Field(i)
		createClusterField, createSupported := createClusterType.FieldByName(updateField.Name)
		if updateExtraFieldNames[updateField.Name] {
			require.False(t, createSupported, "Create request has unexpected field: %s", updateField.Name)
		} else {
			require.True(t, createSupported, "Update request has unexpected field: %s", updateField.Name)
			require.Equal(t, updateField.Type, createClusterField.Type, "Type mismatch for field: %s", updateField.Name)
		}
	}
}
