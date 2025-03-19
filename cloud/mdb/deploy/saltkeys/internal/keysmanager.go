package internal

import (
	"context"
	"path"
	"strings"
	"sync"
	"time"

	"github.com/fsnotify/fsnotify"

	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/deployapi"
	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/deploy/saltkeys/internal/saltkeys"
	"a.yandex-team.ru/cloud/mdb/internal/fs"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type KeysManagerConfig struct {
	SaltMasterFQDN string        `json:"salt_master_fqdn" yaml:"salt_master_fqdn"`
	UpdatePeriod   time.Duration `json:"update_period" yaml:"update_period"`
	PageSize       int64         `json:"pagesize" yaml:"pagesize"`
}

func DefaultKeysManagerConfig() KeysManagerConfig {
	return KeysManagerConfig{
		UpdatePeriod: time.Minute * 10,
		PageSize:     1000,
	}
}

type KeysManager struct {
	ctx context.Context
	l   log.Logger

	cfg     KeysManagerConfig
	dapi    deployapi.Client
	skeys   saltkeys.Keys
	watcher fs.Watcher

	minions minionsCache
}

type minionsCache struct {
	mu      *sync.Mutex
	minions map[string]models.Minion
}

func newMinionsCache() minionsCache {
	return minionsCache{
		mu:      &sync.Mutex{},
		minions: make(map[string]models.Minion),
	}
}

func (mc *minionsCache) StoreMinions(minions map[string]models.Minion) {
	mc.mu.Lock()
	defer mc.mu.Unlock()
	mc.minions = minions
}

func (mc *minionsCache) Minion(fqdn string) (models.Minion, bool) {
	mc.mu.Lock()
	defer mc.mu.Unlock()
	m, ok := mc.minions[fqdn]
	return m, ok
}

func (mc *minionsCache) InvalidateMinion(fqdn string) {
	mc.mu.Lock()
	defer mc.mu.Unlock()
	delete(mc.minions, fqdn)
}

func NewKeysManager(
	ctx context.Context,
	cfg KeysManagerConfig,
	dapi deployapi.Client,
	skeys saltkeys.Keys,
	watcher fs.Watcher,
	l log.Logger,
) (*KeysManager, error) {
	if err := watcher.Add(saltkeys.Pre.KeyPath()); err != nil {
		return nil, xerrors.Errorf("failed to add path %q to watcher: %w", saltkeys.Pre.KeyPath(), err)
	}
	if err := watcher.Add(saltkeys.Rejected.KeyPath()); err != nil {
		return nil, xerrors.Errorf("failed to add path %q to watcher: %w", saltkeys.Rejected.KeyPath(), err)
	}
	if err := watcher.Add(saltkeys.Denied.KeyPath()); err != nil {
		return nil, xerrors.Errorf("failed to add path %q to watcher: %w", saltkeys.Denied.KeyPath(), err)
	}

	km := newKeysManager(ctx, cfg, dapi, skeys, watcher, l)

	go km.keysUpdater(ctx)
	go km.handleNewKeys(ctx)
	return km, nil
}

func newKeysManager(
	ctx context.Context,
	cfg KeysManagerConfig,
	dapi deployapi.Client,
	skeys saltkeys.Keys,
	watcher fs.Watcher,
	l log.Logger,
) *KeysManager {
	return &KeysManager{
		ctx:     ctx,
		l:       l,
		cfg:     cfg,
		dapi:    dapi,
		skeys:   skeys,
		watcher: watcher,
		minions: newMinionsCache(),
	}
}

func (km *KeysManager) handleNewKeys(ctx context.Context) {
	for {
		select {
		case <-ctx.Done():
			km.l.Info("Stopping...")
			return
		case event, ok := <-km.watcher.Events():
			if !ok {
				return
			}

			km.l.Debugf("Event: %s", event)

			if event.Op&fsnotify.Write == fsnotify.Write {
				km.l.Debugf("Received event: %s", event)
				fqdn := path.Base(event.Name)

				if strings.HasPrefix(event.Name, saltkeys.Rejected.KeyPath()) ||
					strings.HasPrefix(event.Name, saltkeys.Denied.KeyPath()) {
					km.l.Warnf("Minion %q is appeared in either rejected or denied key path, deleting", fqdn)
					if err := km.skeys.Delete(fqdn); err != nil {
						km.l.Errorf("Failed to delete key of minion %q: %s", fqdn, err)
					}

					km.minions.InvalidateMinion(fqdn)
					continue
				}

				minion, ok, err := km.ourMinion(ctx, fqdn)
				if err != nil {
					km.l.Errorf("failed to check if minion %q is ours: %s", fqdn, err)
					continue
				}

				if !ok {
					km.l.Warnf("Minion %q is not ours, deleting", fqdn)
					if err := km.skeys.Delete(fqdn); err != nil {
						km.l.Errorf("Failed to delete key of minion %q: %s", fqdn, err)
					}

					continue
				}

				if _, err := km.verifyMinionKey(ctx, minion, saltkeys.Pre); err != nil {
					km.l.Errorf("Minion %q is ours, but key verification failed, deleting: %s", fqdn, err)
					if err := km.skeys.Delete(fqdn); err != nil {
						km.l.Errorf("Failed to delete key of minion %q: %s", fqdn, err)
					}

					km.minions.InvalidateMinion(fqdn)
					continue
				}

				if err := km.skeys.Accept(fqdn); err != nil {
					km.l.Errorf("Failed to accept minion %q: %s", fqdn, err)
				}

				km.l.Infof("Minion %q is now accepted", fqdn)
			}
		case err, ok := <-km.watcher.Errors():
			if !ok {
				return
			}

			km.l.Errorf("Watcher error: %s", err)
		}
	}
}

func (km *KeysManager) ourMinion(ctx context.Context, fqdn string) (models.Minion, bool, error) {
	// Check if minion is already in the list of assigned minions
	if minion, ok := km.minions.Minion(fqdn); ok {
		return minion, true, nil
	}

	// Do it the hard way - get minion's master
	minion, err := km.dapi.GetMinion(ctx, fqdn)
	if err != nil {
		if xerrors.Is(err, deployapi.ErrNotFound) {
			km.l.Warnf("Minion %q is unknown to deploy", fqdn)
			return models.Minion{}, false, nil
		}

		return models.Minion{}, false, xerrors.Errorf("error while retrieving master for minion %q: %w", fqdn, err)
	}

	if minion.MasterFQDN != km.cfg.SaltMasterFQDN {
		km.l.Warnf(
			"Master of minion %q is %q but we are %q",
			fqdn,
			minion.MasterFQDN,
			km.cfg.SaltMasterFQDN,
		)

		return models.Minion{}, false, nil
	}

	if minion.Deleted {
		km.l.Warnf("Minion %q is marked as deleted", fqdn)
		return models.Minion{}, false, nil
	}

	return minion, true, nil
}

func (km *KeysManager) verifyMinionKey(ctx context.Context, minion models.Minion, state saltkeys.StateID) (models.Minion, error) {
	realKey, err := km.skeys.Key(minion.FQDN, state)
	if err != nil {
		return models.Minion{}, xerrors.Errorf("error while retrieving public key of minion %q: %w", minion.FQDN, err)
	}

	// Verify minion's key
	if len(minion.PublicKey) == 0 {
		// Try registering minion's key
		registered, err := km.dapi.RegisterMinion(ctx, minion.FQDN, realKey)
		if err != nil {
			// We failed. We should not accept minion because there are all kinds of race conditions if we accept.
			return models.Minion{}, xerrors.Errorf("error while storing public key %q of minion %q: %w", realKey, minion.FQDN, err)
		}

		minion = registered
		km.l.Infof("Registered public key of minion %q (had no key before)", minion.FQDN)
		return minion, nil
	}

	// Check that key is what we expect
	if minion.PublicKey != realKey {
		if minion.RegisterUntil.IsZero() {
			return models.Minion{}, xerrors.Errorf("minion %q: %w", minion.FQDN, saltkeys.ErrKeyMismatch)
		}

		// Minion can be re-registered, try doing it!
		registered, err := km.dapi.RegisterMinion(ctx, minion.FQDN, realKey)
		if err != nil {
			// We failed. We should not accept minion because there are all kinds of race conditions if we accept.
			return models.Minion{}, xerrors.Errorf("error while storing public key %q of minion %q: %w", realKey, minion.FQDN, err)
		}

		minion = registered
		km.l.Infof("Registered public key of minion %q (had key before, but was allowed to register again)", minion.FQDN)
		return minion, nil
	}

	return minion, nil
}

func (km *KeysManager) keysUpdater(ctx context.Context) {
	km.cleanupKeyStates()
	km.updateKeys(ctx)

	ticker := time.NewTicker(km.cfg.UpdatePeriod)
	defer ticker.Stop()

	for {
		select {
		case <-ctx.Done():
			km.l.Debug("Stopping keys updater.")
			return
		case <-ticker.C:
			km.cleanupKeyStates()
			km.updateKeys(ctx)
		}
	}
}

func (km *KeysManager) cleanupKeyStates() {
	km.l.Debug("Starting key states cleanup")
	defer km.l.Debug("Finished key states cleanup")

	minions, err := saltkeys.Minions(km.skeys, saltkeys.Rejected, saltkeys.Denied)
	if err != nil {
		km.l.Errorf("failed to retrieve rejected and denied minions: %s", err)
		return
	}

	// Delete keys with invalid states
	for fqdn, states := range minions {
		km.l.Warnf("minion %q is known and in %+v state(s), deleting", states, fqdn)
		if err := km.skeys.Delete(fqdn); err != nil {
			km.l.Warnf("failed to delete minion %q key: %s", fqdn, err)
		}
	}
}

func (km *KeysManager) updateKeys(ctx context.Context) {
	km.l.Debug("Starting keys update")
	defer km.l.Debug("Finished keys update")

	// We load known minions before requesting list of our minions from API to avoid race condition
	// (if we load list from API first, a super-quick minion can register in API and knock on
	// salt-master's door before we load known minions -> we will have known minion which is not supposed
	// to be there, and that complicates deletion logic).
	known, err := saltkeys.Minions(km.skeys, saltkeys.Pre, saltkeys.Accepted)
	if err != nil {
		km.l.Errorf("failed to retrieve known minions: %s", err)
		return
	}

	loaded := map[string]models.Minion{}
	paging := deployapi.Paging{Size: km.cfg.PageSize}

	for {
		var minionsPage []models.Minion
		var err error
		minionsPage, paging, err = km.dapi.GetMinionsByMaster(ctx, km.cfg.SaltMasterFQDN, paging)
		if err != nil {
			km.l.Errorf("Failed to retrieve minions for keys update: %s", err)
			return
		}

		for _, minion := range minionsPage {
			if !minion.Deleted {
				loaded[minion.FQDN] = minion
			}
		}

		// Did we load everything?
		if int64(len(minionsPage)) < paging.Size {
			km.l.Debugf("Loaded %d minions", len(loaded))
			km.actualizeKeys(ctx, known, loaded)
			km.minions.StoreMinions(loaded)
			return
		}
	}
}

func (km *KeysManager) actualizeKeys(ctx context.Context, known map[string][]saltkeys.StateID, loaded map[string]models.Minion) {
	// Iterate over all minions that belong to us
	for _, minion := range loaded {
		// Check if minion is known to us
		states, ok := known[minion.FQDN]
		if !ok {
			km.l.Debugf("Minion %q is ours but not known to us (no key yet), skipping", minion.FQDN)
			continue
		}

		if len(states) > 1 {
			km.l.Warnf("minion %q is known but in more than one state %+v, deleting", minion.FQDN, states)
			if err := km.skeys.Delete(minion.FQDN); err != nil {
				km.l.Warnf("failed to delete minion %q key: %s", minion.FQDN, err)
			}

			delete(known, minion.FQDN)
			continue
		}

		if len(states) == 0 {
			km.l.Errorf("minion %q is known but has zero states which is impossible", minion.FQDN)
			delete(known, minion.FQDN)
			continue
		}

		state := states[0]

		// Check key before deeming minion fit - we don't want to keep old key if it has changed
		verified, err := km.verifyMinionKey(ctx, minion, state)
		if err != nil {
			km.l.Warnf("Minion %q is known and ours but key verification failed: %s", minion.FQDN, err)
			continue
		}

		minion = verified

		// We keep in 'known' list only those minions which are not ours
		delete(known, minion.FQDN)

		// Minion must be accepted. If its not, accept it.
		if state == saltkeys.Accepted {
			km.l.Debugf("Minion %q is already accepted", minion.FQDN)
			continue
		}

		if err := km.skeys.Accept(minion.FQDN); err != nil {
			km.l.Errorf("Minion %q is known and ours but accept failed: %s", minion.FQDN, err)
			continue
		}

		km.l.Infof("Accepted minion %q (known and ours)", minion.FQDN)
	}

	// Iterate over whatever is left of known minions
	for fqdn := range known {
		// If minion is pre or accepted, delete its key
		// We can simply delete pre because we guarantee that loaded minions list is not outdated compared
		// to known minions list (look above for race condition comment)
		km.l.Infof("deleting key of minion %q (known but NOT ours)", fqdn)

		if err := km.skeys.Delete(fqdn); err != nil {
			km.l.Errorf("Minion %q is known and NOT ours but we failed to delete its key: %s", fqdn, err)
			continue
		}
	}
}
