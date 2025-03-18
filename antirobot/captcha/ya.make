LIBRARY()

OWNER(
    g:antirobot
)

PEERDIR(
    library/cpp/archive
    library/cpp/http/cookies
)

SET(cpp_inc_all ${BINDIR}/captcha_runtime_variables.inc)

SET(archiver_out captcha.inc)

SRCS(
    ${CURDIR}/localized_data.cpp
    ${BINDIR}/${archiver_out}
)

# Localized captcha stuff
#
SET(template_dir ${CURDIR})

SET(template_dir_out ${BINDIR}/captcha_lang)

SET(
    xml_templates
    ${template_dir}/captcha_partner.xml
    ${template_dir}/blocked_partner.xml
)

SET(
    json_templates
    ${template_dir}/captcha_json.txt
    ${template_dir}/captcha_json_success.txt
    ${template_dir}/blocked_mjs_json.txt
)

SET(cpp_template cpp.tpl)

SET(cpp_inc_dir_out ${BINDIR}/captcha_cpp)

SET(
    template_srcs
    bad_request.html
    blocked.html
    many_requests.html

    # --- 1 ---
    smart_generated/versions/1/ru/checkbox/index.html
    smart_generated/versions/1/ru/advanced/index.html
    smart_generated/versions/1/com/checkbox/index.html
    smart_generated/versions/1/com/advanced/index.html
    smart_generated/versions/1/kz/checkbox/index.html
    smart_generated/versions/1/kz/advanced/index.html
    smart_generated/versions/1/tr/checkbox/index.html
    smart_generated/versions/1/tr/advanced/index.html
    smart_generated/versions/1/uz/checkbox/index.html
    smart_generated/versions/1/uz/advanced/index.html
    # --- /1 ---

    # --- 2 ---
    smart_generated/versions/2/com/advanced/index.html
    smart_generated/versions/2/com/checkbox/index.html
    smart_generated/versions/2/kz/advanced/index.html
    smart_generated/versions/2/kz/checkbox/index.html
    smart_generated/versions/2/ru/advanced/index.html
    smart_generated/versions/2/ru/checkbox/index.html
    smart_generated/versions/2/tr/advanced/index.html
    smart_generated/versions/2/tr/checkbox/index.html
    smart_generated/versions/2/uz/advanced/index.html
    smart_generated/versions/2/uz/checkbox/index.html
    # --- /2 ---
)

SET(
    antirobot_rendered_versioned_files

    # --- 1 ---
    ${template_dir_out}/1-captcha_advanced.html.com
    ${template_dir_out}/1-captcha_advanced.html.kz
    ${template_dir_out}/1-captcha_advanced.html.ru
    ${template_dir_out}/1-captcha_advanced.html.uz
    ${template_dir_out}/1-captcha_advanced.html.tr
    ${template_dir_out}/1-captcha_checkbox.html.com
    ${template_dir_out}/1-captcha_checkbox.html.kz
    ${template_dir_out}/1-captcha_checkbox.html.ru
    ${template_dir_out}/1-captcha_checkbox.html.uz
    ${template_dir_out}/1-captcha_checkbox.html.tr
    # --- /1 ---

    # --- 2 ---
    ${template_dir_out}/2-captcha_advanced.html.com
    ${template_dir_out}/2-captcha_checkbox.html.com
    ${template_dir_out}/2-captcha_advanced.html.kz
    ${template_dir_out}/2-captcha_checkbox.html.kz
    ${template_dir_out}/2-captcha_advanced.html.ru
    ${template_dir_out}/2-captcha_checkbox.html.ru
    ${template_dir_out}/2-captcha_advanced.html.tr
    ${template_dir_out}/2-captcha_checkbox.html.tr
    ${template_dir_out}/2-captcha_advanced.html.uz
    ${template_dir_out}/2-captcha_checkbox.html.uz
    # --- /2 ---
)

