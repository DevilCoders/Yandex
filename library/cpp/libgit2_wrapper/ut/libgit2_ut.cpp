#include <library/cpp/testing/unittest/tests_data.h>
#include <library/cpp/testing/unittest/registar.h>

#include <library/cpp/libgit2_wrapper/util.h>

#include <contrib/libs/libgit2/include/git2.h>
#include <contrib/libs/libgit2/include/git2/signature.h>

#include <util/folder/iterator.h>
#include <util/folder/tempdir.h>
#include <util/stream/file.h>

Y_UNIT_TEST_SUITE(TParseSignatureTests) {
    Y_UNIT_TEST(Empty) {
        git_libgit2_init();

        git_signature* result;
        UNIT_ASSERT(git_signature_from_buffer(&result, "") != 0);
    }
}

Y_UNIT_TEST_SUITE(TCommitTests) {
    using namespace NLibgit2;

    Y_UNIT_TEST(CheckCommit) {
        TTempDir repoPath;
        TRepository repo = TRepository::Init(repoPath.Path());
        TIndex index = repo.Index();
        TTree tree(repo, index.WriteTree());
        TSignature author("Sergey Makarov", "setser@yandex-team.ru");

        TOid newCommit = repo.CommitToHead(tree, author, author, "UTF-8", "Initial commit");
        UNIT_ASSERT(newCommit.Get() != nullptr);
        UNIT_ASSERT(repo.HeadOid() == newCommit);
        TCommit headCommit(repo, newCommit);
        UNIT_ASSERT(headCommit.Message() == "Initial commit");
        UNIT_ASSERT(headCommit.ParentCount() == 0);
    }
}

Y_UNIT_TEST_SUITE(TRepositoryTests) {
    using namespace NLibgit2;

    Y_UNIT_TEST(TestHead) {
        TFsPath repoRoot(GetWorkPath() / TFsPath("dot2tex"));
        TRepository repo(repoRoot);
        UNIT_ASSERT(repo.Get());
        Cout << repo.HeadOid() << Endl;
    }

    Y_UNIT_TEST(TestCheckout) {
        TFsPath repoRoot(GetWorkPath() / TFsPath("dot2tex"));
        TRepository repo(repoRoot);
        TOid rev("de9006b47ecbefca388fc68cf6b3804671f37eab");
        UNIT_ASSERT_EQUAL(rev.ToString(), "de9006b47ecbefca388fc68cf6b3804671f37eab");
        UNIT_ASSERT(repo.Get());
        TOdbObject obj = repo.FindFile(rev.ToString(), TFsPath("dot2tex/__init__.py"));
        Cout << obj.Data() << Endl;
        TCommit commit(repo, rev);
        UNIT_ASSERT(commit.Get());
        Cout << commit.Message() << Endl;
        Cout << commit.Time() << Endl;
    }

    Y_UNIT_TEST(TestForEachFile) {
        TFsPath repoRoot(GetWorkPath() / TFsPath("dot2tex"));
        TRepository repo(repoRoot);
        UNIT_ASSERT(repo.Get());
        repo.ForEachFile("9aabb3243b5b10f6fd73be531ab79d067e5edf73", TFsPath(),
            [](TTreeEntryRef, const TFsPath& path) {
                Cout << path << Endl;
            });
        repo.ForEachFile("de9006b47ecbefca388fc68cf6b3804671f37eab", TFsPath("docs"),
            [](TTreeEntryRef, const TFsPath& path) {
                Cout << path << Endl;
            });
    }

    Y_UNIT_TEST(TestForEachDelta) {
        TFsPath repoRoot(GetWorkPath() / TFsPath("dot2tex"));
        TRepository repo(repoRoot);
        UNIT_ASSERT(repo.Get());
        TDiff diff(repo, "9aabb3243b5b10f6fd73be531ab79d067e5edf73", "de9006b47ecbefca388fc68cf6b3804671f37eab");
        diff.ForEachDelta([](const TDiffDelta &delta, float) {
            Cout << static_cast<int>(delta.Status) << Endl;
            Cout << "- " << static_cast<int>(delta.OldFile.Mode) << ' ' << delta.OldFile.Path << Endl;
            Cout << "+ " << static_cast<int>(delta.NewFile.Mode) << ' ' << delta.NewFile.Path << Endl;
        });
    }

    Y_UNIT_TEST(TestDiffWithHead) {

        TFsPath repoRoot(GetWorkPath() / TFsPath("dot2tex"));

        TRepository repo(repoRoot);
        UNIT_ASSERT(repo.DiffWithHead().empty());

        {
            TFileOutput junk(repoRoot / "junk.txt");
            junk << "A piece of junk";
        }

        UNIT_ASSERT(!repo.DiffWithHead().empty());
    }
}
