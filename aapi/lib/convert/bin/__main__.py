import os
import optparse
import logging
import json
import cProfile
import pstats

import subvertpy.repos

import aapi.lib.py_common.store as common_store

import aapi.lib.convert.convert as convert


def parse_args():
    parser = optparse.OptionParser()
    parser.add_option('-r', '--svn-repo')
    parser.add_option('-s', '--store')
    parser.add_option('-n', '--n-revisions')
    parser.add_option('-l', '--blacklist-json')
    parser.add_option('-p', '--profile-to')
    opts, args = parser.parse_args()

    assert opts.svn_repo, '--svn-repo must be specified'

    if not opts.store:
        opts.store = 'store'

    if not opts.n_revisions:
        opts.n_revisions = 10
    else:
        opts.n_revisions = int(opts.n_revisions)

    return opts


def main():
    logging.basicConfig(level=logging.DEBUG)

    opts = parse_args()

    store = common_store.Store3(opts.store)
    svn_head_file = os.path.join(opts.store, 'SVN_HEAD')
    if not os.path.exists(svn_head_file):
        convert.init(store, svn_head_file)

    blacklist = set()
    if opts.blacklist_json:
        bl = json.load(open(opts.blacklist_json))
        for p in bl:
            blacklist.add('/' + p.encode('utf-8').strip('/') + '/')

    profile = cProfile.Profile()

    if opts.profile_to:
        profile.enable()

    repo_fs = subvertpy.repos.Repository(opts.svn_repo).fs()

    for i in xrange(opts.n_revisions):
        with open(svn_head_file) as f:
            svn_head = int(f.read().strip())

        convert.update(repo_fs, store, svn_head, blacklist)

        with open(svn_head_file, 'w') as f:
            f.write(str(svn_head + 1))

    if opts.profile_to:
        profile.disable()
        st = pstats.Stats(profile)
        st.dump_stats(opts.profile_to)


if __name__ == '__main__':
    main()
