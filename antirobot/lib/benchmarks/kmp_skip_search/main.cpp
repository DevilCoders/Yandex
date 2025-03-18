#include <antirobot/lib/kmp_skip_search.h>

#include <library/cpp/testing/benchmark/bench.h>

#include <util/generic/xrange.h>
#include <library/cpp/deprecated/kmp/kmp.h>

#include <experimental/functional>

using namespace NAntiRobot;

namespace {
    class TKmpSkipSearchUtil {
    public:
        explicit TKmpSkipSearchUtil(TStringBuf searchString)
            : SearchStringLen(searchString.size())
            , Matcher(searchString.begin(), searchString.end())
        {
        }

        inline TStringBuf SearchInText(const TStringBuf& t) const noexcept {
            if (const char* result; Matcher.SubStr(t.begin(), t.end(), result)) {
                return t.SubStr(result - t.data());
            }

            return TStringBuf();
        }

        inline bool InText(const TStringBuf& t) const noexcept {
            const char* result;
            return Matcher.SubStr(t.begin(), t.end(), result);
        }

        inline size_t Length() const noexcept {
            return SearchStringLen;
        }

    private:
        size_t SearchStringLen;
        TKMPMatcher Matcher;
    };

    const char* GetCaptchaCheckMessage() {
        static size_t idx = 0;
        idx++;
        if (idx % 2 == 0) {
            return "<?xml version='1.0'?>\n"
                   "<image_check>ok</image_check>";
        }
        return "<?xml version='1.0'?>\n"
               "<image_check>falied</image_check>";
    }

    const char* GetFromVector(const TVector<const char*>& vector, size_t& idx) {
        const char* res = vector[idx];
        idx++;
        if (idx >= vector.size()) {
            idx = 0;
        }
        return res;
    }

    const char* GetGroupsOnPageString() {
        static const TVector<const char*> vector = {
            "attr=d.mode=deep.groups-on-page=10.docs-in-group=1",
            "attr=d.mode=deep.groups-on-page=10.docs-in-group=1",
            "attr=d.mode=deep.groups-on-page=10.docs-in-group=1",
            "attr=d.mode=deep.groups-on-page=10.docs-in-group=1",
            "attr=d.mode=deep.groups-on-page=10.docs-in-group=1",
            "attr=.mode=flat.groups-on-page=10.docs-in-group=1",
            "attr=d.mode=deep.groups-on-page=10.docs-in-group=1",
            "attr=d.mode=deep.groups-on-page=10.docs-in-group=1",
            "attr=d.mode=deep.groups-on-page=10.docs-in-group=1",
            "attr=d.mode=deep.groups-on-page=150.docs-in-group=1",
            "attr=.mode=flat.groups-on-page=10.docs-in-group=1",
            "attr=d.mode=deep.groups-on-page=10.docs-in-group=1",
            "attr=d.mode=deep.groups-on-page=150.docs-in-group=1",
            "attr=d.mode=deep.groups-on-page=10.docs-in-group=1",
            "attr=.mode=flat.groups-on-page=10.docs-in-group=1",
            "attr=d.mode=deep.groups-on-page=10.docs-in-group=1",
            "attr=d.mode=deep.groups-on-page=10.docs-in-group=1",
            "attr=d.mode=deep.groups-on-page=10.docs-in-group=1",
            "attr=d.mode=deep.groups-on-page=10.docs-in-group=1",
            "attr=d.mode=deep.groups-on-page=10.docs-in-group=1",
            "attr=d.mode=deep.groups-on-page=10.docs-in-group=1",
            "attr=d.mode=deep.groups-on-page=10.docs-in-group=1",
            "attr=d.mode=deep.groups-on-page=150.docs-in-group=1",
            "attr=d.mode=deep.groups-on-page=10.docs-in-group=1",
            "attr=d.mode=deep.groups-on-page=150.docs-in-group=1",
            "attr=d.mode=deep.groups-on-page=10.docs-in-group=1",
            "attr=d.mode=deep.groups-on-page=10.docs-in-group=1",
            "attr=d.mode=deep.groups-on-page=150.docs-in-group=1",
            "attr=d.mode=deep.groups-on-page=10.docs-in-group=1",
            "attr=d.mode=deep.groups-on-page=10.docs-in-group=1",
            "attr=.mode=flat.groups-on-page=10.docs-in-group=1",
            "attr=d.mode=deep.groups-on-page=10.docs-in-group=1",
            "attr=d.mode=deep.groups-on-page=10.docs-in-group=1",
            "attr=d.mode=deep.groups-on-page=10.docs-in-group=1",
            "attr=d.mode=deep.groups-on-page=10.docs-in-group=1",
            "attr=d.mode=deep.groups-on-page=10.docs-in-group=1",
            "attr=d.mode=deep.groups-on-page=150.docs-in-group=1",
            "attr=d.mode=deep.groups-on-page=10.docs-in-group=1",
            "attr=d.mode=deep.groups-on-page=10.docs-in-group=1",
            "attr=.mode=flat.groups-on-page=10.docs-in-group=1",
            "attr=d.mode=deep.groups-on-page=10.docs-in-group=1",
            "attr=d.mode=deep.groups-on-page=10.docs-in-group=1",
            "attr=d.mode=deep.groups-on-page=150.docs-in-group=1",
            "attr=d.mode=deep.groups-on-page=10.docs-in-group=1",
            "attr=d.mode=deep.groups-on-page=10.docs-in-group=1",
            "attr=d.mode=deep.groups-on-page=150.docs-in-group=1",
            "attr=d.mode=deep.groups-on-page=10.docs-in-group=1",
            "attr=.mode=flat.groups-on-page=10.docs-in-group=1",
            "attr=d.mode=deep.groups-on-page=10.docs-in-group=1",
            "attr=d.mode=deep.groups-on-page=10.docs-in-group=1",
            "attr=d.mode=deep.groups-on-page=150.docs-in-group=1",
            "groups-on-page=100",
            "attr=d.mode=deep.groups-on-page=10.docs-in-group=1",
            "attr=d.mode=deep.groups-on-page=150.docs-in-group=1",
            "attr=d.mode=deep.groups-on-page=10.docs-in-group=1",
            "attr=d.mode=deep.groups-on-page=10.docs-in-group=1",
            "attr=d.mode=deep.groups-on-page=10.docs-in-group=1",
            "attr=d.mode=deep.groups-on-page=10.docs-in-group=1",
            "attr=d.mode=deep.groups-on-page=150.docs-in-group=1"};
        static size_t idx = 0;
        return GetFromVector(vector, idx);
    }

