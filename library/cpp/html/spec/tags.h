#pragma once

#include <util/system/defaults.h>
#include <util/generic/strbuf.h>

enum TIrregTag {
    IRREG_none = 0,
    IRREG_B = 1 << 0,
    IRREG_BIG = 1 << 1,
    IRREG_FONT = 1 << 2,
    IRREG_I = 1 << 3,
    IRREG_S = 1 << 4,
    IRREG_SMALL = 1 << 5,
    IRREG_STRIKE = 1 << 6,
    IRREG_TT = 1 << 7,
    IRREG_U = 1 << 8,
    IRREG_SUB = 1 << 9,
    IRREG_SUP = 1 << 10,
    IRREG_DEL = 1 << 11,
    IRREG_INS = 1 << 12,
    IRREG_A = 1 << 13,
    IRREG_BDO = 1 << 14,
    IRREG_EM = 1 << 15,
    IRREG_STRONG = 1 << 16,
    IRREG_NOINDEX = 1 << 17,
    IRREG_NOSCRIPT = 1 << 18,
    IRREG_MARK = 1 << 19,
    IRREG_HLWORD = 1 << 20,
    IRREG_scope = -1,
};

/**
 * Namespaces.
 * Unlike in X(HT)ML, namespaces in HTML5 are not denoted by a prefix.  Rather,
 * anything inside an <svg> tag is in the SVG namespace, anything inside the
 * <math> tag is in the MathML namespace, and anything else is inside the HTML
 * namespace.  No other namespaces are supported, so this can be an enum only.
 */
enum class ETagNamespace : ui8 {
    HTML = 0,
    SVG,
    MATHML
};

enum HT_TAG {
    HT_PCDATA = 0x0000,
    HT_any = HT_PCDATA,
    // ie3.01 ver  html4.0  ie4
    HT_A,       //        1.0
    HT_ABBR,    // (-)    4.0           (-)
    HT_ACRONYM, // (-)    4.0
    HT_ADDRESS, //        2.0
    HT_APPLET,  //        3.2
    HT_AREA,    //        3.2
    HT_ARTICLE, //        5.0
    HT_ASIDE,   //        5.0
    HT_AUDIO,   //        5.0

    HT_B,          //        3.2
    HT_BASE,       //        3.2
    HT_BASEFONT,   //        4.0
    HT_BB,         //        5.0
    HT_BDO,        // (-)    4.0           (-)
    HT_BGSOUND,    //        ie2     (-)
    HT_BIG,        //        3.2
    HT_BLINK,      // (-)    ns?     (-)   (-)
    HT_BLOCKQUOTE, //        2.0
    HT_BODY,       //        2.0
    HT_BR,         //        2.0
    HT_BUTTON,     // (-)    4.0

    HT_CANVAS,   //        5.0
    HT_CAPTION,  //        3.2
    HT_CENTER,   //        3.2
    HT_CITE,     //        2.0
    HT_CODE,     //        2.0
    HT_COL,      //        4.0
    HT_COLGROUP, //        4.0
    HT_COMMAND,  //        5.0
    HT_COMMENT,  //        ie2,mo  (-)

    HT_DATAGRID, //        5.0
    HT_DATALIST, //        5.0
    HT_DD,       //        2.0
    HT_DEL,      // (-)    4.0
    HT_DETAILS,  //        5.0
    HT_DFN,      //        3.2
    HT_DIALOG,   //        5.0
    HT_DIR,      //        2.0
    HT_DIV,      //        3.2
    HT_DL,       //        2.0
    HT_DT,       //        2.0

    HT_EM,          //        2.0
    HT_EMBED,       //        ns2,ie3 (-)
    HT_EVENTSOURCE, //        5.0

    HT_FIELDSET, // (-)    4.0

    HT_FIGURE, //        5.0
    HT_FONT,   //        3.2
    HT_FOOTER, //        5.0

    HT_FORM,     //        2.0
    HT_FRAME,    //        4.0
    HT_FRAMESET, //        4.0

