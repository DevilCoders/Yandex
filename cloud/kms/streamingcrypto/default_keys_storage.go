package streamingcrypto

import (
	"context"
	"errors"
	"fmt"
	"io/ioutil"
	"os"
	"path/filepath"
	"sort"
	"sync"
	"time"

	"github.com/gofrs/flock"
	"github.com/golang/protobuf/proto"
	"github.com/golang/protobuf/ptypes"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"

	eks "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/kms/v1/keystore"
	"a.yandex-team.ru/cloud/kms/client/go/keystoreclient"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/nop"
)

const (
	defaultKeystoreBackupDir = "/dev/shm/eks"
	keystoreBackupFilePrefix = "eks_backup"
	keystoreRefreshPeriod    = time.Minute * 30
	keystoreRequestTimeout   = time.Second * 10
)

var errClosed = errors.New("default keys storage is closed")

type IAMTokenProvider func() string

type DefaultKeysStorage interface {
	GetDefaultKeyByID(defaultKeyID string) (DefaultKey, error)
	GetPrimaryDefaultKeyID() (string, error)
	GetPrimaryDefaultKey() (DefaultKey, error)
	GetLastRefreshStatus() (time.Time, error)
	Close()
}

type defaultKeysStorage struct {
	keystoreID    string
	tokenProvider IAMTokenProvider
	ksClient      keystoreclient.KeystoreClient

	options keysStorageOptions

	rwLock sync.RWMutex
	keys   defaultKeys

	closeC chan struct{}
	closed bool

	refreshTime  time.Time
	refreshError error
}

type defaultKeys struct {
	keysMap                  map[string]*defaultKey
	sortedByPrimarySinceDesc []*defaultKey
}

type defaultKey struct {
	id           string
	contents     []byte
	primarySince time.Time
}

type DefaultKey struct {
	id       string
	contents []byte
}

type keysStorageOptions struct {
	backupDir        string
	backupPath       string
	backupFlock      *flock.Flock
	refreshPeriod    time.Duration
	ksRequestTimeout time.Duration
	logger           log.Logger
}

func NewDefaultKeysStorage(
	keystoreID string,
	ksClient keystoreclient.KeystoreClient,
	tokenProvider IAMTokenProvider,
	keysBackupDir string,
	logger log.Logger,
) (DefaultKeysStorage, error) {
	return newDefaultKeysStorageWithOptions(keystoreID, ksClient, tokenProvider, keysStorageOptions{
		backupDir:        keysBackupDir,
		refreshPeriod:    keystoreRefreshPeriod,
		ksRequestTimeout: keystoreRequestTimeout,
		logger:           logger,
	})
}

func newDefaultKeysStorageWithOptions(
	keystoreID string, ksClient keystoreclient.KeystoreClient, tokenProvider IAMTokenProvider, options keysStorageOptions,
) (DefaultKeysStorage, error) {
	if keystoreID == "" {
		return nil, errors.New("keystoreID required")
	}
	if options.logger == nil {
		options.logger = &nop.Logger{}
	}
	if options.backupDir == "" {
		options.backupDir = defaultKeystoreBackupDir
	}
	backupFileName := fmt.Sprintf("%s_%s", keystoreBackupFilePrefix, keystoreID)
	backupFlock := flock.New(filepath.Join(options.backupDir, backupFileName+".lock"))
	options.backupPath = filepath.Join(options.backupDir, backupFileName)
	options.backupFlock = backupFlock

	dks := &defaultKeysStorage{
		keystoreID:    keystoreID,
		tokenProvider: tokenProvider,
		ksClient:      ksClient,
		options:       options,
		closeC:        make(chan struct{}),
	}
	err := dks.initKeys()
	if err != nil {
		return nil, err
	}
	dks.refreshTime = time.Now()

	go runDefaultKeysRefresher(dks)
	return dks, nil
}

func (s *defaultKeysStorage) GetDefaultKeyByID(defaultKeyID string) (DefaultKey, error) {
	s.rwLock.RLock()
	defer s.rwLock.RUnlock()
	if !s.closed {
		if defaultKey, ok := s.keys.keysMap[defaultKeyID]; !ok {
			return DefaultKey{}, fmt.Errorf("default key %v does not exists in storage", defaultKeyID)
		} else {
			return DefaultKey{defaultKeyID, defaultKey.contents}, nil
		}
	}
	return DefaultKey{}, errClosed
}

func (s *defaultKeysStorage) GetPrimaryDefaultKey() (DefaultKey, error) {
	s.rwLock.RLock()
	defer s.rwLock.RUnlock()
	if !s.closed {
		now := time.Now()
		for _, key := range s.keys.sortedByPrimarySinceDesc {
			if key.primarySince.Before(now) {
				return DefaultKey{key.id, key.contents}, nil
			}
		}
		// we validateDefaultKeys, at least one primary key should exists
		panic("illegal default keys storage state: primary key does not exists")
	}
	return DefaultKey{}, errClosed
}

func (s *defaultKeysStorage) GetPrimaryDefaultKeyID() (string, error) {
	s.rwLock.RLock()
	defer s.rwLock.RUnlock()
	if !s.closed {
		now := time.Now()
		for _, key := range s.keys.sortedByPrimarySinceDesc {
			if key.primarySince.Before(now) {
				return key.id, nil
			}
		}
		// we validateDefaultKeys, at least one primary key should exists
		panic("illegal default keys storage state: primary key does not exists")
	}
	return "", errClosed
}