    const char* GetShowCaptchaUrl() {
        TVector<const char*> vector = {
            "/showcaptcha?cc=1&retpath=https%3A//translate.yandex.com/ocr%3F_9d11e74d6ddf854f4be2cd355ef3b5ee&t=0/1547587840/1775d8992cbb4d25e643da2e388602f4&s=e2fc2819c189e8a42ec2fda77c3793b7 HTTP/1.1",
            "/showcaptcha?retpath=https%3A//yandex.ru/search%3Ftext%3D%25D1%2582%25D0%25BE%25D0%25B9%25D0%25BE%25D1%2582%25D0%25B0%2520%25D1%2584%25D0%25BE%25D1%2580%25D1%2582%25D1%2583%25D0%25BD%25D0%25B5%25D1%2580%2520%25D0%25BE%25D1%2582%25D0%25B7%25D1%258B%25D0%25B2%25D1%258B%2520%25D0%25B2%25D0%25BB%25D0%25B0%25D0%25B4%25D0%25B5%25D0%25BB%25D1%258C%25D1%2586%25D0%25B5%25D0%25B2%26numdoc%3D50%26lr%3D35_736923e1e51a1ae7078b53cff751daec&t=0/1547587835/247c6329506603135f7362425096f91c&s=2936f074c890daad51676eaf9dd1aae4 HTTP/1.1",
            "/showcaptcha?cc=1&retpath=https%3A//translate.yandex.com/ocr%3F_9d11e74d6ddf854f4be2cd355ef3b5ee&t=0/1547587838/b6d6330f56cc6355646d46d69b357ed8&s=54d2ce50833bbe08a7ef79b9e3b0cd30 HTTP/1.1",
            "/showcaptcha?cc=1&retpath=https%3A//translate.yandex.com/ocr%3F_9d11e74d6ddf854f4be2cd355ef3b5ee&t=0/1547587840/5c4bc2bf90fa7ab6e2775d19f5bd2501&s=1810f8dd57dc6d182bcaf699ac14376b HTTP/1.1",
            "/showcaptcha?cc=1&retpath=https%3A//translate.yandex.com/ocr%3F_9d11e74d6ddf854f4be2cd355ef3b5ee&t=0/1547587842/ea3bb84ab598ca33f1da21fd8b25582e&s=6e63f22ee41e9afbe2447834ba4a8697 HTTP/1.1",
            "/showcaptcha?cc=1&retpath=https%3A//translate.yandex.com/ocr%3F_9d11e74d6ddf854f4be2cd355ef3b5ee&t=0/1547587840/caad0f5f18687421e5dd4959b347e6b0&s=0ef2e2382f7f8f9d71331596732fd3d2 HTTP/1.1",
            "/showcaptcha?cc=1&retpath=https%3A//translate.yandex.com/ocr%3F_9d11e74d6ddf854f4be2cd355ef3b5ee&t=0/1547587842/fddc8a7dec2efaeeae96b76fae720eea&s=78acf0f9b0631e176bca8d4f96329b71 HTTP/1.1",
            "/showcaptcha?cc=1&retpath=https%3A//translate.yandex.com/ocr%3F_9d11e74d6ddf854f4be2cd355ef3b5ee&t=0/1547587845/170e8b6b08324d3c3c558b7aded8758a&s=277edcce38abc6dd16aa9258c81a0679 HTTP/1.1",
            "/showcaptcha?cc=1&retpath=https%3A//www.yandex.ru/search%3Ftext%3D%26text%3D%25D0%25B8%25D0%25B3%25D1%2580%25D1%258B%26clid%3DPRENSR%26mkt%3Dru-ru%26httpsmsn%3D1%26refig%3D539c74c814ac48dca268fc57eecffc15_a62afd668c042b7d1a911f40f5cfc102&t=0/1547587845/c868abe8e8f38b6789da4c839ab3bb73&s=2d850a69275d0534a57f173ec47091c3 HTTP/1.1",
            "/showcaptcha?cc=1&retpath=https%3A//translate.yandex.com/ocr%3F_9d11e74d6ddf854f4be2cd355ef3b5ee&t=0/1547587846/6bd8f5f062b60c911a45135658283989&s=43f35d1f7a64a7b7c2dbe943f2cf293c HTTP/1.1",
            "/showcaptcha?cc=1&retpath=https%3A//www.yandex.ru/search%3Ftext%3D%25D0%25B1%25D0%25B5%25D0%25BB%25D1%258B%25D0%25B9%2B%25D1%2581%25D0%25B2%25D0%25B0%25D0%25B4%25D0%25B5%25D0%25B1%25D0%25BD%25D1%258B%25D0%25B9%2B%25D0%25B1%25D1%2583%25D0%25BA%25D0%25B5%25D1%2582%26p%3D2%26lr%3D172%26numdoc%3D50_32eeaef6fd82a00a1405d457e8d15595&t=0/1547587839/9cc3114d5c57743aede51bdfdb68d836&s=0ca92cc1e597b2f7ce77febbf43a3c58 HTTP/1.1",
            "/showcaptcha?cc=1&retpath=https%3A//translate.yandex.com/ocr%3F_9d11e74d6ddf854f4be2cd355ef3b5ee&t=0/1547587846/ebf6e5b79a662a844b58e8400eae45a5&s=22217ddbe4599c62f422710d02f6017f HTTP/1.1",
            "/showcaptcha?retpath=https%3A//yandex.ru/search%3Ftext%3D%25D0%25B3%2520%25D0%25BC%25D0%25BE%25D1%2581%25D0%25BA%25D0%25B2%25D0%25B0%2520%25D1%2583%25D0%25BB%2520%25D0%25BD%25D0%25B0%25D0%25B3%25D0%25B0%25D1%2582%25D0%25B8%25D0%25BD%25D1%2581%25D0%25BA%25D0%25B0%25D1%258F%2520%25D0%25B4%252027%2520%25D0%25BA%25202%2520%25D0%25B3%25D0%25B8%25D0%25B1%25D0%25B4%25D0%25B4%26numdoc%3D50%26lr%3D35_fd18abde4019c9fe626cc82aa7293954&t=0/1547587847/852bce4f9fe390ac952c8e84f13f4247&s=096a93f57d5b86b2a7896393aa6bea4b HTTP/1.1",
            "/showcaptcha?cc=1&retpath=https%3A//market.yandex.ru/catalog/54971/list%3Fwas_redir%3D1%26hid%3D90575%26glfilter%3D7893318%253A152900%26local-offers-first%3D0%26deliveryincluded%3D0%26onstock%3D1%26page%3D19%26viewtype%3Dlist%26lr%3D213_ce47277becd3849b1945e361654f5ea8&t=0/1547587851/51ac832a8a7424ff447a9931408ce4b3&s=93a1beb6baebcc388b597cf5cde5333b HTTP/1.1",
            "/showcaptcha?cc=1&retpath=https%3A//m.market.yandex.ru/catalog--fotoapparaty/56199/list%3Ftext%3Dsony%2520alpha%2520a6500%26hid%3D91148%26rt%3D9%26was_redir%3D1%26srnum%3D95%26rs%3DeJwzCjLy4_Lh4uV43s0jwCTBoOrSUWYP5L4_zCrACOReL-oBcXtOsAowSASqvnLSdeDi45g74wOrADNQeuHE5fuB0t-BqlkkGFUnndi-P4ARAIBaFzI%252C_30c0a29a38e037d120ac9022642c96a6&t=0/1547587851/1cca184ad173d4c5bbbe02a616f9f12e&s=8b63af58d8243ee5c2c90ed90177f074 HTTP/1.1",
            "/showcaptcha?cc=1&retpath=https%3A//translate.yandex.com/ocr%3F_9d11e74d6ddf854f4be2cd355ef3b5ee&t=0/1547587850/11203b7d0e26f6f01c63f247e5f13c18&s=587fc765959284bcca2a89bec7400940 HTTP/1.1",
            "/showcaptcha?cc=1&retpath=https%3A//www.yandex.com/search%3Ftext%3D119.101.112.2%253A9999_f219e4786fd137b66dfbe4a85d35cd8d&t=0/1547587855/b3422046c28224521873453cd959b2f3&s=7f4bef77b6cac8294c6c72a1ebb3bd59 HTTP/1.0",
            "/showcaptcha?cc=1&retpath=https%3A//translate.yandex.com/ocr%3F_9d11e74d6ddf854f4be2cd355ef3b5ee&t=0/1547587855/87cb3ec8f874c48bdf2e37b85f5487d8&s=eaf97de32800b0262e5342d9af828846 HTTP/1.1",
            "/showcaptcha?cc=1&retpath=https%3A//translate.yandex.com/ocr%3F_9d11e74d6ddf854f4be2cd355ef3b5ee&t=0/1547587854/b7bcabeaddcd47e21c87d70354eacbff&s=c10cc314a1fc103afe8dff37d6742f91 HTTP/1.1",
            "/showcaptcha?cc=1&retpath=https%3A//translate.yandex.com/ocr%3F_9d11e74d6ddf854f4be2cd355ef3b5ee&t=0/1547587857/9fc085916db05597d269f5574239113d&s=ecfeb45b37e6de489bfe8f9f7d361d4f HTTP/1.1",
            "/showcaptcha?cc=1&retpath=https%3A//translate.yandex.com/ocr%3F_9d11e74d6ddf854f4be2cd355ef3b5ee&t=0/1547587856/41f9859491421f276a51ac4305fc2814&s=473a1e8916b95a67a0c8d25f29694ffe HTTP/1.1",
            "/showcaptcha?cc=1&retpath=https%3A//translate.yandex.com/ocr%3F_9d11e74d6ddf854f4be2cd355ef3b5ee&t=0/1547587857/086289465d99ef7f49be0006f5b260ad&s=82b60ca49d1d23e41bbd15f0cda1fb20 HTTP/1.1",
            "/showcaptcha?cc=1&retpath=https%3A//avia.yandex.ru/routes/led/lim/sankt-peterburg-lima%3Ffrom%3Daviawizard_pp%26lang%3Dru%26utm_campaign%3Dcity%26utm_content%3Dtitle%26utm_medium%3Dpp_saform%26utm_source%3Dwizard_ru%26wizardReqId%3D1547311905131188-48641117960501603929025-man1-3566-TCH_7987aaf9cd1c1e913dd5aaf18dce2b25&t=0/1547587858/0d31aa9149d0721ee5e7bcb72a495501&s=ed7326efe4b52df5724190386327cfb8 HTTP/1.1",
            "/showcaptcha?cc=1&retpath=https%3A//translate.yandex.com/ocr%3F_9d11e74d6ddf854f4be2cd355ef3b5ee&t=0/1547587857/e1ef1de68ca5cf8260a7b985422d4368&s=ce929acaa64607a46b21e886c1229c93 HTTP/1.1",
            "/showcaptcha?cc=1&retpath=https%3A//auto.ru/stavropolskiy_kray/cars/nissan/largo/used%3Flisting%3Dlisting_5a77b9a81bbadd0259c373561499ec6c&t=0/1547587860/ec0d4d45912c7266a9db1481ff34f3e3&s=305adf88158dddcac6054a36668ce10b HTTP/1.0",
            "/showcaptcha?cc=1&retpath=https%3A//translate.yandex.com/ocr%3F_9d11e74d6ddf854f4be2cd355ef3b5ee&t=0/1547587859/1f45ceaf04b46593001569fede2dab6f&s=da4f35cd82a824b5617a2a9a2f74e556 HTTP/1.1",
            "/showcaptcha?retpath=https%3A//yandex.ru/search%3Ftext%3D%25D0%25B3%2520%25D0%25BC%25D0%25BE%25D1%2581%25D0%25BA%25D0%25B2%25D0%25B0%2520%25D1%2583%25D0%25BB%2520%25D0%25BD%25D0%25B0%25D0%25B3%25D0%25B0%25D1%2582%25D0%25B8%25D0%25BD%25D1%2581%25D0%25BA%25D0%25B0%25D1%258F%2520%25D0%25B4%252027%2520%25D0%25BA%25202%2520%25D0%25B3%25D0%25B8%25D0%25B1%25D0%25B4%25D0%25B4%26numdoc%3D50%26lr%3D35_fd18abde4019c9fe626cc82aa7293954&t=0/1547587859/3413f8392898a22d3919c64088d92c90&s=b67629708a7f9fc4068919dc41c7a0b8 HTTP/1.1",
            "/showcaptcha?cc=1&retpath=https%3A//www.yandex.com/search%3Ftext%3D203.202.249.97%253A80_dfb2655806a622470c6ab4ed2bcf21b7&t=0/1547587861/2f55ff315ca99c396026658c646ec928&s=8605085ff5a16e4151529d3edbac0804 HTTP/1.0",
            "/showcaptcha?retpath=https%3A//auto.ru/stavropolskiy_kray/cars/nissan/datsun/used%3Flisting%3Dlisting_fc4f1fea8985fb6d1bb20cef0b1f5311&t=0/1547587863/c936de120ade3dc10bccd5914ee37f9d&s=0c8d23996de3b35ed81052738d13a11e HTTP/1.0",
            "/showcaptcha?cc=1&retpath=https%3A//www.yandex.com/search%3Ftext%3D1.20.96.142%253A51637_59d5fb5c16519f85f756edf9dcdd4b95&t=0/1547587864/a19b2da0bf23724a105988eca506941b&s=d63764e18fb597fb6e1e6d8c263e8e76 HTTP/1.0",
            "/showcaptcha?cc=1&retpath=https%3A//translate.yandex.com/ocr%3F_9d11e74d6ddf854f4be2cd355ef3b5ee&t=0/1547587863/4b4ef5977d1600c6e289cc901f864f99&s=1e404a2bdbf55727d2f52b5bc39101a3 HTTP/1.1",
            "/showcaptcha?cc=1&retpath=https%3A//translate.yandex.com/ocr%3F_9d11e74d6ddf854f4be2cd355ef3b5ee&t=0/1547587864/1d0df4bbe459b2a2a7e2e2484a092183&s=14af9b2e21c13660f0c75deb6511a0aa HTTP/1.1",
            "/showcaptcha?cc=1&retpath=https%3A//yandex.ru/yandsearch%3Flr%3D213%26text%3DZ%252052%2520%2522Login%2520Form%2522%2520%2522Remember%2520Me%2522%2520%2522Log%2520in%2522%2520%2522Forgot%2520your%2520password%2522%2520%2522Forgot%2520your%2520username%2522%2520%2522Create%2520an%2520account%2522_14ee68bfaa5951d4a3d6bd22199fed5a&t=0/1547587700/f8d20b1e9c423983d53602170d7d6e41&s=5c7aab7f4d5a09b98ec59670001b0d59 HTTP/1.1",
            "/showcaptcha?cc=1&retpath=https%3A//translate.yandex.com/ocr%3F_9d11e74d6ddf854f4be2cd355ef3b5ee&t=0/1547587866/94cb0d712cf5393c1f5f513c4ff372dc&s=05fc92fc4c017efb3e74eb7bf1f1a3bb HTTP/1.1",
            "/showcaptcha?cc=1&retpath=https%3A//translate.yandex.com/ocr%3F_9d11e74d6ddf854f4be2cd355ef3b5ee&t=0/1547587864/59af17d2f25643142c5bea86bf6b0640&s=1edaffdfadef5e7c75b1409ede519ea0 HTTP/1.1",
            "/showcaptcha?cc=1&retpath=https%3A//translate.yandex.com/ocr%3F_9d11e74d6ddf854f4be2cd355ef3b5ee&t=0/1547587869/3bd5037e4f40254fdfa20d639a15fef1&s=bf2a97659f2ce139c86e1349420f359a HTTP/1.1",
            "/showcaptcha?cc=1&retpath=https%3A//translate.yandex.com/ocr%3F_9d11e74d6ddf854f4be2cd355ef3b5ee&t=0/1547587873/28908ae7854630fd94c1b5dc160972a3&s=b684c3fda4c8916ab67eee95fbc053c1 HTTP/1.1",
            "/showcaptcha?cc=1&retpath=https%3A//market.yandex.ru/catalog/54910/list%3Fwas_redir%3D1%26hid%3D1005910%26rt%3D2%26glfilter%3D7893318%253A152831%26local-offers-first%3D0%26deliveryincluded%3D0%26onstock%3D1%26page%3D25%26viewtype%3Dlist%26lr%3D213_c87c706dae2e46aa39fe04bf55398187&t=0/1547587879/452721177c91c4e13888e12ddb101370&s=97a57313947c2695ad7b157c4d57d717 HTTP/1.1",
            "/showcaptcha?retpath=https%3A//yandex.ru/search%3Flr%3D970%26msid%3D1547587825.96946.122082.225968%26text%3D%25D0%25BA%25D1%2583%25D0%25BF%25D0%25B8%25D1%2582%25D1%258C%2B%25D0%25B1%25D1%258B%25D1%2582%25D0%25BE%25D0%25B2%25D0%25BA%25D1%2583%2B%25D0%25B2%2B%25D1%2580%25D0%25B0%25D0%25BC%25D0%25B5%25D0%25BD%25D1%2581%25D0%25BA%25D0%25BE%25D0%25BC%26suggest_reqid%3D736332257154758782578339214225043_948e3e77b19ec6302a0cfc3f942541fc&status=failed&t=0/1547587833/f450ed67c3a7cccd7c9607eb38b7b1cf&s=9e38c27533358072572482e0db2d9bb3 HTTP/1.1",
            "/showcaptcha?cc=1&retpath=https%3A//translate.yandex.com/ocr%3F_9d11e74d6ddf854f4be2cd355ef3b5ee&t=0/1547587878/1ed80ec6861f204ec22174e698cc4a9a&s=68bd91963c4f5a6c8f250a5009bb962d HTTP/1.1",
            "/showcaptcha?retpath=https%3A//auto.ru/stavropolskiy_kray/cars/nissan/datsun/used%3Flisting%3Dlisting_fc4f1fea8985fb6d1bb20cef0b1f5311&t=0/1547587880/ce0c11ce2af1e9301f3cc1af3172583f&s=bc195485d0d2f0182893b9ea977cabba HTTP/1.0",
            "/showcaptcha?cc=1&retpath=https%3A//yandex.ru/images/search%3Ftext%3DCommercial%2520hard%2520money%2520lenders%2520massachusetts%2520bay_4e19ec429fb8805033a603e897a338e0&t=0/1547587877/bb08b83796f8bc4847faf45106878b9f&s=011ff9325b66be1de42523dee8b4d4e7 HTTP/1.0",
            "/showcaptcha?cc=1&retpath=https%3A//translate.yandex.com/ocr%3F_9d11e74d6ddf854f4be2cd355ef3b5ee&t=0/1547587879/692d6eb03ac55a7b0a5f5d50e9f6e37b&s=c7f584773e58a514d10e811aa4992fd3 HTTP/1.1",
            "/showcaptcha?cc=1&retpath=https%3A//translate.yandex.com/ocr%3F_9d11e74d6ddf854f4be2cd355ef3b5ee&t=0/1547587879/63f61c3618119b7ff961702d7cbdbf25&s=9b0bc4e50c723a382494a56df590cdd9 HTTP/1.1",
            "/showcaptcha?cc=1&retpath=https%3A//yandex.ru/maps/api/search%3FcsrfToken%3D_a5509afe4f5d4dc29230374f6bdb92b3&t=0/1547587883/6c13c03f0193d4f67381d46228969ab9&s=be7cfb85bc80ede27f9b5af57a9f92d5 HTTP/1.1",
            "/showcaptcha?retpath=https%3A//yandex.ru/maps/api/search%3FcsrfToken%3D%26text%3D%25D0%259A%25D1%2580%25D0%25B0%25D1%2581%25D0%25BD%25D0%25BE%25D0%25B4%25D0%25B0%25D1%2580%25D1%2581%25D0%25BA%25D0%25B8%25D0%25B9%2B%25D0%25BA%25D1%2580%25D0%25B0%25D0%25B9%2B%25D0%2592%25D0%25B8%25D1%2582%25D1%258F%25D0%25B7%25D0%25B5%25D0%25B2%25D0%25BE%2B%25D0%259E%25D1%2580%25D1%2583%25D0%25B6%25D0%25B8%25D0%25B5%2B%25D0%25B8%2B%25D1%2581%25D1%2580%25D0%25B5%25D0%25B4%25D1%2581%25D1%2582%25D0%25B2%25D0%25B0%2B%25D1%2581%25D0%25B0%25D0%25BC%25D0%25BE%25D0%25B7%25D0%25B0%25D1%2589%25D0%25B8%25D1%2582%25D1%258B%26ll%3D37.751000%252C55.684018%26lang%3Dru_RU%26results%3D400%26spn%3D1%252C1%26z%3D11%26origin%3Dmaps-location_72962f3dd14ec092cffa1fb0e5091b90&t=0/1547587883/762e7daa80cd993f6dfda2cbf1c3f927&s=e60a0ee145718f54c0c9da4e9fe712ee HTTP/1.1",
            "/showcaptcha?cc=1&retpath=http%3A//yandex.ua/yandsearch%3Ftext%3Dsite%253Aintoaction.com%26lr%3D143%26redircnt%3D1379982784.1%26ncrnd%3D3905_a48456567af31be857be92e4b5963555&t=0/1547587884/dc130d6b830f5a897560b7945cc2c181&s=b9605ebc9cbc79ac6cc045614c56fb73 HTTP/1.1",
            "/showcaptcha?cc=1&retpath=https%3A//translate.yandex.com/ocr%3F_9d11e74d6ddf854f4be2cd355ef3b5ee&t=0/1547587875/f5948c53d160eaf647c4368696a77b22&s=bc7774a609106d62b78664f4bf0074cb HTTP/1.1",
            "/showcaptcha?cc=1&retpath=https%3A//translate.yandex.com/ocr%3F_9d11e74d6ddf854f4be2cd355ef3b5ee&t=0/1547587884/62099c56e2ce9e7cb44c714ca3b495f1&s=6715e859431713f368a4822084bd4389 HTTP/1.1",
            "/showcaptcha?cc=1&retpath=https%3A//translate.yandex.com/ocr%3F_9d11e74d6ddf854f4be2cd355ef3b5ee&t=0/1547587884/0ac772b8061d496727eba6cdf386c90a&s=23d60e5649865db2763545c80cf05f4b HTTP/1.1",
            "/showcaptcha?cc=1&retpath=https%3A//avia.yandex.ru/routes/ist/asb/stambul-ashkhabad%3Ffrom%3Daviawizard_pp%26lang%3Dru%26utm_campaign%3Dcity%26utm_content%3Dtitle%26utm_medium%3Dpp_saform%26utm_source%3Dwizard_tm%26wizardReqId%3D1547312287580547-850025611834253832190224-sas2-9041-TCH_fd8f9864780bdc2cd6ea6597aa3ef90e&t=0/1547587885/cc612ac7068bbdf9af212371b717716d&s=ab981119ab39c7cacfcece8456ac1fce HTTP/1.1",
            "/showcaptcha?retpath=https%3A//auto.ru/stavropolskiy_kray/cars/nissan/largo/used%3Flisting%3Dlisting_5a77b9a81bbadd0259c373561499ec6c&t=0/1547587887/468c702a2069b823fec2d05b2cdb4a3b&s=f95cde2a35f38a11a706f666ac2ca563 HTTP/1.0",
            "/showcaptcha?retpath=https%3A//auto.ru/stavropolskiy_kray/cars/nissan/datsun/used%3Flisting%3Dlisting_fc4f1fea8985fb6d1bb20cef0b1f5311&t=0/1547587888/212da186258e6da03a8f0fabebf094b2&s=235ef4236065668d94d5455bb22b03ef HTTP/1.0",
            "/showcaptcha?retpath=https%3A//yandex.ru/search%3Ftext%3D%25D0%25B0%25D0%25BA%25D1%2582%25D1%2583%25D0%25B0%25D0%25BB%25D1%258C%25D0%25BD%25D1%258B%25D0%25B5%2520%25D0%25BF%25D0%25B4%25D0%25B4%25202018%26numdoc%3D50%26lr%3D35_55fdb299c6027c1c62d75efc129876fa&t=0/1547587884/77b0ca6b355472b9463b6dca053a7021&s=c9a0b2fef05206217df15414fc68b4c6 HTTP/1.1",
            "/showcaptcha?retpath=http%3A//yandex.ru/search%3Ftext%3Dtanki%2520online%2520com%2520%D0%B8%D0%B3%D1%80%D0%B0%D1%82%D1%8C%26clid%3D2334558_6ae102fd38be0c66625074678364ad07&t=0/1547587889/2aeaeeb8b560fe234a6998ce3853fd2d&s=2ce362ead3ec7e5b8c5531fe4401ab48 HTTP/1.1",
            "/showcaptcha?cc=1&retpath=https%3A//translate.yandex.com/ocr%3F_9d11e74d6ddf854f4be2cd355ef3b5ee&t=0/1547587889/a004dd6dc3c82d12449f2d54a3c2a651&s=c682a8405c3f34fa98d074c0b6bb7c13 HTTP/1.1",
            "/showcaptcha?cc=1&retpath=https%3A//translate.yandex.com/ocr%3F_9d11e74d6ddf854f4be2cd355ef3b5ee&t=0/1547587891/8e1d3ce6e2f2ae344c54ff5310d4b447&s=69b4760d8e256e940299513ec40a680f HTTP/1.1",
        };
        static size_t idx = 0;
        return GetFromVector(vector, idx);
    }