    HT_H1,     //        2.0
    HT_H2,     //        2.0
    HT_H3,     //        2.0
    HT_H4,     //        2.0
    HT_H5,     //        2.0
    HT_H6,     //        2.0
    HT_HEAD,   //        2.0
    HT_HEADER, //        5.0
    HT_HLWORD,
    HT_HR,   //        2.0
    HT_HTML, //        2.0

    HT_I,       //        2.0
    HT_IFRAME,  // (-)    4.0
    HT_ILAYER,  // (-)    ns4     (-)   (-)
    HT_IMG,     //        2.0
    HT_INPUT,   //        2.0
    HT_INS,     // (-)    4.0
    HT_ISINDEX, //        2.0

    HT_KBD,    //        2.0
    HT_KEYGEN, // (-)    ns3     (-)   (-)

    HT_LABEL,   // (-)    4.0
    HT_LAYER,   // (-)    ns4     (-)   (-)
    HT_LEGEND,  // (-)    4.0
    HT_LI,      //        2.0
    HT_LINK,    //        2.0
    HT_LISTING, //        2.0     (-)

    HT_MAP,      //        3.2
    HT_MARK,     //        5.0
    HT_MARQUEE,  //        ie2     (-)
    HT_MENU,     //        2.0
    HT_META,     //        2.0
    HT_METER,    //        5.0
    HT_MULTICOL, // (-)    ns3     (-)   (-)

    HT_NAV,      //        5.0
    HT_NEXTID,   // (-)    2.0     (-)   (-)
    HT_NOBR,     //        ns2,ie3 (-)
    HT_NOINDEX,  //        rambler
    HT_NOEMBED,  // (-)    ns2     (-)   (-)
    HT_NOFRAMES, //        4.0
    HT_NOSCRIPT, // (-)    4.0

    HT_OBJECT,   //        4.0
    HT_OL,       //        2.0
    HT_OPTGROUP, // (-)    4.0           (-)
    HT_OPTION,   //        2.0
    HT_OUTPUT,   //        5.0

    HT_P,         //        2.0
    HT_PARAM,     //        3.2
    HT_PLAINTEXT, //        2.0     (-)
    HT_PRE,       //        2.0
    HT_PROGRESS,  //        5.0

    HT_Q, // (-)    4.0

    HT_RP,   //        5.0
    HT_RT,   //        5.0
    HT_RUBY, //        5.0

    HT_S,       //        4.0
    HT_SAMP,    //        2.0
    HT_SCRIPT,  //        3.2
    HT_SECTION, //        5.0
    HT_SELECT,  //        2.0
    HT_SMALL,   //        3.2
    HT_SOUND,   // (-)    mo3     (-)  (-)
    HT_SOURCE,  //        5.0
    HT_SPACER,  // (-)    ns3     (-)  (-)
    HT_SPAN,    //        4.0
    HT_STRIKE,  //        3.2
    HT_STRONG,  //        2.0
    HT_STYLE,   //        3.2
    HT_SUB,     //        3.2
    HT_SUP,     //        3.2

    HT_TABLE,    //        3.2
    HT_TBODY,    //        3.2
    HT_TD,       //        3.2
    HT_TEXTAREA, //        2.0
    HT_TFOOT,    //        3.2
    HT_TH,       //        3.2
    HT_THEAD,    //        3.2
    HT_TIME,     //        5.0
    HT_TITLE,    //        2.0
    HT_TR,       //        3.2
    HT_TT,       //        2.0

    HT_U,  //        3.2
    HT_UL, //        2.0

    HT_VAR,   //        2.0
    HT_VIDEO, //        5.0

    HT_WBR, //        ns2,ie3 (-)

    HT_XML, //                (-)
    HT_XMP, //        2.0     (-)