SET(
    rendered_static_htmls

    ${template_dir_out}/blocked.html.com
    ${template_dir_out}/blocked.html.kz
    ${template_dir_out}/blocked.html.ru
    ${template_dir_out}/blocked.html.tr
    ${template_dir_out}/blocked.html.uz

    ${template_dir_out}/many_requests.html.ru

    ${template_dir_out}/bad_request.html.com
    ${template_dir_out}/bad_request.html.kz
    ${template_dir_out}/bad_request.html.ru
    ${template_dir_out}/bad_request.html.tr
    ${template_dir_out}/bad_request.html.uz
)

SET(
    rendered_htmls
    ${rendered_static_htmls}
    ${antirobot_rendered_versioned_files}
)

SET(
    rendered_cpp_incs
    ${cpp_inc_dir_out}/com.cpp.inc
    ${cpp_inc_dir_out}/ru.cpp.inc
    ${cpp_inc_dir_out}/tr.cpp.inc
    ${cpp_inc_dir_out}/uz.cpp.inc
    ${cpp_inc_dir_out}/kz.cpp.inc
)

SET(
    static_files
    ${CURDIR}/generated/captcha.min.css
    ${CURDIR}/generated/captcha.ie.min.css

    ${CURDIR}/tov_major/app/build/tmgrdfrend.js

    ${CURDIR}/demo.html
    ${CURDIR}/themer_generated/themer.html
)

SET(
    langs
    com
    kz
    ru
    tr
    uz
)

SET(
    lang_files
    lang/basic
    lang/com
    lang/tr
    lang/kz
    lang/ru
    lang/uz
)

RUN_PROGRAM(
    antirobot/captcha/render_htmls --template-files ${template_srcs} --out-dir ${template_dir_out} --templ-dir ${template_dir} --lang-srcs ${langs}
    IN ${template_srcs} ${lang_files}
    OUT_NOAUTO ${rendered_htmls}
)

SET(
    antirobot_versioned_files

    # --- 1 ---
    ${CURDIR}/smart_generated/versions/1/captcha_smart.min.css
    ${CURDIR}/smart_generated/versions/1/captcha_smart.min.js
    ${CURDIR}/smart_generated/versions/1/captcha_smart_error.min.js
    # --- /1 ---

    # --- 2 ---
    ${CURDIR}/smart_generated/versions/2/./captcha_smart_error.5205103d27eb76a58bbb.min.js
    ${CURDIR}/smart_generated/versions/2/./captcha_smart.5205103d27eb76a58bbb.min.css
    ${CURDIR}/smart_generated/versions/2/./captcha_smart.5205103d27eb76a58bbb.min.js
    # --- /2 ---

    ${antirobot_rendered_versioned_files}
)