    const char* GetYandexHost() {
        TVector<const char*> vector = {
            "collections.yandex.ua",
            "yandex.ru",
            "yandex.ru",
            "yandex.ru",
            "yandex.ru",
            "yandex.ru",
            "yandex.ru",
            "yandex.ru",
            "yandex.ru",
            "yandex.ru",
            "clck.yandex.ru",
            "www.yandex.ru",
            "yandex.ru",
            "yandex.ru",
            "yandex.ru",
            "yandex.ru",
            "yandex.ru",
            "yandex.ru",
            "yandex.ru",
            "yandex.ru",
            "yandex.ru",
            "yandex.ru",
            "yandex.ru",
            "yandex.ru",
            "yandex.ru",
            "yandex.ru",
            "yandex.ru",
            "yandex.ru",
            "yandex.ru",
            "yandex.ru",
            "yandex.ua",
            "yandex.ru",
            "antirobot",
            "yandex.ru",
            "yandex.ru",
            "yandex.ru",
            "api.sport.news.yandex.ru",
            "yandex.ru",
            "yandex.ru",
            "yandex.ru",
            "yandex.ru",
            "yandex.ru",
            "yandex.ru",
            "yandex.ru",
            "yandex.ru",
            "yandex.ru",
            "yandex.ru",
            "www.yandex.ru",
            "api.sport.news.yandex.ru",
            "yandex.ru"};
        static size_t idx = 0;
        return GetFromVector(vector, idx);
    }
}

