package streamingcrypto

import (
	"context"
	"os"
	"testing"
	"time"

	"github.com/golang/protobuf/ptypes/timestamp"
	"github.com/stretchr/testify/require"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"

	eks "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/kms/v1/keystore"
)

const (
	fakeKeystoreID      = "fake-store-id"
	fakeExportableKeyID = "fake-exportable-key-id"
)

var (
	fakeTokenProvider = func() string { return "fake-token" }

	testKeysStorageOptions = keysStorageOptions{
		backupDir:        "./test_backup_dir",
		refreshPeriod:    time.Minute,
		ksRequestTimeout: time.Second,
	}

	fakeDefaultKeysResponse = &eks.ExportKeystoreKeysResponse{
		Keys: []*eks.ExportableKey{
			{
				Id:             fakeExportableKeyID,
				KeystoreId:     fakeKeystoreID,
				Type:           eks.ExportableKey_L256_BITS,
				PrimarySince:   &timestamp.Timestamp{Nanos: 0, Seconds: 0},
				CryptoMaterial: []byte{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31},
			},
		},
	}

	fakeExportableKeysMap = map[string]fakeExportableKeysResponse{
		fakeKeystoreID: {
			err:  nil,
			resp: fakeDefaultKeysResponse,
		},
	}
)

type fakeKeystoreClient struct {
	closed         bool
	err            error
	exportableKeys map[string]fakeExportableKeysResponse
}

type fakeExportableKeysResponse struct {
	err  error
	resp *eks.ExportKeystoreKeysResponse
}

func (f *fakeKeystoreClient) ExportKeystore(ctx context.Context, keystoreID string, IAMToken string) (*eks.ExportKeystoreKeysResponse, error) {
	if f.closed {
		return nil, status.Error(codes.Unavailable, "transport is closed")
	}
	if f.err != nil {
		return nil, f.err
	}
	resp, ok := f.exportableKeys[keystoreID]
	if !ok {
		return nil, status.Error(codes.NotFound, "not found")
	}
	return resp.resp, resp.err
}

func (f *fakeKeystoreClient) Close() error {
	f.closed = true
	return nil
}

func newFakeKeystoreClient() *fakeKeystoreClient {
	return &fakeKeystoreClient{
		exportableKeys: fakeExportableKeysMap,
	}
}

func TestNewDefaultKeysStorage(t *testing.T) {
	keystoreClient := newFakeKeystoreClient()
	dks, err := newDefaultKeysStorageWithOptions(fakeKeystoreID, keystoreClient, fakeTokenProvider, testKeysStorageOptions)
	defer func() {
		dks.Close()
		clearTestBackup(testKeysStorageOptions)
		require.NoError(t, keystoreClient.Close())
	}()

	require.NoError(t, err)
	require.NotNil(t, dks)
}

func TestDefaultKeysStorage_GetDefaultKeyByID(t *testing.T) {
	keystoreClient := newFakeKeystoreClient()
	dks, err := newDefaultKeysStorageWithOptions(fakeKeystoreID, keystoreClient, fakeTokenProvider, testKeysStorageOptions)
	defer func() {
		dks.Close()
		clearTestBackup(testKeysStorageOptions)
		require.NoError(t, keystoreClient.Close())
	}()

	require.NoError(t, err)
	require.NotNil(t, dks)

	dk, err := dks.GetDefaultKeyByID(fakeExportableKeyID)
	require.NoError(t, err)
	require.Equal(t, fakeExportableKeyID, dk.id)
	require.Equal(t, fakeDefaultKeysResponse.Keys[0].CryptoMaterial, dk.contents)

	dk, err = dks.GetDefaultKeyByID("notExistedId")
	require.Errorf(t, err, "default key %v does not exists in storage", "notExistedId")
	require.Zero(t, dk)
}

func TestDefaultKeysStorage_GetPrimaryDefaultKey(t *testing.T) {
	keystoreClient := newFakeKeystoreClient()
	dks, err := newDefaultKeysStorageWithOptions(fakeKeystoreID, keystoreClient, fakeTokenProvider, testKeysStorageOptions)
	defer func() {
		dks.Close()
		clearTestBackup(testKeysStorageOptions)
		require.NoError(t, keystoreClient.Close())
	}()
	require.NoError(t, err)
	require.NotNil(t, dks)

	pk, err := dks.GetPrimaryDefaultKey()
	require.NoError(t, err)
	require.Equal(t, fakeExportableKeyID, pk.id)
	require.Equal(t, fakeDefaultKeysResponse.Keys[0].CryptoMaterial, pk.contents)
}

func TestDefaultKeysStorage_GetPrimaryDefaultKeyID(t *testing.T) {
	keystoreClient := newFakeKeystoreClient()
	dks, err := newDefaultKeysStorageWithOptions(fakeKeystoreID, keystoreClient, fakeTokenProvider, testKeysStorageOptions)
	defer func() {
		dks.Close()
		clearTestBackup(testKeysStorageOptions)
		require.NoError(t, keystoreClient.Close())
	}()
	require.NoError(t, err)
	require.NotNil(t, dks)

	pkID, err := dks.GetPrimaryDefaultKeyID()
	require.NoError(t, err)
	require.Equal(t, fakeExportableKeyID, pkID)
}

func clearTestBackup(options keysStorageOptions) {
	_ = os.RemoveAll(options.backupDir)
}