func (s *defaultKeysStorage) GetLastRefreshStatus() (time.Time, error) {
	return s.refreshTime, s.refreshError
}

func (s *defaultKeysStorage) Close() {
	s.rwLock.Lock()
	defer s.rwLock.Unlock()
	if !s.closed {
		close(s.closeC)
		s.keys = defaultKeys{}
		s.closed = true
	}
}

func (s *defaultKeysStorage) initKeys() error {
	fromBackup := false
	callCtx, cancelFunc := context.WithTimeout(context.Background(), s.options.ksRequestTimeout)
	defer cancelFunc()
	exportResp, err := s.ksClient.ExportKeystore(callCtx, s.keystoreID, s.tokenProvider())
	if err != nil {
		// if wrong keystore parameters or invalid authentication
		if status := status.Code(err); status != codes.Unavailable && status != codes.DeadlineExceeded {
			return err
		}
		// if keystore temporary unavailable, try read from backup
		exportResp, err = s.readFromBackup()
		if err != nil {
			return err
		}
		fromBackup = true
	}
	keys, err := convertToDefaultKeys(exportResp)
	if err != nil {
		return err
	}
	if err = validateDefaultKeys(keys); err != nil {
		return err
	}
	if !fromBackup {
		if err = s.saveToBackup(exportResp); err != nil {
			return err
		}
	}
	s.rwLock.Lock()
	defer s.rwLock.Unlock()
	s.keys = keys
	return nil
}

func validateDefaultKeys(keys defaultKeys) error {
	if len(keys.sortedByPrimarySinceDesc) == 0 {
		return errors.New("keystore is empty")
	}
	oldestPrimarySince := keys.sortedByPrimarySinceDesc[len(keys.sortedByPrimarySinceDesc)-1].primarySince
	if oldestPrimarySince.After(time.Now()) {
		return fmt.Errorf("oldest default key primary since %v, which after current time", oldestPrimarySince)
	}
	return nil
}

func (s *defaultKeysStorage) refreshDefaultKeys() error {
	callCtx, cancelFunc := context.WithTimeout(context.Background(), s.options.ksRequestTimeout)
	defer cancelFunc()
	exportResp, err := s.ksClient.ExportKeystore(callCtx, s.keystoreID, s.tokenProvider())
	if err != nil {
		return err
	}
	keys, err := convertToDefaultKeys(exportResp)
	if err != nil {
		return err
	}
	if err = validateDefaultKeys(keys); err != nil {
		return err
	}
	if err = s.saveToBackup(exportResp); err != nil {
		return err
	}
	s.rwLock.Lock()
	defer s.rwLock.Unlock()
	if !s.closed {
		s.keys = keys
		return nil
	}
	return errClosed
}

func (s *defaultKeysStorage) saveToBackup(keysResponse *eks.ExportKeystoreKeysResponse) error {
	if err := os.MkdirAll(s.options.backupDir, 0700); err != nil {
		return err
	}
	if err := s.backupFileLock(); err != nil {
		return err
	}
	defer s.backupFileUnlock()
	protoBytes, err := proto.Marshal(keysResponse)
	if err != nil {
		return err
	}
	return ioutil.WriteFile(s.options.backupPath, protoBytes, 0600)
}

func (s *defaultKeysStorage) readFromBackup() (*eks.ExportKeystoreKeysResponse, error) {
	if err := s.backupFileLock(); err != nil {
		return nil, err
	}
	defer s.backupFileUnlock()
	protoBytes, err := ioutil.ReadFile(s.options.backupPath)
	if err != nil {
		return nil, err
	}
	var backupKeys eks.ExportKeystoreKeysResponse
	if err = proto.Unmarshal(protoBytes, &backupKeys); err != nil {
		return nil, err
	}
	return &backupKeys, nil
}

func runDefaultKeysRefresher(storage *defaultKeysStorage) {
	refreshTicker := time.NewTicker(storage.options.refreshPeriod)
	for {
		done := false
		select {
		case t := <-refreshTicker.C:
			// log err if refresh failed
			if storage.refreshError = storage.refreshDefaultKeys(); storage.refreshError != nil {
				storage.options.logger.Errorf("failed to refresh default keys: %v", storage.refreshError)
			}
			storage.refreshTime = t
		case <-storage.closeC:
			done = true
		}
		if done {
			break
		}
	}
	refreshTicker.Stop()
}

func convertToDefaultKeys(exportedKeys *eks.ExportKeystoreKeysResponse) (defaultKeys, error) {
	keysMap := make(map[string]*defaultKey, len(exportedKeys.Keys))
	keysSlice := make([]*defaultKey, len(exportedKeys.Keys))
	for i, key := range exportedKeys.Keys {
		if primarySince, err := ptypes.Timestamp(key.PrimarySince); err != nil {
			return defaultKeys{}, err
		} else {
			defaultKey := defaultKey{key.Id, key.CryptoMaterial, primarySince}
			keysMap[defaultKey.id] = &defaultKey
			keysSlice[i] = &defaultKey
		}
	}
	sort.Slice(keysSlice, func(i, j int) bool {
		return keysSlice[i].primarySince.After(keysSlice[j].primarySince)
	})
	return defaultKeys{
		keysMap:                  keysMap,
		sortedByPrimarySinceDesc: keysSlice,
	}, nil
}

func (s *defaultKeysStorage) backupFileLock() error {
	return s.options.backupFlock.Lock()
}

func (s *defaultKeysStorage) backupFileUnlock() {
	_ = s.options.backupFlock.Unlock()
}
