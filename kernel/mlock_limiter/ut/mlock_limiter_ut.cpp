#include <library/cpp/testing/gtest/gtest.h>

#include <kernel/mlock_limiter/mlock_limiter.h>

#include <util/folder/tempdir.h>
#include <util/generic/size_literals.h>
#include <util/stream/file.h>
#include <util/system/info.h>



class MlockLimiterTest : public ::testing::Test {
public:
    static inline const NFileMapper::TBytes PageSize = NSystemInfo::GetPageSize();

    void SetUp() override {
        Dir_.ConstructInPlace();
        InitFiles(5);
    }

    void TearDown() override {
        Dir_.Clear();
        Files_.clear();
    }

private:
    void InitFiles(int count) {
        Dir_->Path().MkDirs();
        for (int i = 0; i < count; ++i) {
            TFsPath path = Files_.emplace_back(Dir_->Path() / ToString(i));
            TFileOutput{path}.Write(TString(PageSize, 'a' + i));

            TFileStat stat;
            path.Stat(stat);
            Y_VERIFY(stat.Size == PageSize);
        }
    }

protected:
    TMaybe<TTempDir> Dir_;
    TVector<TFsPath> Files_;
};

TEST_F(MlockLimiterTest, Smoke) {
    NFileMapper::TFileMapper mapper{{.MappedLimit = PageSize * 2}};
    EXPECT_EQ(mapper.MappedSize(), 0u);
    EXPECT_EQ(mapper.MappedLimit(), PageSize * 2);

    TVector<TBlob> blobs;
    EXPECT_NO_THROW(blobs.push_back(mapper.MapFileOrThrow(Files_[0], NFileMapper::EMapMode::Unlocked)));
    EXPECT_EQ(mapper.MappedSize(), PageSize);
    EXPECT_NO_THROW(blobs.push_back(mapper.MapFileOrThrow(Files_[0], NFileMapper::EMapMode::Unlocked)));
    EXPECT_NO_THROW(blobs.push_back(mapper.MapFileOrThrow(Files_[0], NFileMapper::EMapMode::Unlocked)));
    EXPECT_NO_THROW(blobs.push_back(mapper.MapFileOrThrow(Files_[0], NFileMapper::EMapMode::Unlocked)));
    EXPECT_NO_THROW(blobs.push_back(mapper.MapFileOrThrow(Files_[0], NFileMapper::EMapMode::Unlocked)));
    EXPECT_EQ(mapper.MappedSize(), PageSize);

    EXPECT_NO_THROW(blobs.push_back(mapper.MapFileOrThrow(Files_[1], NFileMapper::EMapMode::Unlocked)));
    EXPECT_NO_THROW(blobs.push_back(mapper.MapFileOrThrow(Files_[1], NFileMapper::EMapMode::Unlocked)));
    EXPECT_EQ(mapper.MappedSize(), 2 * PageSize);

    EXPECT_FALSE(mapper.TryMapFile(Files_[2], NFileMapper::EMapMode::Unlocked));
    EXPECT_FALSE(mapper.TryMapFile(Files_[2], NFileMapper::EMapMode::Unlocked));
    EXPECT_FALSE(mapper.TryMapFile(Files_[2], NFileMapper::EMapMode::Unlocked));
    EXPECT_EQ(mapper.MappedSize(), 2 * PageSize);

    blobs.clear();
    EXPECT_EQ(mapper.MappedSize(), 2 * PageSize);
    mapper.ReleaseUnusedFiles();
    EXPECT_EQ(mapper.MappedSize(), 0u);

    EXPECT_TRUE(mapper.TryMapFile(Files_[2], NFileMapper::EMapMode::Unlocked));
    EXPECT_TRUE(mapper.TryMapFile(Files_[3], NFileMapper::EMapMode::Unlocked));
    EXPECT_TRUE(mapper.TryMapFile(Files_[0], NFileMapper::EMapMode::Unlocked));
    EXPECT_TRUE(mapper.TryMapFile(Files_[4], NFileMapper::EMapMode::Unlocked));
    EXPECT_EQ(mapper.MappedSize(), PageSize);
    mapper.ReleaseUnusedFiles();
    EXPECT_EQ(mapper.MappedSize(), 0u);
}