#define BENCHMARK_BYCICLE(needle, func)                       \
    Y_CPU_BENCHMARK(func##_BycicleKMP, iface) {               \
        TKmpSkipSearch kmpSearch(needle);                     \
        for (const auto i : xrange(iface.Iterations())) {     \
            Y_UNUSED(i);                                      \
            Y_DO_NOT_OPTIMIZE_AWAY(kmpSearch.InText(func())); \
        }                                                     \
    }

#define BENCHMARK_UTIL(needle, func)                          \
    Y_CPU_BENCHMARK(func##_UtilKMP, iface) {                  \
        TKmpSkipSearchUtil kmpSearch(needle);                 \
        for (const auto i : xrange(iface.Iterations())) {     \
            Y_UNUSED(i);                                      \
            Y_DO_NOT_OPTIMIZE_AWAY(kmpSearch.InText(func())); \
        }                                                     \
    }

#define BENCHMARK_CONTAINS(needle, func)                                       \
    Y_CPU_BENCHMARK(func##_TStringBufContains, iface) {                        \
        const TStringBuf searchString = needle;                                \
        for (const auto i : xrange(iface.Iterations())) {                      \
            Y_UNUSED(i);                                                       \
            Y_DO_NOT_OPTIMIZE_AWAY(TStringBuf(func()).Contains(searchString)); \
        }                                                                      \
    }

