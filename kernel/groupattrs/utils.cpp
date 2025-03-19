#include "utils.h"

#include <util/folder/path.h>
#include <util/generic/yexception.h>
#include <util/stream/file.h>
#include <util/stream/str.h>
#include <library/cpp/string_utils/url/url.h>
#include <util/string/cast.h>
#include <util/system/fs.h>

#include <kernel/doom/offroad_common/accumulating_output.h>

#include <kernel/doom/wad/wad.h>

#include <library/cpp/offroad/custom/null_serializer.h>
#include <library/cpp/offroad/custom/subtractors.h>
#include <library/cpp/offroad/custom/ui32_vectorizer.h>
#include <library/cpp/offroad/key/fat_key_reader.h>
#include <library/cpp/offroad/key/key_reader.h>

namespace NGroupingAttrs {
    using TKeyReader = NOffroad::TKeyReader<ui32, NOffroad::TUi32Vectorizer, NOffroad::TI1Subtractor>;
    using TModel = TKeyReader::TModel;
    using TTable = TKeyReader::TTable;

    namespace NPrivate {

        class TCategToNameWadReader {
        public:
            TCategToNameWadReader(const TString& filename) {
                Y_ENSURE(filename.EndsWith(".wad"));
                Wad_ = NDoom::IWad::Open(filename);
                Model_.Load(Wad_->LoadGlobalLump(NDoom::TWadLumpId(NDoom::NameToCategIndexType, NDoom::EWadLumpRole::KeysModel)));
                Table_.Reset(Model_);
                Keys_ = Wad_->LoadGlobalLump(NDoom::TWadLumpId(NDoom::EWadIndexType::NameToCategIndexType, NDoom::EWadLumpRole::Keys));
                Reader_.Reset(&Table_, Keys_);
            }

            bool ReadKey(TStringBuf* name, ui32* categoryId) {
                return Reader_.ReadKey(name, categoryId);
            }

        private:
            THolder<NDoom::IWad> Wad_;
            TKeyReader Reader_;
            TBlob Keys_;
            TTable Table_;
            TModel Model_;
        };

    } // namespace NPrivate

    TVector<TString> ReadCategToNameWad(const TString& filename) {
        NPrivate::TCategToNameWadReader reader = NPrivate::TCategToNameWadReader(filename);
        TVector<TString> categToName;
        TStringBuf name = "";
        ui32 categoryId = 0;
        while (reader.ReadKey(&name, &categoryId)) {
            if (categToName.size() <= categoryId) {
                categToName.resize(categoryId + 1);
            }
            Y_ENSURE(categToName[categoryId].empty());
            categToName[categoryId] = ToString(name);
        }
        return categToName;
    }

    TVector<TString> ReadCategToName(const TString& filename) {
        // not to write many times if something exists then blabla
        if (NFs::Exists(filename + ".wad")) {
            return ReadCategToNameWad(filename + ".wad");
        }
        TIFStream c2n(filename);
        TVector<TString> categToName;
        TString inputLine;
        while (c2n.ReadLine(inputLine)) {
            TStringInput input(inputLine);
            ui32 categoryId;
            TString name;
            input >> categoryId >> name;
            if (categToName.size() <= categoryId) {
                categToName.resize(categoryId + 1);
            }
            Y_ENSURE(categToName[categoryId].empty());
            categToName[categoryId] = name;
        }
        return categToName;
    }

    THashMap<TString, ui32> ReadNameToCategWad(const TString& filename) {
        NPrivate::TCategToNameWadReader reader = NPrivate::TCategToNameWadReader(filename);
        THashMap<TString, ui32> nameToCateg;
        TStringBuf name = "";
        ui32 categoryId = 0;
        while (reader.ReadKey(&name, &categoryId)) {
            bool inserted = nameToCateg.insert(std::make_pair(AddSchemePrefix(ToString(name)), categoryId)).second;
            Y_ENSURE(categoryId != 0);
            Y_ENSURE(inserted, "Duplicate name in " + filename + " file: " << name);
        }
        return nameToCateg;
    }

    THashMap<TString, ui32> ReadNameToCateg(const TString& filename) {
        if (NFs::Exists(filename + ".wad")) {
            return ReadNameToCategWad(filename + ".wad");
        }
        TFileInput c2nFile(filename);
        THashMap<TString, ui32> nameToCateg;
        TString line;
        ui32 i = 0;
        while (c2nFile.ReadLine(line)) {
            TStringInput input(line);
            TString host;
            ui32 categoryId;
            input >> categoryId >> host;
            ++i;
            Y_ENSURE(categoryId == i, "Expected sorted h.c2n file, but " << categoryId << " found at position " << i);

            // This is an old stuff:
            // * in h.c2n file 'http://' host don't have protocol prefix but 'https' host do have one;
            // * in kiwi all hosts has a protocol prefix.
            host = AddSchemePrefix(host);

            bool inserted = nameToCateg.insert(std::make_pair(host, categoryId)).second;
            Y_ENSURE(inserted, "Duplicate host in h.c2n file: " << host);
        }
        return nameToCateg;
    }

} // namespace NGroupingAttrs
