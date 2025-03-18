#include <contrib/libs/libgit2/include/git2.h>
#include <library/cpp/openssl/init/init.h>

namespace NLibgit2::NPrivate {
    struct TInit {
        TInit() {
            InitOpenSSL();
            git_libgit2_init();
        }

        ~TInit() {
            git_libgit2_shutdown();
        }
    };

    const TInit init;

} // namespace NLibgit2::NPrivate