#define BENCHMARK_STD_SEARCH(needle, func)                                                                                                  \
    Y_CPU_BENCHMARK(func##_std_search, iface) {                                                                                             \
        const TStringBuf searchString = needle;                                                                                             \
        for (const auto i : xrange(iface.Iterations())) {                                                                                   \
            Y_UNUSED(i);                                                                                                                    \
            const TStringBuf message = func();                                                                                              \
            Y_DO_NOT_OPTIMIZE_AWAY(std::search(message.begin(), message.end(), searchString.begin(), searchString.end()) != message.end()); \
        }                                                                                                                                   \
    }

#define BENCHMARK_STD_SEARCH_BOYER_MOORE(needle, func)                                                           \
    Y_CPU_BENCHMARK(func##_std_search_boyer_moore, iface) {                                                      \
        const TStringBuf searchString = needle;                                                                  \
        const auto searcher = std::experimental::boyer_moore_searcher(searchString.begin(), searchString.end()); \
        for (const auto i : xrange(iface.Iterations())) {                                                        \
            Y_UNUSED(i);                                                                                         \
            const TStringBuf message = func();                                                                   \
            Y_DO_NOT_OPTIMIZE_AWAY(std::search(message.begin(), message.end(), searcher) != message.end());      \
        }                                                                                                        \
    }