    HT_BDI,        //        5.0
    HT_DATA,       //        5.0
    HT_FIGCAPTION, //      5.0
    HT_HGROUP,     //
    HT_IMAGE,      //
    HT_MAIN,       //        5.0
    HT_MENUITEM,   //
    HT_RB,         //        5.0
    HT_SUMMARY,    //        5.0
    HT_TEMPLATE,   //
    HT_TRACK,      //        5.0

    HT_MATH,       //        5.0     (mathml)
    HT_MALIGNMARK, //      5.0     (mathml)
    HT_MGLYPH,     //        5.0     (mathml)
    HT_MI,         //        5.0     (mathml)
    HT_MN,         //        5.0     (mathml)
    HT_MO,         //        5.0     (mathml)
    HT_MS,         //        5.0     (mathml)
    HT_MTEXT,      //        5.0     (mathml)

    HT_SVG,           //        5.0 (svg)
    HT_DESC,          //        5.0 (svg)
    HT_FOREIGNOBJECT, //   5.0 (svg)

    HT_AMP_VK,        // Google AMP tag for Vkontakte

    HT_TagCount, // number of tags hashed by gperf

    HT_pre = 0x00000100,                // <pre> <listing> <td nowrap>
    HT_irreg = 0x00000200,              // irregular markup <i>, etc
    HT_br = 0x00000400,                 // <address>,<p>,<div>,<h1>
    HT_list = 0x00000800,               // <ol>,<ul>,<menu>,<dir>
    HT_form = 0x00001000,               // <input> <textarea>
    HT_empty = 0x00002000,              // <EMPTY_TAG/>
    HT_wbr = 0x00004000,                // word breaking tags <WBR>
    HT_lit = 0x00008000,                // literals (CDATA), <script>, <style>, <title>, <textarea>, <xmp>
    HT_subwin = 0x00010000,             // context-switching tags <iframe>, <button>, <td>, <th>, <textarea>
    HT_head = 0x00020000,               // anything that can be outside of <body>
    HT_ = 0x00000000,                   // similar to literal, but all tags ignored except content-tags
    HT_table = 0x00080000,              // content of tables
    HT_frame = 0x00100000,              // frames
    HT_unscoped = HT_irreg | HT_subwin, // non-scoped irreg markers, now noscript and noindex

    HT_ignore = 0x00200000, // ignore in sanitizer
    HT_delete = 0x00400000, // delete with contents in sanitizer
    HT_par = 0x00800000,    // close <p>
    HT_subtxt = 0x01000000, // clear irregular markup on start
    HT_subtab = 0x02000000, // tags inside table

    H5_format = 0x00040000,
    H5_spec = 0x04000000,
    H5_scope = 0x08000000,
    H5_phrase = 0x80000000,

    HT_w0 = 0x10000000, // NOINDEX_RELEVANCE
    HT_w1 = 0x20000000, // HIGH_RELEVANCE
    HT_w2 = 0x40000000, // BEST_RELEVANCE

    HT_block = 0x04000000,     // block tag via w3c
    HT_inline = 0x08000000,    // inline tag via w3c
    HT_container = 0x00040000, // container tag via w3c
};

namespace NHtml {
    struct TTag {
        const char* name;
        const char* lowerName;
        int flags;
        TIrregTag irreg;

        unsigned len() const;

        HT_TAG id() const {
            return (HT_TAG)(flags & 0xFF);
        }
        bool operator==(HT_TAG tgId) const {
            return (HT_TAG)(flags & 0xFF) == tgId;
        }
        bool operator!=(HT_TAG tgId) const {
            return (HT_TAG)(flags & 0xFF) != tgId;
        }
        bool is(int f) const {
            return f < 0x100 ? (flags & 0xFF) == f : ((flags & f) == f);
        }
        bool is_irreg() const {
            return irreg != IRREG_none;
        }
    };

    const TTag& FindTag(const char* tagname);
    const TTag& FindTag(const char* tagname, size_t len);
    const TTag& FindTag(HT_TAG id);
    inline const TTag& FindTag(TStringBuf tagname) {
        return FindTag(tagname.data(), tagname.size());
    }

}
