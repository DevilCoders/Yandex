#include "baseconf.h"
#include <kernel/indexer_iface/yandind.h>
#include <library/cpp/mime/types/mime.h>
#include <util/folder/dirut.h>
#include <util/system/fs.h>

namespace {
    bool CheckPathExists(const TString& path) {
        return (!path || NFs::EnsureExists(path));
    }
}

TBaseConfig::TBaseConfig(const INDEX_CONFIG* config /*= NULL*/)
{
    ui32 iFlag = config ? config->IndexingFlag : (ui32)INDFL_DEFAULT;
    UseExtProcs = iFlag & INDFL_USE_EXTENDED_PROCESSORS;
    UseFreqAnalize = false;
    DocCount = ((!config || config->PortionDocCount < 50) ? 50 : config->PortionDocCount);
    MaxMemory = (config ? config->PortionMaxMemory : 0);
    if (MaxMemory < DocCount * 30000)
        MaxMemory = DocCount * 30000;
    if (config) {
        PureTrieFile = config->PureTrieFile;
        StopWordFile = config->StopWordFile;
        DocProperties = config->DocProperties;
        PassageProperties = config->PassageProperties;
        Groups = config->Groups;
        RecognizeLibraryFile = config->RecognizeLibraryFile;
        for (ui32 i = 0; i < config->formatSize; i++) {
            MimeTypes mimeType = mimeByStr(config->formats[i].MimeType);
            if (mimeType == MIME_HTML)
                ParserConfig = config->formats[i].Config;
            else if (mimeType == MIME_XML)
                XmlParserConf = config->formats[i].Config;
        }
        DefaultLangMask = config->LangMask;
    }
    StoreIndex = true;
    FinalMerge = false;
    NoMorphology = iFlag & INDFL_NO_MORPHOLOGY;
    StoreSegmentatorData = !(iFlag & INDFL_DISCARD_SEGMENTATOR_DATA);

    UseArchive = !(iFlag & INDFL_DISCARDARCHIVE);
    UseFullArchive = iFlag & INDFL_STOREFULLARCHIVE;
    SaveHeader = true;

    UseOldC2N = !(iFlag & INDFL_REINDEX);
}

void TBaseConfig::CheckConfigsExists() const {
    CheckPathExists(StopWordFile);
    CheckPathExists(PureTrieFile);
    CheckPathExists(RecognizeLibraryFile);
    CheckPathExists(ParserConfig);
    CheckPathExists(XmlParserConf);
}

void TBaseConfig::ResolveWorkDir() {
    TString cur_dir = NFs::CurrentWorkingDirectory();
    SlashFolderLocal(cur_dir);
    if (!!WorkDir) {
        resolvepath(WorkDir, cur_dir);
    } else {
        WorkDir = cur_dir;
    }
    SlashFolderLocal(WorkDir);
}
