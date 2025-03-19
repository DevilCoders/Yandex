# -*- coding: utf-8 -*-
from __future__ import print_function, absolute_import
import logging
import apt_pkg
from time import sleep

log = logging.getLogger(__name__)
YC_RELEASE_PIN_PKGS_PILLAR = "yc-release"       # pins from cluster configs or CI
YC_BASE_PIN_PKGS_PILLAR = "yc-pinning"          # pins from salt-formula
VALID_CACHE_TIME = 1200
RETRY_BACKOFF = 15
RETRIES = 30


def __virtual__():
    """
    Only make these states available if a pkg provider has been detected or
    assigned for this minion
    """
    return "pkg.installed" in __states__


def _get_apt_cache():
    """Get apt cache"""
    apt_pkg.init()
    apt_cache = apt_pkg.Cache()
    return apt_cache


def _get_yc_pkg_pillar(major, minor="packages"):
    try:
        pillar = __pillar__[major][minor]
        if not pillar:
            return {}
    except KeyError:
        log.debug("Pillar %s:%s doesn't exist", major, minor)
        return {}
    return pillar


def _get_pin_pkgs():
    yc_pin_pkgs = _get_yc_pkg_pillar(YC_BASE_PIN_PKGS_PILLAR).copy()
    yc_release_pin_pkgs = _get_yc_pkg_pillar(YC_RELEASE_PIN_PKGS_PILLAR)

    for pkg, ver in yc_release_pin_pkgs.iteritems():
        if pkg in yc_pin_pkgs:
            log.debug("Redefining version for package %r: %s was changed to %s", pkg, yc_pin_pkgs[pkg], ver)

        yc_pin_pkgs[pkg] = ver

    return yc_pin_pkgs


def _get_pkg_map(apt_cache):
    """
    We can find more one package with the same name in apt cache
    pkgs = [pkg for pkg in apt_cache.packages if pkg.name == "zlib1g"]
    [<apt_pkg.Package object: name:'zlib1g' id:3259>,
     <apt_pkg.Package object: name:'zlib1g' id:4159>]
    and not all of these packages contain version list or each of package contain different versions
    pkgs[0].version_list
    [<apt_pkg.Version object: Pkg:'zlib1g' Ver:'1:1.2.8.dfsg-2ubuntu4.1' Section:'libs'  Arch:'amd64' Size:51222 ISize:159744 Hash:53119 ID:2753 Priority:1>,
     <apt_pkg.Version object: Pkg:'zlib1g' Ver:'1:1.2.8.dfsg-2ubuntu4' Section:'libs'  Arch:'amd64' Size:51328 ISize:160768 Hash:40794 ID:2746 Priority:1>]
    pkgs[1].version_list
    []
    So we create dict with "name" as key and "set of versions" as value
    """
    map = {}
    for pkg in apt_cache.packages:
        if not map.get(pkg.name):
            map[pkg.name] = set()
        for ver in pkg.version_list:
            map[pkg.name].add(ver.ver_str)
    return map


def _get_dependency_pkg(apt_cache, package, version, reverse=False):
    packages_by_name = (pkg for pkg in apt_cache.packages if pkg.name == package)
    for pkg in packages_by_name:
        if not reverse:
            package_version = (ver for ver in pkg.version_list if ver.ver_str == version)
            for ver in package_version:
                for dep_pkg in ver.depends_list_str.get("Depends", []):
                    yield dep_pkg[0][0]
        else:
            """
            target_pkg - current package that we want to install
            parent_pkg - package that depends on target_pkg
            We are trying to find reverse dependencies for parent_pkg that should be installed the same version as target_pkg
            """
            package_reverse_dependencies = (rev_dep.parent_pkg.name for rev_dep in pkg.rev_depends_list
                                            if rev_dep.parent_ver.ver_str == version and
                                            rev_dep.parent_pkg.current_ver and
                                            rev_dep.comp_type == "=")
            for reverse_dependency in package_reverse_dependencies:
                yield reverse_dependency


def _get_installed_pkgs(install_pkgs, apt_cache):
    """Get all installed packages in system"""
    installed_pkgs = {}

    for pkg in apt_cache.packages:
        if pkg.current_ver and pkg.name in install_pkgs:
            installed_pkgs[pkg.name] = pkg.current_ver.ver_str
    return installed_pkgs


