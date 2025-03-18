#pragma once

#include <nginx/modules/strm_packager/src/base/workers.h>
#include <nginx/modules/strm_packager/src/common/source.h>

namespace NStrm::NPackager {

    template <typename T>
    concept TDataConverter =
        std::move_constructible<T> &&
        requires(
            T converter,
            TVector<TTrackInfo const*>& tracksInfo,
            TTrackInfo const* const trackInfo,
            const TSampleData& sd)
    {
        { T::NeedConvertion(tracksInfo) } -> std::same_as<bool>;
        { converter(trackInfo) } -> std::same_as<TTrackInfo const*>;
        { converter(sd) } -> std::same_as<TSampleData>;
    };

    template <TDataConverter TConverter>
    class TSourceConverter: public ISource {
    public:
        template <typename... TArgs>
        static TSourceFuture Make(
            TRequestWorker& request,
            TSourceFuture source,
            TArgs... args);

        TSourceConverter(
            TRequestWorker& request,
            ISource* source,
            TConverter&& converter);

    private:
        TIntervalP FullInterval() const override;
        TVector<TTrackInfo const*> GetTracksInfo() const override;
        TRepFuture<TMediaData> GetMedia() override;

        TIntervalP Interval;
        TVector<TTrackInfo const*> Info;
        TRepFuture<TMediaData> Data;

        ISource* const Source;
        const TConverter Converter;
    };

    // ======= implementation

    template <TDataConverter TConverter>
    template <typename... TArgs>
    inline TSourceFuture TSourceConverter<TConverter>::Make(
        TRequestWorker& request,
        TSourceFuture source,
        TArgs... args)
    {
        TConverter converter(std::forward<TArgs>(args)...);
        return source.Apply([conv = std::move(converter), &request](const TSourceFuture& future) mutable -> ISource* {
            ISource* original = future.GetValue();
            Y_ENSURE(original);
            if (!TConverter::NeedConvertion(original->GetTracksInfo())) {
                return original;
            }

            return (ISource*)request.GetPoolUtil<TSourceConverter>().New(request, original, std::move(conv));
        });
    }

    template <TDataConverter TConverter>
    inline TSourceConverter<TConverter>::TSourceConverter(
        TRequestWorker& request,
        ISource* const source,
        TConverter&& converter)
        : ISource(request)
        , Source(source)
        , Converter(std::move(converter))
    {
        Y_ENSURE(Source);
        Interval = Source->FullInterval();

        Info = Source->GetTracksInfo();
        for (TTrackInfo const*& trackInfo : Info) {
            trackInfo = Converter(trackInfo);
            Y_ENSURE(trackInfo);
        }
    }

    template <TDataConverter TConverter>
    inline TIntervalP TSourceConverter<TConverter>::FullInterval() const {
        return Interval;
    }

    template <TDataConverter TConverter>
    inline TVector<TTrackInfo const*> TSourceConverter<TConverter>::GetTracksInfo() const {
        return Info;
    }

    template <TDataConverter TConverter>
    inline TRepFuture<TMediaData> TSourceConverter<TConverter>::GetMedia() {
        if (Data.Initialized()) {
            return Data;
        }

        Data = Source->GetMedia().Apply([this](const TMediaData& data) -> TMediaData {
            Y_ENSURE(data.TracksSamples.size() == Info.size());

            TMediaData result;
            result.Interval = data.Interval;
            result.TracksInfo = Info;
            result.TracksSamples.resize(Info.size());

            for (size_t i = 0; i < Info.size(); ++i) {
                result.TracksSamples[i].reserve(data.TracksSamples.size());
                for (const TSampleData& sd : data.TracksSamples[i]) {
                    result.TracksSamples[i].push_back(Converter(sd));
                }
            }

            return result;
        });

        return Data;
    }

}