#define BENCHMARK_STD_SEARCH_BOYER_MOORE_HORSPOOL(needle, func)                                                           \
    Y_CPU_BENCHMARK(func##_std_search_boyer_moore_horspool, iface) {                                                      \
        const TStringBuf searchString = needle;                                                                           \
        const auto searcher = std::experimental::boyer_moore_horspool_searcher(searchString.begin(), searchString.end()); \
        for (const auto i : xrange(iface.Iterations())) {                                                                 \
            Y_UNUSED(i);                                                                                                  \
            const TStringBuf message = func();                                                                            \
            Y_DO_NOT_OPTIMIZE_AWAY(std::search(message.begin(), message.end(), searcher) != message.end());               \
        }                                                                                                                 \
    }

BENCHMARK_BYCICLE("<image_check>ok</image_check>", GetCaptchaCheckMessage)
BENCHMARK_UTIL("<image_check>ok</image_check>", GetCaptchaCheckMessage)
BENCHMARK_CONTAINS("<image_check>ok</image_check>", GetCaptchaCheckMessage)
BENCHMARK_STD_SEARCH("<image_check>ok</image_check>", GetCaptchaCheckMessage)
BENCHMARK_STD_SEARCH_BOYER_MOORE("<image_check>ok</image_check>", GetCaptchaCheckMessage)
BENCHMARK_STD_SEARCH_BOYER_MOORE_HORSPOOL("<image_check>ok</image_check>", GetCaptchaCheckMessage)

