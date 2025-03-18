#include "output_arc.h"

#include <kernel/tarc/docdescr/docdescr.h>

#include <library/cpp/mime/types/mime.h>

#include <library/cpp/charset/codepage.h>
#include <util/stream/output.h>

namespace NSnippets {

    //TODO: print more stuff
    void TArcOutput::Process(const TJob& job) {
        Cout << "url: " << job.ArcUrl << Endl;
        if (job.DocD.IsAvailable()) {
            Cout << "docd ::" << Endl;
            Cout << "URL=" << job.DocD.get_url() << Endl;
            Cout << "INDEXDATA=" << job.DocD.get_mtime() << Endl;
            Cout << "MIMETYPE=" << strByMime((MimeTypes)job.DocD.get_mimetype()) << Endl;
            Cout << "CHARSET=" << NameByCharset(job.DocD.get_encoding()) << Endl;
            Cout << "*HOSTID=" << job.DocD.get_hostid() << Endl;
            Cout << "*URLID=" << job.DocD.get_urlid() << Endl;
            Cout << "*SIZE=" << job.DocD.get_size() << Endl;
        }
        if (job.DocInfos.Get()) {
            Cout << "docinfos ::" << Endl;
            for (TDocInfos::const_iterator it = job.DocInfos->begin(); it != job.DocInfos->end(); ++it) {
                Cout << it->first << "=" << it->second << Endl;
            }
            Cout << "::" << Endl;
        }
    }

} //namespace NSnippets