SET(
    external_versioned_files

    # --- themer ---
    ${CURDIR}/themer_generated/2a4093c5aee45739eea2.svg
    ${CURDIR}/themer_generated/3b10b20b3414f7ee747745e5b2e06fac.png
    ${CURDIR}/themer_generated/50e8fbfd057fa03d3ef5.svg
    ${CURDIR}/themer_generated/7f327141bd12eba7257680ca11a317c1.png
    ${CURDIR}/themer_generated/9b1d380eb417fac7f311.svg
    ${CURDIR}/themer_generated/a428f3364aa6c3df525c.svg
    ${CURDIR}/themer_generated/a86397b0a081280d0bdd5d4184ca18a4.png
    ${CURDIR}/themer_generated/a8cb16806ad251e32c45.svg
    ${CURDIR}/themer_generated/b24eae07bc84e10fb7e7.svg
    ${CURDIR}/themer_generated/b3a0de57b72052192997accbb5d16aa8.png
    ${CURDIR}/themer_generated/b98ddbe033d4fe825e9e31d1f0e9afef.png
    ${CURDIR}/themer_generated/ba4e52896b5fa1b82c2cff973a25858a.png
    ${CURDIR}/themer_generated/c6a6fb88ef580d8d3177.svg
    ${CURDIR}/themer_generated/df799553fae5f58720b377755fd1272c.png
    ${CURDIR}/themer_generated/f12a4edb3daf72d3fd8f210d8044ab6c.png
    ${CURDIR}/themer_generated/f74abe21e7314e6850087e9af555dbe7.png
    ${CURDIR}/themer_generated/main.js
    ${CURDIR}/themer_generated/main.3a152f092b05507648f5.js
    # --- /themer ---

    # --- 26 ---
    ${CURDIR}/external_generated/versions/26/advanced.31ef01925ffcb98e6aa6.html
    ${CURDIR}/external_generated/versions/26/advanced.a2fd8162aaea3b9e0caf.js
    ${CURDIR}/external_generated/versions/26/checkbox.053351f54462169fc5ed.html
    ${CURDIR}/external_generated/versions/26/checkbox.cf9aed3f5ef38ff9c7b8.js
    ${CURDIR}/external_generated/versions/26/26-captcha.js
    # --- /26 ---

    # --- 27 ---
    ${CURDIR}/external_generated/versions/27/advanced.f8773b9aee55fe9205d8.html
    ${CURDIR}/external_generated/versions/27/advanced.f8773b9aee55fe9205d8.js
    ${CURDIR}/external_generated/versions/27/checkbox.f8773b9aee55fe9205d8.html
    ${CURDIR}/external_generated/versions/27/checkbox.f8773b9aee55fe9205d8.js
    ${CURDIR}/external_generated/versions/27/27-captcha.js
    # --- /27 ---

    # --- 28 ---
    ${CURDIR}/external_generated/versions/28/advanced.3787347d7e73373e227e.html
    ${CURDIR}/external_generated/versions/28/advanced.3787347d7e73373e227e.js
    ${CURDIR}/external_generated/versions/28/checkbox.3787347d7e73373e227e.html
    ${CURDIR}/external_generated/versions/28/checkbox.3787347d7e73373e227e.js
    ${CURDIR}/external_generated/versions/28/28-captcha.js
    # --- /28 ---    

    # --- 29 ---
    ${CURDIR}/external_generated/versions/29/advanced.a052935cde174f1c0f06.html
    ${CURDIR}/external_generated/versions/29/advanced.a052935cde174f1c0f06.js
    ${CURDIR}/external_generated/versions/29/checkbox.a052935cde174f1c0f06.html
    ${CURDIR}/external_generated/versions/29/checkbox.a052935cde174f1c0f06.js
    ${CURDIR}/external_generated/versions/29/29-captcha.js
    # --- /29 ---

    # Здесь должны сохраняться ресурсы текущей и предыдущей версий
    # Посколько выкатка происходит не единомоментно, некоторые клиенты могут смотреть на старые ссылки, поэтому важно, чтобы они работали
)

# cpp .inc for each version
RUN_PROGRAM(
    antirobot/captcha/render_external_static_cpp_incs external ${BINDIR}/external_static_versions.inc ${BINDIR}/external_static_version_list.inc ${external_versioned_files}
    IN ${external_versioned_files}
    OUT ${BINDIR}/external_static_versions.inc ${BINDIR}/external_static_version_list.inc
)
RUN_PROGRAM(
    antirobot/captcha/render_external_static_cpp_incs antirobot ${BINDIR}/antirobot_static_versions.inc ${BINDIR}/antirobot_static_version_list.inc ${antirobot_versioned_files}
    IN ${antirobot_versioned_files}
    OUT ${BINDIR}/antirobot_static_versions.inc ${BINDIR}/antirobot_static_version_list.inc
)

ARCHIVE(
    NAME ${archiver_out}
    ${xml_templates}
    ${json_templates}
    ${static_files}
    ${antirobot_versioned_files}
    ${external_versioned_files}
    ${rendered_static_htmls}
)

# cpp inc for each language
RUN_PROGRAM(
    antirobot/captcha/render_cpp_incs ${cpp_inc_dir_out} ${template_dir} ${langs}
    IN ${cpp_template} ${lang_files}
    OUT ${rendered_cpp_incs}
)

PYTHON(
    ${ARCADIA_ROOT}/build/scripts/cat.py ${rendered_cpp_incs}
    IN ${rendered_cpp_incs}
    STDOUT ${cpp_inc_all}
)

END()

RECURSE(
    captcha_test_service
    ext_captcha_demo
    fingerprint
    greed_example
    render_external_static_cpp_incs
)