BENCHMARK_BYCICLE("groups-on-page=", GetGroupsOnPageString)
BENCHMARK_UTIL("groups-on-page=", GetGroupsOnPageString)
BENCHMARK_CONTAINS("groups-on-page=", GetGroupsOnPageString)
BENCHMARK_STD_SEARCH("groups-on-page=", GetGroupsOnPageString)
BENCHMARK_STD_SEARCH_BOYER_MOORE("groups-on-page=", GetGroupsOnPageString)
BENCHMARK_STD_SEARCH_BOYER_MOORE_HORSPOOL("groups-on-page=", GetGroupsOnPageString)

BENCHMARK_BYCICLE("&s=", GetShowCaptchaUrl)
BENCHMARK_UTIL("&s=", GetShowCaptchaUrl)
BENCHMARK_CONTAINS("&s=", GetShowCaptchaUrl)
BENCHMARK_STD_SEARCH("&s=", GetShowCaptchaUrl)
BENCHMARK_STD_SEARCH_BOYER_MOORE("&s=", GetShowCaptchaUrl)
BENCHMARK_STD_SEARCH_BOYER_MOORE_HORSPOOL("&s=", GetShowCaptchaUrl)

BENCHMARK_BYCICLE("xmlsearch.", GetYandexHost)
BENCHMARK_UTIL("xmlsearch.", GetYandexHost)
BENCHMARK_CONTAINS("xmlsearch.", GetYandexHost)
BENCHMARK_STD_SEARCH("xmlsearch.", GetYandexHost)
BENCHMARK_STD_SEARCH_BOYER_MOORE("xmlsearch.", GetYandexHost)
BENCHMARK_STD_SEARCH_BOYER_MOORE_HORSPOOL("xmlsearch.", GetYandexHost)