def _get_versioned_pkgs(install_versioned_pkgs, install_pkgs, pinned_pkgs_version, apt_cache):
    install_deps_pkg = []
    for pkg in install_pkgs:
        if isinstance(pkg, dict) or "=" in pkg:
            raise RuntimeError("Package version must be managed by YC-release or YC-pinning pillars")
        # Workaround according to CLOUD-5622
        if pkg == "contrail-vrouter":
            pkg = "contrail-vrouter-dkms"
        if pkg not in install_versioned_pkgs:
            if pkg in pinned_pkgs_version:
                install_pkg_version = pinned_pkgs_version.get(pkg)
                log.debug("Package %s: %s will be installed", pkg, install_pkg_version)
                install_versioned_pkgs[pkg] = install_pkg_version

                install_deps_pkg.extend(_get_dependency_pkg(apt_cache, pkg, install_pkg_version))
                install_deps_pkg.extend(_get_dependency_pkg(apt_cache, pkg, install_pkg_version, reverse=True))

                _get_versioned_pkgs(install_versioned_pkgs, install_deps_pkg, pinned_pkgs_version, apt_cache)
            elif _get_pkg_map(apt_cache).get(pkg):
                log.debug("Package %s will be installed any version", pkg)
                install_versioned_pkgs[pkg] = None
            else:
                log.warning("I can't find package %s in repository", pkg)


def installed(
        name,
        version=None,
        pkgs=None,
        disable_update=False,
        hold_pinned_pkgs=False,
        **kwargs):

    ret = {"name": name, "changes": {}, "result": False, "comment": ""}
    if name and not pkgs:
        pkgs = [name]

    if version:
        raise RuntimeError("Package version must be managed by YC-release or YC-pinning pillars")
    apt_cache = _get_apt_cache()
    yc_pinned_pkgs = _get_pin_pkgs()
    versioned_pkgs = {}
    try:
        _get_versioned_pkgs(versioned_pkgs, pkgs, yc_pinned_pkgs, apt_cache)
    except KeyError as err:
        raise RuntimeError("There is no pkg {} to install".format(err))

    if disable_update:
        for pkg_name, pkg_version in _get_installed_pkgs(pkgs, apt_cache).items():
            log.debug("Ignoring update of %r as version %s is already installed (%s pinned)",
                      pkg_name, pkg_version, versioned_pkgs.get(pkg_name))
            del versioned_pkgs[pkg_name]

    pkgs_to_hold = [name for name in pkgs if name in yc_pinned_pkgs]
    if hold_pinned_pkgs and pkgs_to_hold:
        # We're holding pinned packages so that apt won't delete/upgrade it
        # if not explicitly asked to do so. For example, because of dependencies
        # it could decide to remove contrail-vrouter-agent when installing linux-crashdump
        # See CLOUD-20203 and CLOUD-15690 for details.
        #
        # Note: we hold only those packages, that are listed in state without their dependencies.
        # Although dependencies could still be uncontrollably upgraded, they at least won't be deleted.
        #
        # For doing upgrades of held packages we rely on aptpkg.install to automatically
        # hold/unhold them.
        # See: https://github.com/saltstack/salt/commit/d511dda90a843cb587fe56a11646f2f9485b2016

        log.info('Holding pinned packages: %r', pkgs_to_hold)
        hold_ret = __salt__['pkg.hold'](pkgs=pkgs_to_hold)

        for pkg, pkg_ret in hold_ret.items():
            ok = pkg_ret['result'] or (__opts__['test'] and pkg_ret['result'] is None)
            if not ok:
                raise RuntimeError("Failed to hold pkg {!r}: {}".format(pkg, pkg_ret.get('comment')))

    for i in range(1, RETRIES + 1):
        log.debug("It's %d retry of package installation", i)
        ret = __states__["pkg.installed"](
            name,
            pkgs=[{name: version} for name, version in versioned_pkgs.items()],
            **kwargs
        )
        if not ret.get('result') and "Could not get lock" in ret.get('comment'):
            log.debug("I've found dpkg lock, I will try to install package in %d seconds", RETRY_BACKOFF)
            sleep(RETRY_BACKOFF)
            continue
        break

    return ret
