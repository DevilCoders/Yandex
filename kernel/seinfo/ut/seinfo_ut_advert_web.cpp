#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc67() {
    // ADVERT_WEB
    {
        TInfo info(SE_YANDEX, ST_ADV_WEB);
        KS_TEST_URL("http://an.yandex.ru/count/7WEHpFstNSS40X00gP800Own4X4V1OMGR1UL0fi1RaEt0Yi1\
DOYpe9qG0eca9rMoc0MTgS3k2fQh6Bl6klh3LLQC2ALW0vAWegMUfY2AhgMZHge1fPCkxP6vX8vG28-whHYZ2v-pQaag2fE53Pb\
xGeoGhqkWa21Ehv2lIvIV5IkdXkwejNyypWAam0000501h1Qk-w-GMf-ZLE84iBR0wf83iG6o4Bld35xC2c_gz7iK?test-tag=\
2609",
                    info);
        info.Name = SE_GOOGLE;
        KS_TEST_URL("http://www.googleadservices.com/pagead/aclk?sa=L&ai=Ciwu9Xw_xUenBNO2-yAONx\
4GoB7TQ4OYFxMq_sH_AjbcBEAEgr5T9EigEUOHJpYD______wFghJXshdwdoAHktPbhA8gBAakCPKGELWh_YD6oAwHIA9MEqgR9\
T9BVOCBMiXs4VVRTucFuL17k8uw3Pv8NPVfAQPK4WyhJVz2a8k13Y6cFAD21VSesb4qnsTLN__2EK6y2fHYIBSDS0BCHdIklUTF\
0DEPndbzZTxYQKDnC_GfT1f3t4YcSblYQyp2wit_J_KSzR12OXmliHoX1mYvJZ15UtsSIBgGAB4TLiR4&num=1&cid=5Gij0_lD\
fELWwKpcpuf-kaqp&sig=AOD64_1PhwKQKxKWmqfUFnfas0S5aqLURg&client=ca-pub-0980136772223858&adurl=http:/\
/www.mvideo.ru/price/lvl_8/class_130/%3Freff%3Dgoogle_cpc_notebooks&nm=11&mb=2&bg=!A0SPlfnmbq3SyAIA\
AABdUgAAADgqANc82U7UwJUCY_PkQMz1XF2hSfm53lJU8flDD-bEdk3SkpRyh06ImMS_PF8i7o7qWlrlEOTjVJirthb6Gss3K9L\
E4y8Qj_HLeUB2zHY0adFbLFWe-KoPvvA183kvZkBqKtZTAs34iL3uXyyON6d3FfVCTjft_wtDU_IOKf0EGkDTuMS80E-mzh3z9J\
tZoxvxtpRMfa6jPd3hlvEXUFW97YToZmj_xQYCS1OrwskXoWUfbx6GuDeUuPKq9g-N6fFrDUWlKr4I9Xe8d8ZgkDdcoZvJVdHtV\
oaHQw",
                    info);
        KS_TEST_URL("http://googleads.g.doubleclick.net/aclk?sa=L&ai=CsPAiXw_xUenBNO2-yAONx4GoB\
6aarLkDzrmAwzPAjbcBEAQgr5T9EigEUM60jaD7_____wFghJXshdwdyAEBqQI8oYQtaH9gPqgDAcgD0wSqBH1P0EV1EECJfjhV\
VFO5wW4vXuTy7Dc-_w09V8BA8rhbKElXPZryTXdjpwUAPbVVJ6xviqexMs3__YQrrLZ8dggFINLQEId0iSVRMXQMQ-d1vNlPFhA\
oOcL8Z9PV_e3hhxJ0ai_bnbCK38n8pLNHXY5eaWIehfWZi8lnQjCyz4AHxJHkFA&num=4&sig=AOD64_1zc2ORrLctV4fJw-1G4\
Kef2oruZQ&client=ca-pub-0980136772223858&adurl=http://www.73master.ru/&nm=19&mb=2&bg=!A0SPlfnmbq3Sy\
AIAAABdUgAAAEMqANc82U7UwJUCY_PkQMz1XF2hSfm53lJU8flDD-bEdk3SkpRyh06ImMS_PF8i7o7qWlrlEOTjVJirthb6Gss3\
K9LE4y8Qj_HLeUB2zHY0adFbLFWe-KoPvvA183kvZkBqKtZTAs34iL3uXyyON6d3FfVCTjft_wtDU_IOKfkEi96c8lS3MidwQSY\
caj06gIxF-c2sOFs2AJckMZMNsQfwnYe51IZ1fcgcd9EDfNduwFxJ81PefZT93wv5Ui3qigpktQCBSSC7GVYxBcIuhH3rDjRky6\
BMC6PkMQ",
                    info);
        info.Name = SE_BANNERSBROKER_COM;
        KS_TEST_URL("http://bannersbroker.com/incentives/click_adspot?cId=MjQ4M3wxMzc0NjQxNDky",
                    info);
        info.Name = SE_ADMITAD_COM;
        KS_TEST_URL("http://ad.admitad.com/goto/10af371bcec885d6370626496783af", info);
        info.Name = SE_ADRIVER_RU;
        KS_TEST_URL("http://ad.adriver.ru/cgi-bin/click.cgi?sid=1&bt=2&ad=385733&pid=1072515&bi\
d=2587787&bn=2587787&rnd=77700024",
                    info);
        info.Name = SE_RAMBLER;
        KS_TEST_URL("http://ad.rambler.ru/roff/ban.clk?pg=8842%26bn=233347%26cid=206054%26words\
=BHEX727562726963732B73706F72742B73706C742E7472622E3931302B73706C742E7472622E3535342B73706C742E7472\
622E3932302B73706C742E7472622E3931382B73706C742E7472622E3634362B73706C742E7472622E3634352B73706C742\
E7472622E3932362B73706C742E7472622E3630342B73706C742E7472622E3434382B73706C742E7472622E3931392B7370\
6C742E7472622E3730382B73706C742E7472622E3532382B73706C742E7472622E3930342B73706C742E7472622E3537342\
B73706C742E7472622E393237",
                    info);
        info.Name = SE_YIELDMANAGER_COM;
        KS_TEST_URL("http://ad.yieldmanager.com/iframe3?XxICAA.URgB-KkIBAAAAAM8kTgAAAAAAAgAAAAA\
AAAAAAP8AAAAED8t7bQAAAAAARpQ9AAAAAAAaxWQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\
AFsBgAAAAAAAICAwAAgD8ApCdmvei9wz-kJ2a96L3DP7PLf0iXc9A.s8t.SJdz0D-E4XoUPnfeP4ThehQ-d94.AAAAAAAAAAAAA\
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAnBVJs3aQ4DvyBo-doas2W2QLsS5Prdzfl3kWmAAAAAA==,,http%3A%2F%2Fwbn.regie\
depub.com%2Fcgi-bin%2Fwbnads%2Frender.cgi%3Fcid%3D1496999709%26rid%3D14e0363d61f64f8fb49bd570529247\
98%26template%3Dpopunder%23wbnotifier,B%3D10%26H%3Dhttp%253A%252F%252Fwbn.regiedepub.com%252Fcgi-bi\
n%252Fwbnads%252Frender.cgi%253Fcid%253D1496999709%2526rid%253D14E0363D61F64F8FB49BD57052924798%252\
6template%253Dpopunder%2523wbnotifier%26M%3D1%26Z%3D0x0%26_salt%3D4227332454%26r%3D1%26s%3D4641807%\
26y%3D28,bd8dbc90-f457-11e2-a762-1bce1e6bad89,1374666845404",
                    info);
        info.Name = SE_ADFOX_RU;
        KS_TEST_URL("http://ads.adfox.ru/166267/goLink?p2=escx&p1=bkdwa&p5=btyhq&pr=ehhkxmj@htt\
p://www.flotiliya.com",
                    info);
        info.Name = SE_ADVERTLINK_RU;
        KS_TEST_URL("http://advertlink.ru/vmclick.php?7f91ac7c2aa88772e22669f37b409d4a17826735c\
d53c7162158c3effb33d2a2059cd2935227c668644921aa27949e6324366f574d67b46ae11849ced6e23f93fd61b279e058\
c4454619a1ec62bb15862aae288287d940736078d7a9767fd546a219b9b3b24edf1b327c82b75d1069fdf175d7aa85c5c48\
7be365d2f46589a7a;5b24520693ca8c3d6a66f4889d0c50ac71a5417744bcf6758596e16ab1ba9c54",
                    info);
        info.Name = SE_ADNXS_COM;
        KS_TEST_URL("http://ams1.ib.adnxs.com/click?AAAAAAAAAAAAAAAAAAAAAAAAAAAAAPA_AAAAAAAAAAA\
AAAAAAAAAAHtg-cuHfcQun59AQWUjtlfTEfFRAAAAAIyxEgD6BQAAUAMAAAIAAADe9GsALTwDAAAAAQBVU0QAVVNEAKAAWAIxQQ\
AAqGQAAgUCAQIAAIYAbhU8MwAAAAA./cnd=%21dQb9OQjtq2AQ3umvAxit-AwgAg../referrer=http%3A%2F%2Fbannersbro\
ker.com%2Fbanner.php%3Fbanner%3D9/clickenc=http%3A%2F%2Fwww.onedate.com%2Fprovenienze.php%3FgVar%3D\
prov_1909-MTZ-All-AmateurLonelyHearts%7CredirTo_signup",
                    info);
        info.Name = SE_KAVANGA_RU;
        KS_TEST_URL("http://b.kavanga.ru/click?sid=5592&place=41968&net=4&flight=9465&banner=12\
2605&krqid=1238857698372770462",
                    info);
        info.Name = SE_ADBLENDER_RU;
        KS_TEST_URL("http://bn.adblender.ru/url/100105895736120350/5545a3e8eaa174c5e0522e7db9d4\
2150/",
                    info);
        info.Name = SE_TBN_RU;
        KS_TEST_URL("http://cbn.tbn.ru/sa?nid=205&pid=1921&advId=1251&snid=6940668&queue_advId=\
&adv_w=1&adv_h=1&res_w=1440&res_h=900&rnd=2690311",
                    info);
        info.Name = SE_BEGUN_RU;
        KS_TEST_URL("http://click02.begun.ru/click.jsp?url=mHDcjCcJCAklsZ37cQSFDVYGvu-ZqT6ZtBP1\
XzqhBdJCHWGjcI59UrGztpZWz*WQDQB9B4tDf6D2fHRV2ZAXbj64lnAD1siQVCiYa9nBkm11dzAavUST-VtpRx-EbZ*tTqk1bko\
l5DkEmrJTdqh9GtXbSM7d3E1vaQgTKB*KiWhMva5jS7EtbKRa6EuFyAJZqym-VSifx3Zt9l4svOjgcADJfpUbPdd3jun6bP8qYY\
iziIs8sS2pqRptyxKfByQDFGOaXqkuOmEJuj1V4dq*CONwnbJ-lSQmuHHHUi0Y8av56B1*3uSEePnYSIYb9OHt-kHO9lcDvo6*R\
m83cUm5QCyaW0LBlSDmxdU8tRI6ylLUNZ35JAuMqyWKpfPLYwWfiJWCa-ezR-6Jmr3DhFwABEgkgeep0SB8MDRCrOSeM9y6iWx7\
EmQhKA0Pb8brRtaBYDnV47KSWWurN5DtZ*PfwW*uh5vkey2Fqpjl-KJtahCPqhrCz3Q5vKy990Oyy0Ist7TJkjpix8Kz*ecgqRg\
Jsu4loSPZ5-xlAA8q5yZLz71zBECc6L4rNJUnpITqodBX-7nTpR6E2wv4df-LIYCubhaY6X1Xe8ewDZwC22-INbztgG4dk9m5aQ\
Sh*vfZLsYGjGzwH2mK6aq*kNPWZK0iyJxNLTL3*naavNxyNC2KUg66W*I9x7EvB9TX*6mjHgaeDiawNBn2E0jcGD71DkzU4wQEa\
qzP0ghCsD8Io09XkKZweOCIb1n0Qi*z8OBLNbfTMPVBWkezGfZm9**46XPWFNze69mHN6Rrp*cE8V2PPg0-rPlerWxaH6BoMrx9\
nGnIzuB68DU4tFh*HrD2ow1NeCS3r4gNpEP73uIY*CBJ-amf9rPWR4seqn5Zvzy5F-mG2tlWVBmVjVLeoGrySKWp2GQREc8wJKv\
A-ljf0WqXYw5RlLqlVldIih5*Z8FQMQYge6GeCtPAGu4gYPds9F2-IqqY-uHyfIFt6K0zlK6QJPZJrApatXBXmMjUWhY67ljaWX\
X2ULLr8R4EWoAmrJoxmNuyHfdGrbwKGhg-NJ96BXMR7yt6Rwn7k-bEDX4OK0rnS-pRLOnkaISiwmwqToMfSDHOahvQeEoDLsaI7\
dNHw1TTYpTsb5YnBRlM6PqZhF4Uxkl*1dqKaxQPpqgKvqpYs6g2l*VP6P6sIjPzYVmC-vo86sN-M5DKK*A*RISrodO3nUnswOw4\
jgwvA4AmvHTHIKmocwDF6Tw7fCb58vJuUO-WcADgWFNRBiWfshc0OC3ivdZtfiM4McdmHcnlS6c-Hn*x4pDlG2gApZKXT*j-05e\
gNSElsWdztiBdFoKOUc1Z-l1kl1erxcm8JTUj-G1OVe0qQ9gx3A9vE78CLsL9wFAot5URQaGStJ5EQ298rUyaRVXS-jneAfEaWQ\
KNQ*AqkBZbdbFYtWYGetZrR2CLs-kJxd5RZzi6tGclC2Co*nT47vVXCi-hl*VOe0p93dp-YBPTAHCf8qbb4UwQiILc0wsyDMndh\
57wIZav1U2O6IpgpWBb49AeNnzTizxJG0rSnRnOW*8NvJXvgLBF4x8L4YPDVz-l*e6wbAkOaxpEMU3kwFp2B141bbiMUKiUpBgL\
MXhYLS-SAGSEFlyvfjW9ia1qgwgMM1Kt-GZQYyjBLP*fw2-S-pQfaGeRQKdHFe1wGaMs1gmqwWayNwHpsc68-ta0DDpeFXIEptm\
vEDibF8Z0wx0m7pSLZZTNPU1XOsmES-wSIlOU-TBrGzhuFbG5OEyjNeQWInDbTogEMiClJMkK85NdfpR18bz8B*UKUuT3O2QDV4\
az*FiwXaIZePWebTNdL2pDHkgb4SdUdbJL4gE8rhSD5s9Mhc0e*AF2PVosjlEnmLCzaa8EFze1cYXDx*3bw9E*4YdJpxLf94ntV\
yIsek8JS17CMnG5BjCe0IHnDyO-XJOR*-c6JNiolAJK*LGlMgS34maTdqu9nu*wiH8098W-YHmDs4KkE3I7E-1Ap0HvOSqbVopz\
GyM-fUcYhz8yp2OqkkWQ-mwhXNuF9lJbV0v*0A&eurl%5B%5D=mHDcjBYXFhco0L8fKL50sorKZeDL*weTWuPFzqozhl5QagRE&\
show_time=1374636780901&mouseover_time=1374636783023&mouseover_count=1&mousedown_time=1374636783481\
&click_time=1374636783727&frame_level=",
                    info);
        info.Name = SE_RECREATIV_RU;
        KS_TEST_URL("http://clk.recreativ.ru/go.php?clk=aWQ9MTU0MjMmdGlkPTEwMjQ3NzUmcGM9aWY2OGZ\
2aWxpaWw2Nmw3ZXNwZCZibnVtPXFIVW5XaThOMDMmcm5kPTQ0MTQ5MTk4NCZidj0xMA==",
                    info);
        info.Name = SE_DIRECTADVERT_RU;
        KS_TEST_URL("http://code.directadvert.ru/click/?x=ZGWTy5ax_UWefuoSEo-KVJ7eCZlr_3jdof6_n\
yOzyW7VHAw7U5hUK77DB5-I5ciHRoJajDpzrr6ZWFwVRGbxwHZPkyxSlaU839cGlV-04nynJECkFUt6QVuqHeaCLZ7WnYo-oOCf\
salOEXFZ7NGu1x9uJfTPW_UDtgiO3tR5oobq5hcjvCJROxxvLykCH4fIlKQ_c56hVyX_4qBy9f5rHdh8aWITf4IuhSgfieH2TyK\
CG5z-O9z1-aj471cN-uHQ9BJqEyIVSnw",
                    info);
        info.Name = SE_MEDIALAND_RU;
        KS_TEST_URL("http://engine.rbc.medialand.ru/reference?gid=19&pid=1291&bid=92702&rid=673\
94506",
                    info);
        info.Name = SE_ADWOLF_RU;
        KS_TEST_URL("http://e-01.adwolf.ru/130516/mediaplus/14170/ban240x400_online.html", info);
        info.Name = SE_GOODADVERT_RU;
        KS_TEST_URL("http://engine.goodadvert.ru/r?gid=3&pid=2575&bid=22358&rid=99679565", info);
        info.Name = SE_MAGNA_RU;
        KS_TEST_URL("http://feed.magna.ru/?c=7bda8a72457f049e968b3502daccc401", info);
        info.Name = SE_GAMELEADS_RU;
        KS_TEST_URL("http://gameleads.ru/redirect.php?s=1vl129vf5pj6nbl2bs7d4i2oa0&tt=n&t=-480&\
bc=24&bp=1280x1024",
                    info);
        info.Name = SE_LADYCENTER_RU;
        KS_TEST_URL("http://ladycenter.ru/pnews/1237966/i/4105/pp/3/2/?k=faSMTM3NDY3ODc3NTMxMjQ\
xMDUxMQ%3D%3DfbSMjBlfcSMTQwMTEzZDc2MTA%3DfdSMTQwMTEzZjBhZWQ%3DfeSfgSNDM0fhSMWY2fiSMzlmfjSfkSMWVmflS\
fmSZTI%3DfnSMWY%3DfoSfpSMWY2fqSMjA%3DfrSMw%3D%3DfsSaHR0cDovL3d3dy5jb3Ntby5yd%2495b3VyX2xpZmUveW91X2\
FuZF9oZ%248xMzc1NTA2LzIvftSaHR0cDovL3d3dy5jb3Ntby5yd%2495b3VyX2xpZmUveW91X2FuZF9oZ%248xMzc1NTA2Lw%3\
D%3DfuSaHR0cDovL215Lm1haWwucnUvP2Zyb209ZW1haWw%3DfvSMg%3D%3DfwSNDM0fxSNjIxfySMzlmfaSNjE5fbSMQ%3D%3D\
fcSMQ%3D%3DfdSNTU2feSMzAw url=http://www.ladycenter.ru/tnews/1882369/i/4105/15/p/16/1/?k=faSMTM3NDY\
3ODg4MTM4NDIwNjE1MQ%3D%3DfbSMTg4fcSMTQwMTEzZjE0Njg%3DfdSMTQwMTE0MGExOTM%3DfeSfgSMzNhfhSMTQxfiSMzFif\
jSfkSMTM4flSfmSNjQ%3DfnSNTA%3DfoSfpSMTQxfqSMjA%3DfrSfsSaHR0cDovL2xhZHljZW50ZXIucnUvcG5ld3MvMTIzNzk2\
Ni9pLzQxMDUvcHAvMy8yLz9rPWZhU01UTTNORFkzT0RjM05UTXhNalF4TURVeE1RJTNEJTNEZmJTTWpCbGZjU01UUXdNVEV6WkR\
jMk1UQ%24UzRGZkU01UUXdNVEV6WmpCaFpXU%24UzRGZlU2ZnU05ETTBmaFNNV1kyZmlTTXpsbWZqU2ZrU01XVm1mbFNmbVNaVE\
klM0RmblNNV1klM0Rmb1NmcFNNV1kyZnFTTWpBJTNEZnJTTXclM0QlM0Rmc1Nh%24FIwY0RvdkwzZDNkeTVqYjNOdGJ5NXlkJTI\
0OTViM1Z5WDJ4cFptVXZlVzkxWDJGdVpGOW9aJTI0OHhNemMxTlRBMkx6%24XZmdFNh%24FIwY0RvdkwzZDNkeTVqYjNOdGJ5NX\
lkJTI0OTViM1Z5WDJ4cFptVXZlVzkxWDJGdVpGOW9aJTI0OHhNemMxTlRBMkx3JTNEJTNEZnVTYUh%24MGNEb3ZMMjE1TG0xaGF\
Xd3VjblV2UDJaeWIyMDlaVzFoYVd3JTNEZnZTTWclM0QlM0Rmd1NORE0wZnhTTmpJeGZ5U016bG1mYVNOakU1ZmJTTVElM0QlM0\
RmY1NNU%24UzRCUzRGZkU05UVTJmZVNNekF3ftSaHR0cDovL3d3dy5jb3Ntby5yd%2495b3VyX2xpZmUveW91X2FuZF9oZ%248x\
Mzc1NTA2LzIvfuSbXVpZG49ZDZvRWJ4WXVuQ2VpfvSfwSMzNhfxSOWM1fySMzFifaSOTlhfbSMQ%3D%3DfcSMQ%3D%3DfdSNTU2\
feSMzAw",
                    info);
        info.Name = SE_LUXUP_RU;
        KS_TEST_URL("http://luxup.ru/gg/16/33363/3238195/24529/38664/30030/0/67682/?url=http%3A\
%2F%2Fcl.cpaevent.ru%2F51274962735538376700004b%2F%3Fext_meta_id%3D5904106098215012033",
                    info);
        info.Name = SE_DT00_NET;
        KS_TEST_URL("http://mg.dt00.net/public/informers/torrents.2ru.html?rnd=575180", info);
        info.Name = SE_ADNETWORK_PRO;
        KS_TEST_URL("http://news.adnetwork.pro/click.cgi?h=e86e509da19ee73e363748658eeba136&a=1\
18:542793:169&p=2&url=http%3A%2F%2Fnews.rambler.ru%2F20146118%2F",
                    info);
        info.Name = SE_RMBN_NET;
        KS_TEST_URL("http://post.rmbn.net/cgi-bin/main.fcgi?place_id=885&0.38529828682500944&au\
tolast=4",
                    info);
        info.Name = SE_YADRO_RU;
        KS_TEST_URL("http://sticker.yadro.ru/go?url=http://instaforex.com/ru/%3Fx%3DTWR;id=heXd\
i4eemhjCGid38s6Vrsb7jMd2rswzrOTWTx2HAOZ5AJ2IZOg-ZOv-6mlgGPULcwj",
                    info);
        info.Name = SE_EXOCLICK_COM;
        KS_TEST_URL("http://syndication.exoclick.com/splash.php?cat=2&idsite=147002&idzone=3690\
36&login=zone-on&type=8&http://www.vid2c.com",
                    info);
        info.Name = SE_ADONWEB_RU;
        KS_TEST_URL("http://wu.adonweb.ru/adv_clk_redirect.php?Id=14637&local=1&Type=2", info);
        info.Name = SE_ADCASH_COM;
        KS_TEST_URL("http://www.adcash.com/script/pop_packcpm.php?k=51efb1dc1db58306777.942449&\
h=49c6a8e16df1988796474864e46b40fcca4810f4&id=0&ban=306777&r=145014&ref=h&data=&subid=",
                    info);
        info.Name = SE_CITYADS_RU;
        KS_TEST_URL("http://www.cityads.ru/click-GCQOP56P-SLZKVXTQ", info);
        info.Name = SE_MARKETGID_COM;
        KS_TEST_URL("http://www.marketgid.com/pnews/2365437/i/7738/pp/1/1/?k=faSMTM3NDY2NjIwMTM\
0NTc3MzgzNTE%3DfbSMWM%3DfcSMTQwMTA3ZDk5MDE%3DfdSMTQwMTA3ZDlhYzM%3DfeSfgSMTBifhSfiSfjSfkSflSfmSMTNjf\
nSMWM%3DfoSfpSfqSNDM%3DfrSfsSaHR0cDovL21nLmR0MDAubmV0L21naHRtbC9mcmFtZWh0bWwvbi92L2kvdmlkZW8uYmlnbW\
lyLm5ldC43NzM4Lmh0bWw%3DftSaHR0cDovL2guaG9sZGVyLmNvb%2451Y%249iP3oyNzA0JmIyOTM1NCZrMjY2NTAxMTAxJnMw\
NjIwNQ%3D%3DfuSaHR0cDovL2guaG9sZGVyLmNvb%2451Y%249iP3oyNzA0JmIyOTM1NCZrMjY2NTAxMTAxJnMwNjIwNQ%3D%3D\
fvSMQ%3D%3DfwSMTBifxSfySfaSfbSfcSMQ%3D%3DfdSNTU2feSMzA",
                    info);
        info.Name = SE_RUCLICKS_COM;
        KS_TEST_URL("http://www.ruclicks.com/out/boo0jcie?o=WUNERgpKGhUKVVZaEkYNVktaWg5N", info);
    }
}
