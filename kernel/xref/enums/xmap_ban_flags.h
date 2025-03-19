#pragma once

namespace NBanFlags {

    enum EDocSpmMarksRepacked { // repacked to 18-bits for XMap
        DSMR_BadForum = 1,      // replaces DSM_SpamQ
        DSMR_Spam = 2,
        DSMR_Garbage = 4,       // replaces DSM_SbNolink
        DSMR_Linkator = 8,
        DSMR_NoInLnk = 16,
        DSMR_NoOutLnk = 32,
        DSMR_CyExclude = 64,    // replaces DSM_NoPr
        DSMR_Ags = 128,         // replaces DSM_BadTarget
        DSMR_MfasMasks = 256,   // replaces now - DSM_IncPessP4, first - DSM_LinkPotOld
        DSMR_CatNoOutLnk = 512,
        DSMR_CatNoXLnk = 1024,
        DSMR_NoXLnk = 2048,
        DSMR_Pessimize = 4096,
        DSMR_LinkBlast = 8192,
        DSMR_MfasDyn = 16384,   // replaces now - DSM_CyExclude, first - DSM_SandBox2
        DSMR_Malicious = 32768,
        DSMR_IncPessP4 = 65536, // replaces DSM_SeoNoXLnk
        DSMR_IncPessP8 = 131072,// replaces DSM_SeoOwNoXLnk

        // DSM_PessimizeDocs copied to DSM_Pessimize flag while import all flags, see load_spam_categs()
    };

} // namespace NBanFlags

enum ESeoTrash {
    EST_None    = 0,
    EST_Trash   = 1,
    EST_Blast   = 2,
    EST_Ags     = 3,
};
