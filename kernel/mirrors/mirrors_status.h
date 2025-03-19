#pragma once

namespace NLibMdb
{
    // TODO: need cleanup
    const ui32 BITFLAG_REDIR             = 1 << 0  //    1: local redirect [pagecmp; set by redir target]
             , BITFLAG_ROBTXT_ON         = 1 << 1  //    2:
             , BITFLAG_ROBTXT_WEAK       = 1 << 2  //    4:
             , BITFLAG_WEAK_MIRROR       = 1 << 3  //    8: more than half (see code) of links points to another host.
             , BITFLAG_ROBTXT_BAN        = 1 << 4  //   16:
             , BITFLAG_BAD_URL           = 1 << 5  //   32:
             , BITFLAG_HOST_DIRECTIVE    = 1 << 6  //   64: indicates that this host's host directive has some other host.
             , BITFLAG_NOFOLLOW          = 1 << 7  //  128: host have noindex,nofollow meta tag on index page.
             , BITFLAG_FAR_REDIR         = 1 << 8  //  256: redir to another host
             , BITFLAG_FILTER            = 1 << 9  //  512: host is filtered by filter.so.
             , BITFLAG_CYCLIC_REDIR      = 1 << 10 /* 1024: some of redirects in group outdated (they form a cycle), this host has the
                                                    *       oldest one, so we should ignore it. */
             , BITFLAG_HASH_F1           = 1 << 11 // 2048:
             , BITFLAG_HASH_F2           = 1 << 12 // 4096:
             , BITFLAG_HASH_F3           = 1 << 13 // 8192:
             , BITFLAG_HASH_F4           = 1 << 14 // 16384:
             , BITFLAG_HASH              = 15 * BITFLAG_HASH_F1

                                                // bitflags [16; 19] reserved for hypothesis ID (4 bits, 16 values).
                                                // To check, use bitmask 983040, calculated as: (1 << 16) | (1 << 17) | (1 << 18) | (1 << 19)
                                                // 1 << 16 - hn.tsk
                                                // 2 << 16 - byip.tsk
                                                // 3 << 16 - dirhost.tsk
                                                // 4 << 16 - wwwpairs.tsk
                                                // 5 << 16 - splitter_hypo.tsk
                                                // 6 << 16 - splitter_inttsk.tsk
                                                // 7 << 16 - snippets.tsk
             , BITFLAG_PRI_NOINTPAGES    = 1 << 15 //    32768: set this priority flag for hosts equality of which is decided only on main pages.
             , BITFLAG_PRI_INTPAGES      = 1 << 20 //  1048576: set this priority flag for hosts in which internal pages are being fetched
             , BITFLAG_SOFT_MIRROR       = 1 << 21 //  2097152: soft mirror
             , BITFLAG_UNSAFE_MIRROR     = 1 << 22 //  4194304: not safe mirror - don't span host factors from main mirror
             , BITFLAG_WWW_PAIR          = 1 << 23 //  8388608: host is www pair to its host directive.
             , BITFLAG_PREF_MAIN_MIRROR  = 1 << 24 // 16777216: host should be selected as main mirror.
             , BITFLAG_NOT_MAIN_MIRROR   = 1 << 25 // 33554432: host should not be selected as main mirror.
             , BITFLAG_SPAM              = 1 << 26 // 67108864: host marked as spam

             , BITFLAG_BAD_CERT          = 1 << 27 // 134217728: https host has problems with certificate validation

             , BITFLAG_PREV_MIRROR       = 1 << 28 // 268435456: host is in previous mirror database.
             , BITFLAG_PREV_MAIN_MIRROR  = 1 << 29 // 536870912: host is main mirror in previous mirror database.
             , BITFLAG_WEBMASTER_SIGNAL  = 1 << 30 // 1073741824: Signal from webmaster
             , BITFLAG_CONFLICTING_HOSTDIR  = 1 << 31 // 1073741824: Signal from webmaster
             ;

}
