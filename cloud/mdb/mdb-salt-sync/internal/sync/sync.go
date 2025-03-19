package sync

import (
	"fmt"
	"path"
	"sort"
	"strings"

	"a.yandex-team.ru/cloud/mdb/mdb-salt-sync/internal/repos"
	"a.yandex-team.ru/cloud/mdb/mdb-salt-sync/internal/vcs"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func Env(vcsFactory *vcs.Factory, env string, repos repos.Config, versions map[string]string, l log.Logger) error {
	frvcs := vcsFactory.NewProvider(repos.Fileroots, path.Join(repos.FilerootsEnvsMountPath, env), l)
	l.Info("Syncing env", log.String("env", env))
	if err := syncEnv(vcsFactory, env, repos, versions, frvcs, l); err != nil {
		if errrm := frvcs.RemoveEntireVCSMount(); errrm != nil {
			l.Error("Error while removing vcs mount due to sync error", log.Error(errrm))
		}
		return err
	}
	return nil
}

func syncEnv(vcsFactory *vcs.Factory, env string, repos repos.Config, versions map[string]string, frvcs vcs.Provider, l log.Logger) error {
	l.Info("Checkout fileroot vsc")
	if err := frvcs.Checkout(vcs.CheckoutExclude(excludeForTarget("", versions))); err != nil {
		return xerrors.Errorf("failed to checkout fileroot vcs: %w", err)
	}

	vcsMounts := make([]string, 0, len(repos.FilerootsSubrepos))
	vcses := make(map[string]vcs.Provider, len(repos.FilerootsSubrepos))
	for _, repo := range repos.FilerootsSubrepos {
		vcses[repo.Mount] = vcsFactory.NewProvider(repo, frvcs.GetMountPath(), l)
		vcsMounts = append(vcsMounts, repo.Mount)
	}

	if len(vcses) != len(vcsMounts) {
		return xerrors.Errorf("incorrect vcs array size: %d vs %d", len(vcses), len(vcsMounts))
	}

	// Maps are unordered. Sort slice and lookup in map as needed
	sort.Strings(vcsMounts)
	for _, mount := range vcsMounts {
		l.Debug(fmt.Sprintf("Ordered vcs mount: %s", mount))
	}

	orderedVersionTargets := make([]string, 0, len(versions))
	for target := range versions {
		orderedVersionTargets = append(orderedVersionTargets, target)
	}

	// Maps are unordered. Sort slice and lookup in map as needed
	sort.Strings(orderedVersionTargets)
	for _, target := range orderedVersionTargets {
		l.Debug(fmt.Sprintf("Ordered version target: %s", target))
	}

	checkedOutMounts := make(map[string]bool)
	for _, target := range orderedVersionTargets {
		version := versions[target]
		v := vcsForPath(frvcs, vcsMounts, vcses, target, l)

		if v != frvcs {
			relativeMount := v.GetMountPath()[len(frvcs.GetMountPath()):]
			var err error
			target, err = relativePath(relativeMount, target)
			if err != nil {
				return xerrors.Errorf("failed to get relative path for vcs %q: %w", v, err)
			}
		}

		l.Info("Syncing path", log.String("env", env), log.String("path", target), log.String("version", version))
		if version == "trunk" {
			version = v.LatestVersionSymbol()
		}
		if err := v.Checkout(vcs.CheckoutVersion(version), vcs.CheckoutUpdatePath(target), vcs.CheckoutExclude(excludeForTarget(target, versions))); err != nil {
			return xerrors.Errorf("failed to update vcs %q on path %q: %w", v, target, err)
		}
		checkedOutMounts[v.GetMountPath()] = true
	}
	l.Debug("Examine repos that not pinned in versions", log.Reflect("checkedOutMounts", checkedOutMounts))
	for _, mount := range vcsMounts {
		v := vcses[mount]
		if v == frvcs {
			// we checkout it anyway
			continue
		}
		if checkedOutMounts[v.GetMountPath()] {
			continue
		}
		l.Info(
			"Checkout repo, cause it not pinned in our versions",
			log.String("mount path", mount),
			log.String("vcs", v.String()),
		)
		if err := v.Checkout(); err != nil {
			return xerrors.Errorf("failed to checkout vcs %q: %w", v, err)
		}
	}

	return nil
}

func relativePath(root, p string) (string, error) {
	root = strings.Trim(root, "/")
	p = strings.Trim(p, "/")

	if !strings.HasPrefix(p, root) {
		return "", xerrors.Errorf("root %q is not prefix of path %q", root, p)
	}

	p = p[len(root):]
	p = strings.Trim(p, "/")
	if len(p) == 0 {
		p = "."
	}

	return p, nil
}

func vcsForPath(frvcs vcs.Provider, vcsMounts []string, vsces map[string]vcs.Provider, target string, l log.Logger) vcs.Provider {
	l.Debug("Searching for vcs", log.String("path", target))
	var v vcs.Provider
	for _, mount := range vcsMounts {
		l.Debug("Comparing path with vcs mount", log.String("path", target), log.String("vcs mount", mount))
		if strings.HasPrefix(target, mount) {
			v = vsces[mount]
		}
	}

	if v == nil {
		v = frvcs
	}

	l.Debug("Mapped path to vcs", log.String("path", target), log.String("vcs", v.String()))
	return v
}

// excludeForTarget returns target subdirs that overridden in pins
func excludeForTarget(target string, versions map[string]string) []string {
	var exclude []string
	if target == "" {
		for pin := range versions {
			if !strings.Contains(pin, "/") {
				exclude = append(exclude, pin)
			}
		}
	} else {
		for pin := range versions {
			if pin != target && strings.HasPrefix(pin, target) {
				pinDir := strings.TrimPrefix(pin, target+"/")
				if !strings.Contains(pinDir, "/") {
					exclude = append(exclude, pinDir)
				}
			}
		}
	}
	return exclude
}

func Dev(vcsFactory *vcs.Factory, reposConfig repos.Config, rootRepo repos.RepoConfig, rewriteCheckouts bool, l log.Logger) error {
	l.Info("Syncing Dev")
	saltvcs := vcsFactory.NewProvider(rootRepo, "", l)
	subvcs := make([]vcs.Provider, len(reposConfig.FilerootsSubrepos))
	for i, repo := range reposConfig.FilerootsSubrepos {
		subvcs[i] = vcsFactory.NewProvider(repo, reposConfig.FilerootsEnvsMountPathDevEnv, l)
	}

	if _, err := saltvcs.LocalHead(); err != nil || rewriteCheckouts {
		if err := saltvcs.Checkout(vcs.CheckoutExclude([]string{"envs"})); err != nil {
			return xerrors.Errorf("failed to checkout salt: %w", err)
		}
	}

	// Checkout 'dev' env - its a bit special as it resides in 'root' mount of salt repo, NOT in special directory
	// with softlink to it. So we do not use a 'fileroots vcs' per se (root vcs acts as one).
	// Also we do not update these config as its dev env and developers are responsible for updating vcses themselves.
	for _, v := range subvcs {
		if _, err := v.LocalHead(); err != nil || rewriteCheckouts {
			if err := v.Checkout(); err != nil {
				return xerrors.Errorf("failed while checking out fileroots subrepo vcs for dev env: %w", err)
			}
		}
	}

	return nil
}
