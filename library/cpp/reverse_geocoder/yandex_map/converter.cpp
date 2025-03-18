#include "converter.h"

#include "toponym_checker.h"
#include "toponym_traits.h"

#include <library/cpp/reverse_geocoder/library/fs.h>
#include <library/cpp/reverse_geocoder/logger/log.h>
#include <library/cpp/reverse_geocoder/proto_library/writer.h>
#include <library/cpp/xml/document/xml-textreader.h>

#include <util/generic/queue.h>
#include <util/stream/file.h>
#include <util/stream/input.h>
#include <util/stream/zlib.h>
#include <util/string/split.h>
#include <util/system/yassert.h>

#include <thread>

#include <util/stream/input.h>

using namespace NReverseGeocoder;
using namespace NYandexMap;

namespace {
    class TConverter {
    public:
        TConverter(IInputStream& inputStream, NProto::TWriter& writer, ToponymChecker& checker, const TConfig& config)
            : XmlReader(inputStream, NXml::EOption::Huge)
            , Writer(&writer)
            , Checker(checker)
            , Config(config)
        {
            StringSplitter(Config.SkipKindList).Split(',').AddTo(&SkipKindList);
            Y_ASSERT(SkipKindList.size());

            static const TString skipElems = "Houses,ArrivalPoint,Exclusions";
            StringSplitter(skipElems).Split(',').AddTo(&SkipElementsList);
            Y_ASSERT(SkipElementsList.size());
        }

        void Parse() {
            while (XmlReader.Read()) {
                try {
                    ToponymTraits toponym{XmlReader, SkipKindList, SkipElementsList};
                    if (Checker.IsToponymOK(toponym)) {
                        Writer->Write(toponym.Region);
                        ++RegionsAdded;
                    }
                } catch (const std::exception& ex) {
                    WARNING_LOG << "EX: " << ex.what() << "\n";
                }
            }

            INFO_LOG << "BORDERS_ADDED: " << RegionsAdded << "\n";
            Checker.ShowReminder();
        }

    private:
        NXml::TTextReader XmlReader;
        NProto::TWriter* Writer;
        ToponymChecker& Checker;
        const TConfig& Config;

        int RegionsAdded = 0;
        StrBufList SkipKindList;
        TVector<TString> SkipElementsList;
    };
}

static void Parse(const TVector<TString>& input, const TString& output, const TConfig& config) {
    if (input.empty())
        ythrow yexception() << "There is no input files";

    TQueue<TString> queue;
    for (const TString& s : input)
        queue.push(s);

    NProto::TWriter writer(output.c_str());

    TMutex mutex;
    TVector<std::thread> threads(config.Jobs);

    for (size_t i = 0; i < threads.size(); ++i) {
        auto job = [&]() {
            while (true) {
                TString currentInput;
                size_t currentIndex;

                {
                    TGuard<TMutex> lock(mutex);
                    if (queue.empty())
                        break;
                    currentInput = queue.front();
                    INFO_LOG << "Will be parsed: " << currentInput << Endl;
                    currentIndex = queue.size();
                    queue.pop();
                }

                try {
                    TAutoPtr<ToponymChecker> checkerPtr;
                    if (config.CountryId) {
                        checkerPtr.Reset(new CheckerViaGeodata(config.GeoDataFileName, config.CountryId));
                    } else {
                        checkerPtr.Reset(new CheckerViaGeomapping(config.GeoMappingFileName));
                    }

                    TMappedFileInput inputStream(currentInput);
                    TAutoPtr<IInputStream> decompressedPtr(config.Gz ? new TZLibDecompress(&inputStream) : nullptr);

                    INFO_LOG << "Parse " << currentInput << " (" << input.size() - currentIndex + 1 << "/" << input.size() << ")..." << Endl;
                    TConverter converter(config.Gz ? *decompressedPtr : inputStream, writer, *checkerPtr, config);
                    converter.Parse();
                    INFO_LOG << currentInput << " parsed" << Endl;

                } catch (const yexception& e) {
                    ERROR_LOG << "Unable load " << currentInput << ": " << e.what() << Endl;
                }
            }
        };

        threads[i] = std::thread(job);
    }

    for (size_t i = 0; i < threads.size(); ++i)
        threads[i].join();
}

namespace {
    inline void SortFilesBySizeDescending(TVector<TString>& list) {
        Sort(list.begin(), list.end(), [](const TString& a, const TString& b) {
            return GetFileLength(a) > GetFileLength(b);
        });
    }
}

void NReverseGeocoder::NYandexMap::Convert(const TString& inputPath, const TString& outputPath, const TConfig& config) {
    TVector<TString> list = GetDataFilesList(inputPath.data());
    SortFilesBySizeDescending(list);
    Parse(list, outputPath, config);
}
