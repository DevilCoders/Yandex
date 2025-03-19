#include <kernel/tarc/docdescr/docdescr.h>
#include <library/cpp/testing/unittest/registar.h>


Y_UNIT_TEST_SUITE(TDocDescrTests) {
    static void SetBuild(TBuildInfoExt* build, const TString& branch, ui32 revision) {
        build->Revision = revision;
        strncpy(build->SvnUrl, branch.data(), branch.size());
        build->SvnUrl[branch.size()] = '\0';
    }

    static void SetupExtensions(TDocDescr* desc) {
        TDocInfoExt info;
        info.Info[0] = 'I';
        UNIT_ASSERT_VALUES_EQUAL(desc->AddExtension(DocInfo, sizeof(info), &info), 0);

        TBinDocInfoExt binInfo;
        binInfo.BinFormat = 123;
        UNIT_ASSERT_VALUES_EQUAL(desc->AddExtension(BinDocInfo, sizeof(TBinDocInfoExt), &binInfo), 0);


        TBuildInfoExt build;
        SetBuild(&build, "trunk", 100500);
        UNIT_ASSERT_VALUES_EQUAL(desc->AddExtension(BuildInfo, build.GetSize(), &build), 0);
    }

    static void SetupUrlDescription(TDocDescr* desc, size_t urlid = 2222) {
        UNIT_ASSERT(desc->IsAvailable());

        desc->set_hostid(1111);
        desc->set_urlid(urlid);
        desc->set_size(3);
        desc->set_mtime(12345);
        desc->set_mimetype(MIME_TEXT);

        desc->SetUrlAndEncoding("http://example.com/", CODES_UTF8);
    }

    static void CheckUrlDescription(const TDocDescr& desc, size_t urlid = 2222) {
        UNIT_ASSERT(desc.IsAvailable());

        UNIT_ASSERT_VALUES_EQUAL(desc.get_hostid(), 1111);
        UNIT_ASSERT_VALUES_EQUAL(desc.get_urlid(), urlid);
        UNIT_ASSERT_VALUES_EQUAL(desc.get_size(), 3);
        UNIT_ASSERT_VALUES_EQUAL(desc.get_mtime(), 12345);
        UNIT_ASSERT(desc.get_mimetype() == MIME_TEXT);
        UNIT_ASSERT_VALUES_EQUAL(desc.get_url(), "http://example.com/");
        UNIT_ASSERT(desc.get_encoding() == CODES_UTF8);
    }

    Y_UNIT_TEST(TestSetUrl) {
        TBuffer buf;
        TDocDescr desc;
        desc.UseBlob(&buf);

        const char* shortUrl = "http://www.example.com";
        UNIT_ASSERT(desc.IsAvailable());
        desc.SetUrl(shortUrl);
        UNIT_ASSERT_VALUES_EQUAL(desc.get_url(), shortUrl);

        const TString longUrl = TString("http://www.example.com/") + TString("i") * FULLURL_MAX;
        TString expectedLongUrl = longUrl;
        expectedLongUrl.resize(FULLURL_MAX - 1);

        desc.SetUrl(longUrl.data());
        UNIT_ASSERT_VALUES_EQUAL(desc.get_url(), expectedLongUrl);

        desc.SetUrl(TStringBuf(longUrl));
        UNIT_ASSERT_VALUES_EQUAL(desc.get_url(), expectedLongUrl);
    }

    Y_UNIT_TEST(IsAvailable) {
        TDocDescr desc;
        // Just after object was created there is no available space for data.
        UNIT_ASSERT(!desc.IsAvailable());

        desc.Clear();
        UNIT_ASSERT(!desc.IsAvailable());

        TBuffer buf;
        desc.UseBlob(&buf);
        UNIT_ASSERT(desc.IsAvailable());
        desc.Clear();
        UNIT_ASSERT(desc.IsAvailable());
    }

    Y_UNIT_TEST(UseBlob) {
        TBuffer buf;
        TDocDescr desc;

        // Set external buffer and check for available space.
        desc.UseBlob(&buf);
        UNIT_ASSERT(desc.IsAvailable());
        // Because we set empty buffer there must be no any extensions.
        UNIT_ASSERT(!desc.GetExtension(PicInfo));
        UNIT_ASSERT(!desc.GetExtension(DocInfo));
        UNIT_ASSERT(!desc.GetExtension(BuildInfo));
        UNIT_ASSERT(!desc.GetExtension(BinDocInfo));
        // Setup some extensions.
        SetupExtensions(&desc);
        //
        UNIT_ASSERT( desc.GetExtension(DocInfo));
        UNIT_ASSERT(!desc.GetExtension(PicInfo));
        UNIT_ASSERT( desc.GetExtension(BuildInfo));
        UNIT_ASSERT( desc.GetExtension(BinDocInfo));
        {
            const TBuildInfoExt* ext = (const TBuildInfoExt*)desc.GetExtension(BuildInfo);
            UNIT_ASSERT(ext);
            UNIT_ASSERT_VALUES_EQUAL(ext->SvnUrl, "trunk");
            UNIT_ASSERT_VALUES_EQUAL(ext->Revision, 100500);
        }
        {
            const TBinDocInfoExt* ext = reinterpret_cast<const TBinDocInfoExt*>(desc.GetExtension(BinDocInfo));
            UNIT_ASSERT(ext);
            UNIT_ASSERT_VALUES_EQUAL(ext->BinFormat, 123);
        }
        {
            TDocDescr newDD;
            newDD.UseBlob(buf.Data(), buf.Size());

            UNIT_ASSERT(newDD.IsAvailable());
            UNIT_ASSERT( newDD.GetExtension(DocInfo));
            UNIT_ASSERT(!newDD.GetExtension(PicInfo));
            UNIT_ASSERT( newDD.GetExtension(BuildInfo));
            UNIT_ASSERT( newDD.GetExtension(BinDocInfo));

            TBuildInfoExt build;
            SetBuild(&build, "trunk", 100501);
            // We can't add anything to description with external blob.
            UNIT_ASSERT_VALUES_EQUAL(newDD.AddExtension(BuildInfo, build.GetSize(), &build), 1);

            // But we can change existent values.
            newDD.set_mimetype(MIME_TEXT);
            try {
                // It can damage existent data.
                newDD.SetUrlAndEncoding("http://example.com/", CODES_UTF8);
                UNIT_ASSERT(false);
            } catch (yexception& e) { }

            TBuffer newBuf;
            // Copy data to new buffer.
            newDD.UseBlob(&newBuf);

            UNIT_ASSERT(buf.Size() == newBuf.Size());

            UNIT_ASSERT(newDD.IsAvailable());
            UNIT_ASSERT( newDD.GetExtension(DocInfo));
            UNIT_ASSERT(!newDD.GetExtension(PicInfo));
            UNIT_ASSERT( newDD.GetExtension(BuildInfo));
            UNIT_ASSERT( newDD.GetExtension(BinDocInfo));

            // Now we can add extensions.
            UNIT_ASSERT_VALUES_EQUAL(newDD.AddExtension(BuildInfo, build.GetSize(), &build), 0);
            UNIT_ASSERT(newDD.FindCount(BuildInfo) == 2);
            UNIT_ASSERT(newDD.FindCount(BinDocInfo) == 1);
            UNIT_ASSERT(newDD.FindCount(DocInfo) == 1);

            newDD.Clear();
            UNIT_ASSERT(newBuf.Size() == 24);
        }
    }

    Y_UNIT_TEST(UrlDescr) {
        TBuffer buf;
        {
            TDocDescr desc;
            desc.UseBlob(&buf);
            SetupUrlDescription(&desc);
            CheckUrlDescription(desc);
        }
        {
            // Use buffer from another description.
            TDocDescr desc;
            desc.UseBlob(buf.Data(), buf.Size());
            CheckUrlDescription(desc);

            // Copy url description from existing.
            TDocDescr newDD;
            TBuffer newBuf;
            newDD.UseBlob(&newBuf);
            SetupUrlDescription(&newDD, 3333);
            CheckUrlDescription(newDD, 3333);
            newDD.CopyUrlDescr(desc);
            CheckUrlDescription(newDD);
        }
        {
            // Use buffer from another description.
            TDocDescr desc;
            desc.UseBlob(&buf);
            CheckUrlDescription(desc);
        }
    }

    Y_UNIT_TEST(CopyExtensions) {
        TBuffer buf1;
        TDocDescr desc1;
        desc1.UseBlob(&buf1);
        UNIT_ASSERT(desc1.IsAvailable());
        SetupExtensions(&desc1);
        //
        TBuffer buf2;
        TDocDescr desc2;
        desc2.UseBlob(&buf2);
        UNIT_ASSERT(desc2.IsAvailable());

        {
            TBuildInfoExt build;
            SetBuild(&build, "test", 100501);
            UNIT_ASSERT_VALUES_EQUAL(desc2.AddExtension(BuildInfo, build.GetSize(), &build), 0);
        }

        desc2.CopyExtensions(desc1);

        UNIT_ASSERT(desc2.GetExtension(DocInfo));
        UNIT_ASSERT(desc2.GetExtension(BuildInfo));
        {
            const TBuildInfoExt* ext = (const TBuildInfoExt*)desc2.GetExtension(BuildInfo);
            UNIT_ASSERT(ext);
            UNIT_ASSERT_VALUES_EQUAL(ext->SvnUrl, "trunk");
            UNIT_ASSERT_VALUES_EQUAL(ext->Revision, 100500);
        }
        {
            const TBinDocInfoExt* ext = reinterpret_cast<const TBinDocInfoExt*>(desc2.GetExtension(BinDocInfo));
            UNIT_ASSERT(ext);
            UNIT_ASSERT_VALUES_EQUAL(ext->BinFormat, 123);
        }
    }

    Y_UNIT_TEST(RemoveExtensions) {
        TBuffer buf;
        TDocDescr desc;
        // Set external buffer and check for available space.
        desc.UseBlob(&buf);
        UNIT_ASSERT(desc.IsAvailable());
        // Setup some extensions and remove all of them.
        SetupExtensions(&desc);
        desc.RemoveExtensions();
        // There must be no any extensions.
        UNIT_ASSERT(!desc.GetExtension(PicInfo));
        UNIT_ASSERT(!desc.GetExtension(DocInfo));
        UNIT_ASSERT(!desc.GetExtension(BuildInfo));
        UNIT_ASSERT(!desc.GetExtension(BinDocInfo));

        // Setup some extensions and remove all of them manually.
        {
            TBuildInfoExt build;
            SetBuild(&build, "test", 100500);
            UNIT_ASSERT_VALUES_EQUAL(desc.AddExtension(BuildInfo, build.GetSize(), &build), 0);
        }
        SetupExtensions(&desc);
        // Check extensions.
        UNIT_ASSERT(desc.FindCount(DocInfo) == 1);
        UNIT_ASSERT(desc.FindCount(PicInfo) == 0);
        UNIT_ASSERT(desc.FindCount(BuildInfo) == 2);
        UNIT_ASSERT(desc.FindCount(BinDocInfo) == 1);

        UNIT_ASSERT(desc.RemoveExtension(PicInfo) == 0);
        UNIT_ASSERT(desc.GetExtension(DocInfo));
        UNIT_ASSERT(desc.GetExtension(BuildInfo));
        UNIT_ASSERT(desc.GetExtension(BuildInfo, 1));
        UNIT_ASSERT(desc.GetExtension(BinDocInfo));

        UNIT_ASSERT(desc.RemoveExtension(BuildInfo) == 0);
        UNIT_ASSERT(desc.GetExtension(DocInfo));
        UNIT_ASSERT(!desc.GetExtension(BuildInfo));
        UNIT_ASSERT(!desc.GetExtension(BuildInfo, 1));
        UNIT_ASSERT(desc.GetExtension(BinDocInfo));

        UNIT_ASSERT(desc.RemoveExtension(DocInfo) == 0);
        UNIT_ASSERT(!desc.GetExtension(PicInfo));
        UNIT_ASSERT(!desc.GetExtension(DocInfo));
        UNIT_ASSERT(!desc.GetExtension(BuildInfo));
        UNIT_ASSERT(desc.GetExtension(BinDocInfo));

        UNIT_ASSERT(desc.RemoveExtension(BinDocInfo) == 0);
        UNIT_ASSERT(!desc.GetExtension(PicInfo));
        UNIT_ASSERT(!desc.GetExtension(DocInfo));
        UNIT_ASSERT(!desc.GetExtension(BuildInfo));
        UNIT_ASSERT(!desc.GetExtension(BinDocInfo));
    }

    Y_UNIT_TEST(ExtensionsAccess) {
        TBuffer buf;
        TDocDescr desc;
        desc.UseBlob(&buf);

        TBuildInfoExt build;
        SetBuild(&build, "test", 100500);
        UNIT_ASSERT_VALUES_EQUAL(desc.AddExtension(BuildInfo, build.GetSize(), &build), 0);

        PicInfoExt picInfo;
        picInfo.ThumbnailId = 1;
        picInfo.NaturalImageSize = 1;
        picInfo.NaturalWidth = 1;
        picInfo.NaturalHeight = 1;
        picInfo.ThumbnailWidth = 1;
        picInfo.ThumbnailHeight = 1;
        picInfo.NaturalImageFormat = 1;
        picInfo.ThumbnailImageFormat = 1;
        picInfo.Urls[0] = '\0';

        UNIT_ASSERT_VALUES_EQUAL(desc.AddExtension(PicInfo, sizeof(PicInfoExt), &picInfo), 0);

        UNIT_ASSERT_VALUES_EQUAL(desc.FindCount(BuildInfo), 1);
        UNIT_ASSERT_VALUES_EQUAL(desc.FindCount("PicInfo"), 1);

        picInfo.ThumbnailId = 2;
        UNIT_ASSERT_VALUES_EQUAL(desc.AddExtension(PicInfo, sizeof(PicInfoExt), &picInfo), 0);
        UNIT_ASSERT_VALUES_EQUAL(desc.FindCount(PicInfo), 2);

        UNIT_ASSERT(((PicInfoExt*) desc.GetExtension(PicInfo, 1))->ThumbnailId == 2);
        UNIT_ASSERT(((PicInfoExt*) desc.GetExtension(PicInfo, 0))->ThumbnailId == 1);

        desc.RemoveExtension(PicInfo);
        UNIT_ASSERT_VALUES_EQUAL(desc.FindCount(PicInfo), 0);
        UNIT_ASSERT_VALUES_EQUAL(desc.FindCount(BuildInfo), 1);
    }

    Y_UNIT_TEST(DocInfos) {
        TDocInfoExtWriter writer;

        writer.Add("info_1", "val_1");
        writer.Add("info_2", "val_2");
        writer.Add("info_3", "val_3");

        TBuffer buf;
        TDocDescr desc;
        desc.UseBlob(&buf);

        writer.Write(desc);

        TDocInfos infos;

        desc.ConfigureDocInfos(infos);
        UNIT_ASSERT(infos.size() == 3);
        UNIT_ASSERT_VALUES_EQUAL(infos["info_1"], "val_1");
        UNIT_ASSERT_VALUES_EQUAL(infos["info_2"], "val_2");
        UNIT_ASSERT_VALUES_EQUAL(infos["info_3"], "val_3");

        infos["extra_1"] = "extra_val";

        TDocInfoExtWriter ew;
        for (TDocInfos::const_iterator toDI = infos.begin(); toDI != infos.end(); ++toDI) {
            ew.Add(toDI->first, toDI->second);
        }

        desc.RemoveExtension(DocInfo);
        infos.clear();
        desc.ConfigureDocInfos(infos);
        UNIT_ASSERT(infos.size() == 0);

        ew.Write(desc);

        infos.clear();
        desc.ConfigureDocInfos(infos);
        UNIT_ASSERT(infos.size() == 4);
        UNIT_ASSERT_VALUES_EQUAL(infos["info_1"], "val_1");
        UNIT_ASSERT_VALUES_EQUAL(infos["info_2"], "val_2");
        UNIT_ASSERT_VALUES_EQUAL(infos["info_3"], "val_3");
        UNIT_ASSERT_VALUES_EQUAL(infos["extra_1"], "extra_val");


        TBuffer   newBlob;
        TDocDescr newDD;
        newDD.UseBlob(&newBlob);
        newDD.CopyExtensions(desc);

        infos.clear();
        newDD.ConfigureDocInfos(infos);
        UNIT_ASSERT(infos.size() == 4);
        UNIT_ASSERT_VALUES_EQUAL(infos["info_1"], "val_1");
        UNIT_ASSERT_VALUES_EQUAL(infos["info_2"], "val_2");
        UNIT_ASSERT_VALUES_EQUAL(infos["info_3"], "val_3");
        UNIT_ASSERT_VALUES_EQUAL(infos["extra_1"], "extra_val");

    }
}
