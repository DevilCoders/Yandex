#include <util/system/defaults.h>

const ui64 FACTORS_HASH = 14786346373181874490ULL;

const ui64 CURRENT_FACTORS_HASH =
    #include <antirobot/daemon/factors_hash.h>
    ;

/*
   Automatic checking factors version

   After adding or removing a new factor:
   1. Run "./tools/fnames/fnames -H <NEW_VERSION>" (fnames must be built from the same source tree)
   2. Increase FACTORS_VERSION value at daemon_lib/factors.h
   3. Update FACTORS_HASH in this file with printed value
   4. Run './tools/fnames/fnames > daemon_lib/factors_versions/factors_XX.inc', where XX is a new version
   5. Add new factors_XX.inc file under version control
   6. Update daemon_lib/convert_factors.cpp
   7. Check project is buildable
   8. Commit changes
*/

static_assert(FACTORS_HASH == CURRENT_FACTORS_HASH, "expect FACTORS_HASH == CURRENT_FACTORS_HASH");
