package fs

import (
	"encoding/json"
	"fmt"
	"io/ioutil"
	"os"
	"path"
	"strings"
	"time"

	"a.yandex-team.ru/cloud/mdb/mdb-porto-agent/pkg/statestore"
	"a.yandex-team.ru/cloud/mdb/mdb-porto-agent/pkg/supp"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var _ statestore.Storage = &StateStore{}

const (
	defaultHistDepth = 10
	jSuffix          = ".json"
	jExtraSuffix     = ".extra"
)

// StateStore allow work with state storage
type StateStore struct {
	log       log.Logger
	cachePath string
	libPath   string
}

// StateExtra is extra state for container
type StateExtra struct {
	Operation string `json:"operation"`
}

// New create base StateStore
func New(cachePath, libPath string, log log.Logger) (*StateStore, error) {
	err := os.MkdirAll(libPath, os.ModeDir|0755)
	return &StateStore{
		log:       log,
		cachePath: cachePath,
		libPath:   libPath,
	}, err
}

// GetTargetState load target state, save history and check, that it's not locked
func (ss *StateStore) GetTargetState(target string) (string, statestore.State, error) {
	container := target
	sp := strings.Split(target, "/")
	if len(sp) > 1 {
		container = sp[0]
	}
	container = strings.TrimSuffix(container, ".json")

	st, err := ss.load(target)
	if err != nil {
		return "", statestore.State{}, xerrors.Errorf("failed to load state for target %s, container %s: %w", target, container, err)
	}

	if len(sp) == 1 {
		ss.updateHistory(st, container)
	}

	extra, err := ss.loadExtra(container)
	if err != nil {
		return "", statestore.State{}, err
	}
	if extra.Operation != "" {
		return "", statestore.State{}, xerrors.Errorf("failed to work with container %s, because of operation %s", container, extra.Operation)
	}
	return container, st, nil
}

// ListContainers get all available contaner states in cache
func (ss *StateStore) ListContainers() ([]string, error) {
	var lc []string
	files, err := ioutil.ReadDir(ss.cachePath)
	if err != nil {
		return nil, err
	}

	for _, fi := range files {
		if fi.IsDir() {
			continue
		}
		n := fi.Name()
		if !strings.HasSuffix(n, jSuffix) {
			continue
		}
		lc = append(lc, strings.TrimSuffix(n, jSuffix))
	}
	return lc, nil
}

// ListHistoryContainers get all available contaners in history library
func (ss *StateStore) ListHistoryContainers() ([]string, error) {
	files, err := ioutil.ReadDir(ss.libPath)
	if err != nil {
		return nil, err
	}

	lc := make([]string, 0, len(files))
	for _, fi := range files {
		if !fi.IsDir() {
			continue
		}
		lc = append(lc, fi.Name())
	}
	return lc, nil
}

// CleanHistory remove obsolete history states of container
func (ss *StateStore) CleanHistory(dryRun bool, container string, ttl time.Duration) {
	if ss.isState(container) {
		ss.log.Debugf("untouch history because container in cache %s", container)
		return
	}
	fd := path.Join(ss.libPath, container)
	ss.log.Infof("check for cleaning %s", fd)
	fns, err := ss.listHistoryNames(container)
	if err != nil {
		ss.log.Warnf("failed to get list of history for %s", container)
	}
	now := time.Now().UTC()
	for _, nj := range fns {
		n := nj[:len(statestore.DatetimeTemplate)]
		ft, err := time.Parse(statestore.DatetimeTemplate, n)
		if err != nil {
			ss.log.Warnf("failed to parse datetime format of %s", nj)
			continue
		}

		if now.Sub(ft) > ttl {
			fp := path.Join(fd, nj)
			msg := fmt.Sprintf("remove file %s", fp)
			err = supp.DoActionWithoutChangelist(ss.log, dryRun, msg, func() error {
				return os.Remove(fp)
			})
			if err != nil {
				ss.log.Warnf("%s failed, %s", msg, err)
			}
		}
	}
	files, err := ioutil.ReadDir(fd)
	if err != nil {
		ss.log.Warnf("unable enumerate container history directory, %s", err)
		return
	}

	if len(files) != 0 {
		return
	}

	msg := fmt.Sprintf("remove directory %s", fd)
	err = supp.DoActionWithoutChangelist(ss.log, dryRun, msg, func() error {
		return os.Remove(fd)
	})
	if err != nil {
		ss.log.Warnf("%s failed, %s", msg, err)
		return
	}
}

// RemoveState for container
func (ss *StateStore) RemoveState(container string) error {
	if container == "" {
		return statestore.ErrEmtpyContainerArgument
	}

	fp := path.Join(ss.cachePath, fmt.Sprintf("%s%s", container, jSuffix))
	if _, err := os.Stat(fp); err == nil {
		return os.Remove(fp)
	}
	return nil
}

// load target state
func (ss *StateStore) load(target string) (statestore.State, error) {
	if target == "" {
		return statestore.State{}, statestore.ErrEmtpyContainerArgument
	}

	if !strings.HasSuffix(target, jSuffix) {
		target += jSuffix
	}
	fp := path.Join(ss.cachePath, target)
	if _, err := os.Stat(fp); err == nil {
		return loadState(fp)
	}

	fp = path.Join(ss.libPath, target)
	if _, err := os.Stat(fp); err == nil {
		return loadState(fp)
	}
	return statestore.State{}, statestore.ErrStateNotExist.Wrap(xerrors.Errorf("unknown target %s", target))
}

// loadExtra return extra state of container (to protect from move_container operation)
func (ss *StateStore) loadExtra(container string) (StateExtra, error) {
	fp := path.Join(ss.cachePath, fmt.Sprintf("%s%s", container, jExtraSuffix))
	if _, err := os.Stat(fp); os.IsNotExist(err) {
		return StateExtra{}, nil
	}

	buf, err := ioutil.ReadFile(fp)
	if err != nil {
		return StateExtra{}, statestore.ErrInvalidContainerExState.Wrap(err)
	}

	var s StateExtra
	err = json.Unmarshal(buf, &s)
	if err != nil {
		return StateExtra{}, statestore.ErrInvalidContainerExState.Wrap(err)
	}
	return s, nil
}

// loadHistoryLast return state of last state in history or error if it not exist
func (ss *StateStore) loadHistoryLast(container string) (statestore.State, error) {
	fns, err := ss.listHistoryNames(container)
	if err != nil || len(fns) == 0 {
		ss.log.Debugf("not found history state for container %s", container)
		return statestore.State{}, statestore.ErrStateNotExist
	}
	fp := path.Join(ss.libPath, path.Join(container, fns[len(fns)-1]))
	return loadState(fp)
}

// saveHistory save state for history purposes
func (ss *StateStore) saveHistory(fqdn string, s statestore.State) error {
	ss.log.Infof("is going save history for container %s", fqdn)
	fd := path.Join(ss.libPath, fqdn)
	if _, err := os.Stat(fd); os.IsNotExist(err) {
		err := os.MkdirAll(fd, os.ModeDir|0755)
		if err != nil {
			return err
		}
	}
	now := time.Now()
	nt := now.Format(statestore.DatetimeTemplate)
	fp := path.Join(ss.libPath, fqdn, fmt.Sprintf("%s%s", nt, jSuffix))
	out, err := json.MarshalIndent(s, "", "    ")
	if err != nil {
		return err
	}

	err = ioutil.WriteFile(fp, out, 0444)
	if err != nil {
		return err
	}

	ss.cleanHistory(fqdn, defaultHistDepth)

	return nil
}

func (ss *StateStore) updateHistory(st statestore.State, container string) {
	sh, err := ss.loadHistoryLast(container)
	same := false
	if err != nil {
		ss.log.Debugf("load last history state for container %s: %v", container, err)
	} else {
		same = (st.Options == sh.Options) && len(st.Volumes) == len(sh.Volumes)
		if same {
			for i := 0; i < len(st.Volumes); i++ {
				if st.Volumes[i] != sh.Volumes[i] {
					same = false
					break
				}
			}
		}

	}
	if !same {
		err = ss.saveHistory(container, st)
		if err != nil {
			ss.log.Infof("failed to save state for container %s to history: %v", container, err)
		}
	}
}

func (ss *StateStore) isState(container string) bool {
	fp := path.Join(ss.cachePath, fmt.Sprintf("%s%s", container, jSuffix))
	_, err := os.Stat(fp)
	return err == nil
}

func loadState(fullPath string) (statestore.State, error) {
	buf, err := ioutil.ReadFile(fullPath)
	if err != nil {
		return statestore.State{}, statestore.ErrMissedContainerState.Wrap(err)
	}

	var s statestore.State
	err = json.Unmarshal(buf, &s)
	if err != nil {
		return statestore.State{}, statestore.ErrInvalidContainerState.Wrap(err)
	}
	return s, nil
}

func (ss *StateStore) listHistoryNames(fqdn string) ([]string, error) {
	fd := path.Join(ss.libPath, fqdn)
	files, err := ioutil.ReadDir(fd)
	if err != nil {
		return nil, xerrors.Errorf("failed enumerate directory %s: %w", fd, err)
	}
	ret := make([]string, 0, len(files))
	for _, fi := range files {
		fn := fi.Name()
		if !strings.HasSuffix(fn, jSuffix) || len(fn) != len(statestore.DatetimeTemplate)+len(jSuffix) {
			continue
		}
		ret = append(ret, fn)
	}
	return ret, nil
}

func (ss *StateStore) cleanHistory(fqdn string, histDepth int) {
	fd := path.Join(ss.libPath, fqdn)
	fns, err := ss.listHistoryNames(fqdn)
	if err != nil {
		ss.log.Warnf("%v", err)
		return
	}
	if len(fns) <= histDepth {
		return
	}
	for _, fn := range fns[:len(fns)-histDepth] {
		fp := path.Join(fd, fn)
		err = os.Remove(fp)
		if err != nil {
			ss.log.Warnf("failed remove file %s: %v", fp, err)
		} else {
			ss.log.Debugf("removed file %s", fp)
		}
	}
}
