#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc47() {
    // google search by image
    {
        TInfo info(SE_GOOGLE, ST_BYIMAGE, "", SF_SEARCH);
        KS_TEST_URL("http://images.google.com/search?hl=ru&tbs=sbi%3AAMhZZitHt9eCzSMdmGVTSWATbjMBh9lEtcFErsqRAHFHrRXoNs3a9bOkyn0Ss_1_1kyolHOHj5CQTHMtTO60sopyxSxl6C2K1DrM0FlGH38HYVDalK_1kE4q677XxkR-29CN3i3l-wppnlFniDTrT-4S3BkrpmsnaEHXL1UqY82whIoGslXjtIs-i-y9M96U4Dtf9al5a_1JeHBJEuX490rM3EY38wxUifIobwYotMZ8Qy50GbLaBDvz5KF0JIJQtFsswTHt8admu6aQIUN7npYYlcsPYvCn2EohLuWF9ChYfuooYMyI7wLaCJX1PFk35OUAkLTPV4W7QwUDuoIFAcDRCfiCDF9uV9-n6HNxe6UHpwH_1GnOeDeY-sOWSM3pq22SSj7FYJvSJ2lPyIH5vB--v77R815720CeEXkKELQ39wMFvjqHwqvSzqWGHlkrGdW9oRyhS1F30MY0686h9PIf02O6D_1AAh2s7uqo7RkJWlVylPip-gguv0QbDUZ3ivvmXWV9Ut77MLltYMVaywdRRq7ZVDtuvFyaQtdNRt3fwYFj0VPoOmMsDvXrTycokZXMIi-OlyzYU4SDOVzN1q7ooXEHkeOzqJtCe6syahK3gPgitx3QSz_1WflaHI_1O3rVQWnQmpflySylrxQDnaiSMZXXMaNEOmjZdPf12yOc2-wNZBzUgaThNH-ZMptrAG7gkrJ07ZOGvaL63-L8dE1iFFEIxxorj-2C1RWwP6htXBI2F4pWwcsZl3tpu40YwfbArHip58BLd_1MiC-693weIM90yyDovvpFsRGixmZrmiRHxrsHzYq7lzH9cQ4aptX2LcxXAZ0U8ssFZRQwiOVfaEX76MBeZhxt9U7elVKaqISMHeEGUj05hwj7lrcy5yqLitwlYXQdX_1oEvUTvI3Jt2sp9kQ2cm3KTc8JBfHb5EKmoBqNFjk8DnbtlTrTynJT_1m6n4wklJDlq_1C4Zv2QbsIf5BT9cuSTKoi4JVnspcY3fhSiLVZcBzvSDPXuMuTlLNKUJ-0ql-S6wv92EE4UyhIemNNpaBrGfFNKd-8PdVfNxCJ0KQ3JIVvSKvRajXo243ECCHA5EOxMlUFq96DAaYYGe056bEA4Y74FHCJdAf-UOXoXOMM8mRSEGfRSCqZeCz34S8yMybHERpUdF-OAzQvLXuojbV7mzrQY9uYg8bMIsv3Nr73kG3I7ewsV0jfVZyxvPYvsOqA0aLNtPSFet8EHuRPqQY1M0i5eAdyrn9tjSej6Tx31Oj83QXo46ejQXb9yBleiHO47mg0x_18Z9i47OC6uIO8pf2QuibBXtSSBSNYOY1Ow42GUacuP5x1spaVLHxZG8ZIAdGmyblkQEa1K5FHtApeFCu6lwDTbo_1RTXrwe2ixyEX0dThS9a1U", info);
        info.Query = "http://www.hamaratabla.com/makale/2641,arpa-sehriyeli-mercimekli-salata.htm";
        KS_TEST_URL("http://www.google.com.tr/imgres?imgurl=&imgrefurl=http:%2F%2Fwww.hamaratabla.com%2Fmakale%2F2641,arpa-sehriyeli-mercimekli-salata.htm&h=0&w=0&tbnid=Cxojt-2NIdvl-M&zoom=1&tbnh=183&tbnw=275&docid=3RUw3vxKegJqgM&tbm=isch&ei=PKtSVNDUEILyapf3gNAJ&ved=0CAQQsCUoAA", info);
        info.Query = "";
        KS_TEST_URL("https://www.google.com.tr/search?tbs=simg:CAES1AEa0QELEKjU2AQaAggLDAsQsIynCBpiCmAIAxIoyRDUEO0c6wShD-ka6hrvGucf-g6WMZUxsiXzMa0xmDH3O-s7hjCxMRowOS_12lOpNxHcQJec_1VJCMZWA6q6At9XM-8BvtDXmbVMrvofMarNoPaBQwU31slmzSIAIMCxCOrv4IGgoKCAgBEgReVRBXDAsQne3BCRo_1CgoKCGNsb3RoaW5nCgsKCWhhaXJzdHlsZQoICgZzbGVldmUKEwoRZmFzaGlvbiBhY2Nlc3NvcnkKBQoDYXJtDA&tbm=isch&sa=X&ved=0CBkQsw5qFQoTCOW6p7zuocgCFYpbLAodSgYNJg&biw=1366&bih=664", info);
    }
}
