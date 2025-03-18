#pragma once

#include <util/generic/string.h>
#include <util/generic/yexception.h>

namespace NLibgit2 {

    /*
     * Applies given patch hunks to given content
     *
     * @param source    content on which hunks should be applied
     * @param hunksData content of hunks to apply
     * @param path      path at which we are applying hunks
     *
     * Buils a patch with dummy header and the given hunks and use libgit2 to apply
     * that to the provided buffer.
     * Returns the resulting content after hunk application. If the hunks failed
     * to apply, NLibgit2::TException is raised
     */
    TString ApplyPatch(
        const TString& source,
        const TString& hunksData,
        const TString& path);

}
