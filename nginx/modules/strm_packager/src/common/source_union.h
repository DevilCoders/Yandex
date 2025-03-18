#pragma once

#include <nginx/modules/strm_packager/src/common/source.h>

#include <util/generic/queue.h>

namespace NStrm::NPackager {
    class TSourceUnion: public ISource {
    public:
        struct TConfig {
            TConfig();
            explicit TConfig(const TLocationConfig& locationConfig);

            void Check() const;

            TMaybe<Ti64TimeP> MaxMediaTsGap;
        };
        using TSourceMaker = std::function<TSourceFuture()>;

        static TSourceFuture Make(
            TRequestWorker& request,
            const TConfig& config,
            TVector<TSourceMaker> SourceMakers);

        static TSourceFuture Make(
            TRequestWorker& request,
            const TConfig& config,
            TSourceFuture source,
            TSourceMaker backupMaker);

        TSourceUnion(
            TRequestWorker& request,
            const TConfig& config,
            ISource& source,
            TSourceMaker backupMaker);

    public:
        TIntervalP FullInterval() const override;
        TVector<TTrackInfo const*> GetTracksInfo() const override;
        TRepFuture<TMediaData> GetMedia() override;

    private:
        void AcceptSourceMedia(const TRepFuture<TMediaData>::TDataRef& dataRef);
        void AcceptBackupMedia(const TRepFuture<TMediaData>::TDataRef& dataRef);

        void RequireBackup();

        bool BackupEnabled() {
            return Backup.Initialized();
        }

        void UpdateMedia();
        void FinishSourceMedia();
        void FinishBackupMedia();
        static bool WithLargeGap(const TMediaData& media, const Ti64TimeP maxGap);

    private:
        ISource& Source;

        TSourceMaker BackupMaker;
        TSourceFuture Backup;
        std::exception_ptr BackupException;

        const TIntervalP Interval;
        const TVector<TTrackInfo const*> TracksInfo;

        const Ti64TimeP MaxMediaTsGap;

        // media
        bool MediaPromiseReady;
        Ti64TimeP MediaPromiseEnd;
        TRepPromise<TMediaData> MediaPromise;

        TDeque<TMediaData> SourceMedia;
        TMediaData BackupMedia;
    };
}
