#include "tags.h"

namespace NForumsImpl {

#define TAG(tag, name, value, match) \
    { tag, 0, name, sizeof(name) - 1, value, sizeof(value) - 1, TTagDescriptor::match }
#define TAG_SIMPLE(tag) \
    { tag, 0, NULL, 0, NULL, 0, TTagDescriptor::ExactMatch }
#define TAG_FLAGS(tag, flags, name, value, match) \
    { tag, flags, name, sizeof(name) - 1, value, sizeof(value) - 1, TTagDescriptor::match }
#define TAG_EMPTY \
    { HT_any, 0, NULL, 0, NULL, 0, TTagDescriptor::ExactMatch }

#define TAG_CHAIN(chain) \
    { sizeof(chain) / sizeof(chain[0]), chain }

const TTagDescriptor HrefTag = TAG(HT_A, "href", "javascript:", NotPrefix);

// === UBB.Threads ===
static TTagDescriptor UBBThreadsIdentChain[] = {
    TAG(HT_A, "name", "Post", PrefixWithoutDigits),
};

static TTagDescriptor UBBThreadsPostChain[] = {
    TAG(HT_TABLE, "class", "t_outer", ExactMatch),
};

static TTagDescriptor UBBThreadsMessageChain[] = {
    TAG(HT_DIV, "id", "body", PrefixWithoutDigits),
};

static TTagDescriptor SpanClassDate[] = {
    TAG(HT_SPAN, "class", "date", ExactMatch),
};

static TTagDescriptor UBBThreadsAuthorChain[] = {
    TAG(HT_SPAN, "id", "menu_control_", PrefixWithoutDigits),
};

static TTagDescriptor UBBThreadsQuoteChain[] = {
    TAG(HT_DIV, "class", "ubbcode-block", ExactMatch),
};

// PunBB markup 1
static TTagDescriptor PunBBIdentChain1[] = {
    TAG(HT_DIV, "id", "brd-messages", ExactMatch),
};

static TTagDescriptor PunBBPostChain1[] = {
    TAG(HT_DIV, "class", "post", Prefix),
    TAG_FLAGS(HT_DIV, TTagDescriptor::WaitForPrevClose, "id", "p", PrefixWithoutDigits),
};

static TTagDescriptor PunBBMessageChain1[] = {
    TAG(HT_DIV, "id", "post", PrefixWithoutDigits),
};

static TTagDescriptor PunBBDateChain1[] = {
    TAG(HT_SPAN, "class", "post-link", ExactMatch),
};

static TTagDescriptor PunBBAuthorChain1[] = {
    TAG(HT_LI, "class", "username", ExactMatch),
};

static TTagDescriptor PunBBQuoteChain1[] = {
    TAG(HT_DIV, "class", "quotebox", ExactMatch),
};

static TTagDescriptor PunBBQuoteHeaderChain1[] = {
    TAG(HT_DIV, "class", "quotebox", ExactMatch),
    TAG_SIMPLE(HT_CITE),
};

static TTagDescriptor PunBBSignatureChain1[] = {
    TAG(HT_DIV, "class", "sig-content", ExactMatch),
};

static TTagDescriptor PunBBTitleChain1[] = {
    TAG(HT_H1, "class", "main-title", ExactMatch),
    TAG(HT_A, "href", "", Prefix),
};

static TTagDescriptor PunBBSubforumChain1[] = {
    TAG(HT_DIV, "id", "category", PrefixWithoutDigits),
    TAG(HT_DIV, "id", "forum", PrefixWithoutDigits),
};

static TTagDescriptor PunBBTopicChain1[] = {
    TAG(HT_DIV, "id", "topic", PrefixWithoutDigits),
};

static TTagDescriptor PunBBForumNameChain1[] = {
    TAG(HT_DIV, "class", "item-subject", ExactMatch),
};

static TTagDescriptor PunBBNumTopicsChain1[] = {
    TAG(HT_LI, "class", "info-topics", ExactMatch),
};

static TTagDescriptor PunBBNumPostsChain1[] = {
    TAG(HT_LI, "class", "info-posts", ExactMatch),
};

static TTagDescriptor PunBBLastMessageChain1[] = {
    TAG(HT_LI, "class", "info-lastpost", ExactMatch),
    TAG_FLAGS(HT_SPAN, TTagDescriptor::NoDescent, "class", "label", ExactMatch),
};

static TTagDescriptor PunBBNumRepliesChain1[] = {
    TAG(HT_LI, "class", "info-replies", ExactMatch),
};

static TTagDescriptor PunBBNumViewsChain1[] = {
    TAG(HT_LI, "class", "info-views", ExactMatch),
};

// PunBB markup 2
static TTagDescriptor PunBBIdentChain2[] = {
    TAG(HT_DIV, "id", "punviewtopic", ExactMatch),
};

static TTagDescriptor PunBBPostChain2[] = {
    TAG(HT_DIV, "id", "p", PrefixWithoutDigits),
};

static TTagDescriptor PunBBMessageChain2[] = {
    TAG(HT_DIV, "class", "postmsg", ExactMatch),
};

static TTagDescriptor PunBBDateChain2[] = {
    TAG(HT_A, "href", "viewtopic.php?pid=", Prefix),
};

static TTagDescriptor PunBBAuthorChain2[] = {
    TAG(HT_DIV, "class", "postleft", ExactMatch),
    TAG_SIMPLE(HT_STRONG),
};

static TTagDescriptor PunBBSignatureChain2[] = {
    TAG(HT_DIV, "class", "postsignature", ExactMatch),
};

static TTagDescriptor PunBBQuoteHeaderChain2[] = {
    TAG_SIMPLE(HT_BLOCKQUOTE),
    TAG_SIMPLE(HT_H4),
};

// ExBB
static TTagDescriptor ExBBIdentChain[] = {
    TAG(HT_TABLE, "id", "ipbwrapper", ExactMatch),
};

static TTagDescriptor ExBBPostChain[] = {
    TAG(HT_TABLE, "class", "topic", ExactMatch),
};

static TTagDescriptor ExBBMessageChain[] = {
    TAG(HT_TD, "id", "post", PrefixWithoutDigits),
};

static TTagDescriptor ExBBDateChain[] = {
    TAG(HT_TR, "class", "row4", ExactMatch),
    TAG(HT_TD, "class", "postdetails", ExactMatch),
};

static TTagDescriptor ExBBAuthorChain[] = {
    TAG(HT_TD, "class", "normalname", ExactMatch),
};

static TTagDescriptor ExBBQuoteHeaderChain[] = {
    TAG(HT_DIV, "class", "block", ExactMatch),
};

static TTagDescriptor ExBBQuoteBodyChain[] = {
    TAG(HT_DIV, "class", "block", ExactMatch),
    TAG(HT_DIV, "class", "quote", ExactMatch),
};

static TTagDescriptor ExBBTitleChain[] = {
    TAG(HT_DIV, "id", "navstrip", ExactMatch),
    TAG_SIMPLE(HT_H1),
    TAG(HT_A, "href", "", Prefix),
};

static TTagDescriptor ExBBTitleChain2[] = {
    TAG(HT_TABLE, "id", "ipbwrapper", ExactMatch),
    TAG_SIMPLE(HT_H1),
    TAG(HT_A, "href", "", Prefix),
};

// InvisionPB, markup with <table>s
static TTagDescriptor InvisionPBIdentChain1[] = {
    TAG(HT_DIV, "id", "ipbwrapper", ExactMatch),
    TAG(HT_DIV, "id", "navstrip", ExactMatch),
};

static TTagDescriptor InvisionPBPostChain1[] = {
    TAG_SIMPLE(HT_TABLE),
    TAG_FLAGS(HT_A, TTagDescriptor::WaitForPrevClose, "name", "entry", PrefixWithoutDigits),
};

static TTagDescriptor InvisionPBMessageChain1[] = {
    TAG(HT_TD, "width", "100%", ExactMatch),
    TAG(HT_DIV, "class", "postcolor", ExactMatch),
};

static TTagDescriptor InvisionPBDateChain1[] = {
    TAG(HT_TD, "class", "row", PrefixWithoutDigits),
    TAG(HT_SPAN, "class", "postdetails", ExactMatch),
};

static TTagDescriptor InvisionPBAuthorChain1[] = {
    TAG(HT_SPAN, "class", "normalname", ExactMatch),
};

static TTagDescriptor InvisionPBQuoteHeaderChain1[] = {
    TAG(HT_DIV, "class", "quotetop", ExactMatch),
};

static TTagDescriptor InvisionPBQuoteChain1[] = {
    TAG(HT_DIV, "class", "quotemain", ExactMatch),
};

static TTagDescriptor InvisionPBTitleChain1a[] = {
    TAG(HT_DIV, "class", "maintitle", ExactMatch),
    TAG_FLAGS(HT_B, TTagDescriptor::WaitForParentClose, 0, 0, ExactMatch),
};

static TTagDescriptor InvisionPBTitleChain1b[] = {
    TAG(HT_DIV, "class", "maintitle", ExactMatch),
    TAG_FLAGS(HT_H1, TTagDescriptor::WaitForParentClose, 0, 0, ExactMatch),
};

static TTagDescriptor InvisionPBTitleChain1c[] = {
    TAG(HT_DIV, "class", "maintitle", ExactMatch),
    TAG_FLAGS(HT_H2, TTagDescriptor::WaitForParentClose, 0, 0, ExactMatch),
};

// InvisionPB, markup with <div>s
static TTagDescriptor InvisionPBIdentChain2[] = {
    TAG(HT_DIV, "id", "ipbwrapper", ExactMatch),
    TAG(HT_DIV, "id", "content", ExactMatch),
};

static TTagDescriptor InvisionPBPostChain2[] = {
    TAG(HT_DIV, "id", "post_id_", PrefixWithoutDigits),
};

static TTagDescriptor InvisionPBMessageChain2[] = {
    TAG(HT_DIV, "class", "post entry-content", Prefix),
};

static TTagDescriptor InvisionPBDateChain2[] = {
    TAG(HT_ABBR, "class", "published", ExactMatch),
};

static TTagDescriptor InvisionPBAuthorChain2[] = {
    TAG(HT_SPAN, "class", "author vcard", ExactMatch),
};

static TTagDescriptor InvisionPBQuoteHeaderChain2[] = {
    TAG(HT_P, "class", "citation", ExactMatch),
};

static TTagDescriptor InvisionPBQuoteChain2[] = {
    TAG(HT_DIV, "class", "quote", ExactMatch),
};

static TTagDescriptor InvisionPBSubforumChain2[] = {
    TAG(HT_DIV, "class", "category_block", Prefix),
    TAG_FLAGS(HT_H3, TTagDescriptor::WaitForPrevClose, "class", "maintitle", ExactMatch),
    TAG(HT_TABLE, "class", "ipb_table", ExactMatch),
    TAG_SIMPLE(HT_TR),
};

static TTagDescriptor InvisionPBForumNameChain2[] = {
    TAG(HT_TD, "class", "col_c_forum", Prefix),
};

static TTagDescriptor InvisionPBForumStatChain2[] = {
    TAG(HT_TD, "class", "col_c_stats", Prefix),
};

static TTagDescriptor InvisionPBLastMessageChain2[] = {
    TAG(HT_TD, "class", "col_c_post", Prefix),
};

static TTagDescriptor InvisionPBTitleChain2[] = {
    TAG(HT_H2, "class", "maintitle", ExactMatch),
    TAG(HT_SPAN, "class", "main_topic_title", ExactMatch),
    TAG_FLAGS(HT_SPAN, TTagDescriptor::NoDescent, "class", "main_topic_desc", Substring),
};

static TTagDescriptor InvisionPBTitleChain3[] = {
    TAG(HT_H1, "class", "titlepost", ExactMatch),
};

// vBulletin
static TTagDescriptor VBulletin3IdentChain[] = {
//    TAG(HT_TD, "class", "vbmenu_control", Prefix),
//    DECLARE_NODESCENT_TAG(HT_META, "content", "vBulletin 3", Prefix),
    TAG(HT_INPUT, "name", "vb_login_username", ExactMatch),
};

static TTagDescriptor VBulletin3PostChain[] = {
    TAG(HT_DIV, "id", "edit", PrefixWithoutDigits),
};

static TTagDescriptor VBulletin3MessageChain[] = {
    TAG(HT_DIV, "id", "post_message_", PrefixWithoutDigits),
};

static TTagDescriptor VBulletin3DateChain1[] = {
//    TAG(HT_TD, "class", "thead", ExactMatch),
    TAG_FLAGS(HT_A, TTagDescriptor::WaitForParentClose, "name", "post", PrefixWithoutDigits),
};

static TTagDescriptor VBulletin3DateChain2[] = {
    TAG(HT_TD, "valign", "bottom", ExactMatch),
    TAG(HT_P, "class", "smltxt", ExactMatch),
};

static TTagDescriptor VBulletin3DateChain3[] = {
    TAG(HT_TD, "id", "td_post_", PrefixWithoutDigits),
    TAG(HT_DIV, "class", "smallfont", ExactMatch),
};

static TTagDescriptor VBulletin3AuthorChain1[] = {
    TAG(HT_DIV, "id", "postmenu_", PrefixWithoutDigits),
};

static TTagDescriptor VBulletin3AuthorChain2[] = {
    TAG(HT_A, "class", "bigusername", ExactMatch),
};

static TTagDescriptor VBulletinSmallClass[] = {
    TAG_FLAGS(HT_any, TTagDescriptor::WaitForParentClose, "class", "smallfont", ExactMatch),
};

static TTagDescriptor VBulletin3TitleChain[] = {
    TAG(HT_TD, "class", "navbar", ExactMatch),
    TAG_SIMPLE(HT_STRONG),
};

static TTagDescriptor VBulletin3TitleChainAlt[] = {
    TAG(HT_TD, "class", "navbar", ExactMatch),
    TAG_SIMPLE(HT_H1),
};

static TTagDescriptor VBulletin3TitleChain2[] = {
    TAG(HT_DIV, "class", "page", ExactMatch),
    TAG_FLAGS(HT_A, TTagDescriptor::WaitForPrevClose, "accesskey", "1", ExactMatch),
    TAG_SIMPLE(HT_STRONG),
};

static TTagDescriptor VBulletin3TitleChain3[] = {
    TAG(HT_TABLE, "class", "navpan_table", ExactMatch),
    TAG(HT_TD, "class", "fr_head_block", ExactMatch),
    TAG(HT_SPAN, "class", "bc_row2", ExactMatch),
};

static TTagDescriptor VBulletin3TitleChain4[] = {
    TAG(HT_DIV, "class", "navbar", ExactMatch),
    TAG_FLAGS(HT_SPAN, TTagDescriptor::WaitForParentClose, "class", "navbar", ExactMatch),
    TAG_SIMPLE(HT_STRONG),
};

static TTagDescriptor VBulletin3SubforumChain[] = {
    TAG(HT_TABLE, "class", "list_tabs", Substring),
    TAG_SIMPLE(HT_TR),
};

static TTagDescriptor VBulletin3ForumNameChain[] = {
    TAG(HT_TD, "class", "list_razd", Substring),
};

static TTagDescriptor VBulletin3ForumLastMsgChain[] = {
    TAG(HT_TD, "class", "list_desc", Substring),
};

static TTagDescriptor VBulletin4IdentChain[] = {
//    TAG(HT_INPUT, "name", "vb_login_username", ExactMatch),
    TAG_FLAGS(HT_META, TTagDescriptor::WaitForPrevClose, "content", "vBulletin 4", Prefix),
};

static TTagDescriptor VBulletin4PostChain[] = {
    TAG(HT_LI, "id", "post_", PrefixWithoutDigits),
};

static TTagDescriptor VBulletin4MessageChain1[] = {
    TAG(HT_DIV, "id", "post_message_", PrefixWithoutDigits),
};

static TTagDescriptor VBulletin4MessageChain2[] = {
    TAG(HT_DIV, "class", "content", ExactMatch),
    TAG(HT_BLOCKQUOTE, "class", "restore", ExactMatch),
};

static TTagDescriptor VBulletin4AuthorChain1[] = {
    TAG(HT_DIV, "class", "username_container", ExactMatch),
};

static TTagDescriptor VBulletin4AuthorChain2[] = {
    TAG(HT_DIV, "class", "header", ExactMatch),
    TAG(HT_SPAN, "class", "username", ExactMatch),
};

static TTagDescriptor VBulletin4DateChain2[] = {
    TAG(HT_DIV, "class", "header", ExactMatch),
    TAG(HT_DIV, "class", "datetime", ExactMatch),
};

static TTagDescriptor VBulletin4QuoteHeaderChain[] = {
    TAG_SIMPLE(HT_BLOCKQUOTE),
    TAG(HT_DIV, "class", "bbcode_description", ExactMatch),
};

static TTagDescriptor VBulletin4QuoteChain[] = {
    TAG(HT_DIV, "class", "bbcode_quote", WordPrefix),
};

static TTagDescriptor VBulletin4SignatureChain[] = {
    TAG(HT_DIV, "class", "signaturecontainer", ExactMatch),
};

static TTagDescriptor VBulletin4BreadcrumbChain[] = {
    TAG(HT_DIV, "id", "breadcrumb", ExactMatch),
    TAG(HT_LI, "class", "navbit", ExactMatch),
    TAG(HT_A, "href", "", Prefix),
};

static TTagDescriptor VBulletin4TitleChain[] = {
    TAG(HT_DIV, "id", "pagetitle", ExactMatch),
    TAG(HT_SPAN, "class", "threadtitle", ExactMatch),
    TAG(HT_A, "href", "", Prefix),
};

static TTagDescriptor VBulletin4TitleChain2[] = {
    TAG(HT_DIV, "id", "pagetitle", ExactMatch),
    TAG_SIMPLE(HT_H1),
};

static TTagDescriptor VBulletin4SubforumChain[] = {
    TAG(HT_LI, "id", "forum", PrefixWithoutDigits),
};

static TTagDescriptor VBulletin4TopicChain[] = {
    TAG(HT_LI, "id", "thread_", PrefixWithoutDigits),
};

static TTagDescriptor VBulletin4ForumNameChain[] = {
    TAG(HT_DIV, "class", "foruminfo", Prefix),
};

static TTagDescriptor VBulletin4ForumLastMsgChain[] = {
    TAG(HT_DIV, "class", "forumlastpost", Prefix),
};

static TTagDescriptor VBulletin4ForumStatChain[] = {
    TAG(HT_UL, "class", "forumstats", Prefix),
};

static TTagDescriptor VBulletin4TopicNameChain[] = {
    TAG(HT_any, "class", "threadinfo", Prefix),
    TAG_FLAGS(HT_P, TTagDescriptor::NoDescent, "class", "threaddesc", ExactMatch),
};

static TTagDescriptor VBulletin4TopicLastMsgChain[] = {
    TAG(HT_DL, "class", "threadlastpost", Prefix),
};

static TTagDescriptor VBulletin4TopicStatChain[] = {
    TAG(HT_UL, "class", "threadstats", Prefix),
};

static TTagDescriptor VBulletin4SearchChain[] = {
    TAG(HT_OL, "class", "searchbits", Prefix),
};

// SMF
static TTagDescriptor SMFIdentChain[] = {
    TAG(HT_A, "id", "msg", PrefixWithoutDigits),
};

static TTagDescriptor SMFPostChain[] = {
    TAG(HT_DIV, "id", "forumposts", ExactMatch),
    TAG(HT_DIV, "class", "windowbg", Substring),
};

static TTagDescriptor SMFMessageChain[] = {
    TAG(HT_DIV, "id", "msg_", PrefixWithoutDigits),
};

static TTagDescriptor SMFDateChain[] = {
    TAG(HT_DIV, "class", "keyinfo", ExactMatch),
    TAG(HT_DIV, "class", "smalltext", ExactMatch),
};

static TTagDescriptor SMFAuthorChain[] = {
    TAG(HT_DIV, "class", "poster", Substring),
    TAG_SIMPLE(HT_H4),
};

static TTagDescriptor SMFQuoteHeaderChain[] = {
    TAG(HT_DIV, "class", "quoteheader", ExactMatch),
};

static TTagDescriptor SMFTitleChain[] = {
    TAG(HT_DIV, "class", "navigate_section", ExactMatch),
    TAG(HT_LI, "class", "last", ExactMatch),
    TAG(HT_A, "href", "", Prefix),
};

static TTagDescriptor TagBlockQuote[] = {
    TAG_SIMPLE(HT_BLOCKQUOTE),
};

static TTagDescriptor SMFForumListIdentChain[] = {
    TAG(HT_DIV, "id", "board", Prefix),
};

static TTagDescriptor SMFSubforumChain[] = {
    TAG(HT_TR, "id", "board_", PrefixWithoutDigits),
};

static TTagDescriptor SMFForumNameChain[] = {
    TAG(HT_TD, "class", "info", Prefix),
};

static TTagDescriptor SMFForumStatChain[] = {
    TAG(HT_TD, "class", "stats", Prefix),
};

static TTagDescriptor SMFForumLastMsgChain[] = {
    TAG(HT_TD, "class", "lastpost", Prefix),
};

// phpBB
static TTagDescriptor PhpBBIdentChain[] = {
    TAG(HT_TABLE, "class", "forumline", Substring),
};

static TTagDescriptor PhpBBPostChain[] = {
    TAG_SIMPLE(HT_TR),
    TAG_FLAGS(HT_SPAN, TTagDescriptor::WaitForPrevClose, "class", "name", ExactMatch),
};

static TTagDescriptor PhpBBMessageChain[] = {
    TAG(HT_TD, "class", "post", ExactMatch),
};

static TTagDescriptor PhpBBMessageChainAlt[] = {
    //TAG_FLAGS(HT_SPAN, TTagDescriptor::WaitForParentClose, "class", "postbody", ExactMatch),
    TAG(HT_SPAN, "class", "postbody", ExactMatch),
};

static TTagDescriptor PhpBBDateChain[] = {
    TAG(HT_SPAN, "class", "postdetails", ExactMatch),
};

static TTagDescriptor PhpBBAuthorChain[] = {
    TAG(HT_SPAN, "class", "name", ExactMatch),
};

static TTagDescriptor PhpBBQuoteHeaderChain[] = {
    TAG_SIMPLE(HT_CAPTION),
};

static TTagDescriptor PhpBBQuoteChain[] = {
    TAG_SIMPLE(HT_TABLE),
};

static TTagDescriptor PhpBBSignatureChain[] = {
    TAG(HT_SPAN, "class", "sig", ExactMatch),
};

static TTagDescriptor PhpBBSubforumChain[] = {
    TAG(HT_TD, "class", "fchr", Substring),
};

static TTagDescriptor PhpBBForumNameChain[] = {
    TAG_FLAGS(HT_SPAN, TTagDescriptor::WaitForParentClose, "class", "forumlink", ExactMatch),
};

static TTagDescriptor PhpBBTitleChain[] = {
    TAG(HT_A, "class", "maintitle", ExactMatch),
};

static TTagDescriptor PhpBBTitleChain3[] = {
    TAG(HT_H1, "class", "page-title", ExactMatch),
};

static TTagDescriptor PhpBBIdentChain2[] = {
    TAG(HT_BODY, "id", "phpbb", ExactMatch),
};

static TTagDescriptor PhpBBPostChain2[] = {
    TAG(HT_DIV, "id", "p", PrefixWithoutDigits),
};

static TTagDescriptor PhpBBMessageChain2[] = {
    TAG(HT_DIV, "class", "content", Prefix),
};

static TTagDescriptor PhpBBDateChain2[] = {
    TAG(HT_P, "class", "author", ExactMatch),
    TAG_FLAGS(HT_A, TTagDescriptor::NoDescent | TTagDescriptor::WaitForParentClose, "href", "", Prefix),
};

static TTagDescriptor PhpBBAuthorChain2a[] = {
    TAG(HT_P, "class", "author", ExactMatch),
    TAG_SIMPLE(HT_STRONG),
};

static TTagDescriptor PhpBBAuthorChain2b[] = {
    TAG(HT_DIV, "id", "profile", PrefixWithoutDigits),
    TAG_SIMPLE(HT_STRONG),
};

static TTagDescriptor PhpBBQuoteHeaderChain2[] = {
    TAG_SIMPLE(HT_BLOCKQUOTE),
    TAG_SIMPLE(HT_CITE),
};

static TTagDescriptor PhpBBQuoteChain2[] = {
    TAG_SIMPLE(HT_BLOCKQUOTE),
};

static TTagDescriptor DivClassSignature[] = {
    TAG(HT_DIV, "class", "signature", ExactMatch),
};

static TTagDescriptor PhpBBTitleChain2[] = {
    TAG_SIMPLE(HT_H2),
    TAG(HT_A, "href", "", Prefix),
};

static TTagDescriptor PhpBBForumChain2[] = {
    TAG(HT_UL, "class", "topiclist forums", ExactMatch),
    TAG(HT_LI, "class", "row", Prefix),
};

static TTagDescriptor PhpBBForumNameChain2[] = {
    TAG_SIMPLE(HT_DT),
    TAG_FLAGS(HT_SPAN, TTagDescriptor::NoDescent, "class", "sponsor", Prefix),
};

static TTagDescriptor PhpBBNumTopicsChain2[] = {
    TAG(HT_DD, "class", "topics", ExactMatch),
};

static TTagDescriptor PhpBBNumPostsChain2[] = {
    TAG(HT_DD, "class", "posts", ExactMatch),
};

static TTagDescriptor PhpBBLastMsgChain2[] = {
    TAG(HT_DD, "class", "lastpost", ExactMatch),
    TAG_FLAGS(HT_DFN, TTagDescriptor::NoDescent, NULL, 0, ExactMatch),
};

static TTagDescriptor PhpBBTopicChain2[] = {
    TAG(HT_UL, "class", "topiclist topics", ExactMatch),
    TAG(HT_LI, "class", "row", Prefix),
};

static TTagDescriptor PhpBBTopicNameChain2[] = {
    TAG_SIMPLE(HT_DT),
};

static TTagDescriptor PhpBBNumAnswersChain2[] = {
    TAG(HT_DD, "class", "answers", ExactMatch),
};

static TTagDescriptor PhpBBNumViewsChain2[] = {
    TAG(HT_DD, "class", "views", ExactMatch),
};

static TTagDescriptor PhpBBIdentChain3[] = {
    TAG_FLAGS(HT_A, TTagDescriptor::WaitForParentClose, "name", "top", ExactMatch),
    TAG(HT_DIV, "id", "wrapheader", ExactMatch),
    TAG(HT_DIV, "id", "logodesc", ExactMatch),
};

static TTagDescriptor PhpBBPostChain3[] = {
    TAG(HT_DIV, "id", "pagecontent", ExactMatch),
    TAG(HT_TABLE, "class", "tablebg", ExactMatch),
    TAG_FLAGS(HT_TR, TTagDescriptor::WaitForPrevClose, "class", "row", Prefix),
};

static TTagDescriptor PhpBBMessageChain3[] = {
    TAG(HT_DIV, "class", "postbody", ExactMatch),
};

static TTagDescriptor PhpBBDateChain3[] = {
    TAG(HT_TD, "class", "gensmall", ExactMatch),
};

static TTagDescriptor PhpBBAuthorChain3[] = {
    TAG_FLAGS(HT_A, TTagDescriptor::WaitForParentClose, "name", "p", PrefixWithoutDigits),
};

static TTagDescriptor PhpBBQuoteHeaderChain3[] = {
    TAG(HT_DIV, "class", "quotetitle", ExactMatch),
};

static TTagDescriptor PhpBBQuoteBodyChain3[] = {
    TAG(HT_DIV, "class", "quotecontent", ExactMatch),
};

// ucoz, markup with <div>s
static TTagDescriptor UcozIdentChain1[] = {
    TAG(HT_DIV, "id", "wrap", ExactMatch),
    TAG(HT_DIV, "id", "content", ExactMatch),
    TAG(HT_DIV, "class", "forumContent", ExactMatch),
};

static TTagDescriptor UcozPostChain1[] = {
    TAG(HT_TR, "id", "post", PrefixWithoutDigits),
};

static TTagDescriptor UcozMessageChain1[] = {
    TAG(HT_DIV, "class", "pc-main", Prefix),
    TAG(HT_DIV, "id", "ms_", PrefixWithoutDigits),
};

static TTagDescriptor UcozDateChain1[] = {
    TAG(HT_DIV, "class", "pc-top", Prefix),
    TAG(HT_SPAN, "class", "post-date", ExactMatch),
};

static TTagDescriptor UcozAuthorChain1[] = {
    TAG(HT_DIV, "class", "post-user", ExactMatch),
    TAG(HT_A, "class", "pusername postUser", ExactMatch),
};

static TTagDescriptor UcozQuoteHeaderChain1[] = {
    TAG(HT_DIV, "class", "bbQuoteBlock", ExactMatch),
    TAG(HT_DIV, "class", "bbQuoteName", ExactMatch),
};

static TTagDescriptor UcozQuoteBodyChain1[] = {
    TAG(HT_DIV, "class", "bbQuoteBlock", ExactMatch),
    TAG(HT_DIV, "class", "quoteMessage", ExactMatch),
};

static TTagDescriptor UcozSignatureChain1[] = {
    TAG(HT_DIV, "class", "signatureView", ExactMatch),
};

static TTagDescriptor UcozTitleChain1[] = {
    TAG(HT_TD, "class", "gTableTop", ExactMatch),
};

// ucoz, markup with <td>s
static TTagDescriptor UcozIdentChain2[] = {
//  TAG(HT_TD, "class", "forumtd", ExactMatch),
//  TAG(HT_DIV, "id", "forum-top-links", ExactMatch),
    TAG(HT_DIV, "class", "gDivLeft", ExactMatch),
    TAG(HT_DIV, "class", "gDivRight", ExactMatch),
    TAG(HT_TABLE, "class", "gTable", ExactMatch),
};

static TTagDescriptor UcozMessageChain2[] = {
    TAG(HT_TD, "class", "posttdMessage", ExactMatch),
};

static TTagDescriptor UcozDateChain2[] = {
    TAG(HT_TD, "class", "postTdTop", ExactMatch),
    TAG_FLAGS(HT_A, TTagDescriptor::NoDescent, "", "", ExactMatch),
};

static TTagDescriptor UcozAuthorChain2[] = {
    TAG(HT_TD, "class", "postTdTop", ExactMatch),
    TAG_SIMPLE(HT_A),
};

static TTagDescriptor UcozSignatureChain2[] = {
    TAG(HT_SPAN, "class", "signatureView", ExactMatch),
};

// DonanimHaber
static TTagDescriptor DonanimHaberIdentChain[] = {
    TAG(HT_DIV, "id", "zip", ExactMatch),
};

static TTagDescriptor DonanimHaberPostChain[] = {
    TAG(HT_TABLE, "id", "replies_ust_", PrefixWithoutDigits),
};

static TTagDescriptor DonanimHaberMessageChain[] = {
    TAG(HT_SPAN, "class", "msg", ExactMatch),
    TAG(HT_SPAN, "id", "contextual", ExactMatch),
};

static TTagDescriptor DonanimHaberDateChain[] = {
    TAG(HT_TD, "class", "info", ExactMatch),
    TAG(HT_SPAN, "class", "mButon info", ExactMatch),
};

static TTagDescriptor DonanimHaberAuthorChain[] = {
    TAG(HT_DIV, "class", "singlepost-left", ExactMatch),
    TAG(HT_DIV, "class", "info nick", ExactMatch),
};

static TTagDescriptor DonanimHaberQuoteHeaderChain[] = {
    TAG(HT_BLOCKQUOTE, "class", "quote", ExactMatch),
};

static TTagDescriptor DonanimHaberQuoteBodyChain[] = {
    TAG(HT_BLOCKQUOTE, "class", "quote", ExactMatch),
};

static TTagDescriptor DonanimHaberTitleChain[] = {
    TAG(HT_SPAN, "class", "messagetitle", ExactMatch),
};


static TMarkup KnownMarkups[] = {
    {
        TForumsHandler::UBBThreads,
        TAG_CHAIN(UBBThreadsIdentChain),
        TAG_CHAIN(UBBThreadsPostChain),
        TAG_CHAIN(UBBThreadsMessageChain), {0, nullptr},
        {TAG_CHAIN(SpanClassDate)},
        TAG_CHAIN(UBBThreadsAuthorChain), {0, nullptr},
        {0, nullptr}, TAG_CHAIN(UBBThreadsQuoteChain),
        TAG_CHAIN(DivClassSignature),
        false, // SignatureInsideMessage
        false, // QuoteOutsideMessage
        false,
        false,
        "Post", // AnchorPrefix
        nullptr,   // SidParameterName
        {0, nullptr},  // SubforumTag
        {0, nullptr},  // TopicTag
        {0, nullptr},  // SearchTag
        {{{0, nullptr}, TS_COUNT, false, false}}, // TableItemTags
    },
    {
        TForumsHandler::PunBB,
        TAG_CHAIN(PunBBIdentChain1),
        TAG_CHAIN(PunBBPostChain1),
        TAG_CHAIN(PunBBMessageChain1), {0, nullptr},
        {TAG_CHAIN(PunBBDateChain1)},
        TAG_CHAIN(PunBBAuthorChain1), {0, nullptr},
        TAG_CHAIN(PunBBQuoteHeaderChain1), TAG_CHAIN(PunBBQuoteChain1),
        TAG_CHAIN(PunBBSignatureChain1),
        true, // SignatureInsideMessage
        false, // QuoteOutsideMessage
        false,
        true,
        "p", // AnchorPrefix
        nullptr, // SidParameterName
        TAG_CHAIN(PunBBSubforumChain1), // SubforumTag
        TAG_CHAIN(PunBBTopicChain1), // TopicTag
        {0, nullptr},  // SearchTag
        {   // TableItemTags
            {TAG_CHAIN(PunBBForumNameChain1), TS_FORUM_NAME, true, false},
            {TAG_CHAIN(PunBBNumTopicsChain1), TS_NUM_TOPICS, true, false},
            {TAG_CHAIN(PunBBNumPostsChain1), TS_NUM_MESSAGES, true, false},
            {TAG_CHAIN(PunBBLastMessageChain1), TS_LAST_MESSAGE, true, true},
            {TAG_CHAIN(PunBBForumNameChain1), TS_TOPIC_NAME, false, true},
            {TAG_CHAIN(PunBBNumRepliesChain1), TS_NUM_MESSAGES2, false, true},
            {TAG_CHAIN(PunBBNumViewsChain1), TS_NUM_VIEWS, false, true},
            {{0, nullptr}, TS_COUNT, false, false}
        },
    },
    {
        TForumsHandler::PunBB,
        TAG_CHAIN(PunBBIdentChain2),
        TAG_CHAIN(PunBBPostChain2),
        TAG_CHAIN(PunBBMessageChain2), {0, nullptr},
        {TAG_CHAIN(PunBBDateChain2)},
        TAG_CHAIN(PunBBAuthorChain2), {0, nullptr},
        TAG_CHAIN(PunBBQuoteHeaderChain2), TAG_CHAIN(TagBlockQuote),
        TAG_CHAIN(PunBBSignatureChain2),
        false, // SignatureInsideMessage
        false, // QuoteOutsideMessage
        false,
        true,
        "p", // AnchorPrefix
        nullptr, // SidParameterName
        {0, nullptr},  // SubforumTag
        {0, nullptr},  // TopicTag
        {0, nullptr},  // SearchTag
        {{{0, nullptr}, TS_COUNT, false, false}}, // TableItemTags
    },
    {
        TForumsHandler::ExBB,
        TAG_CHAIN(ExBBIdentChain),
        TAG_CHAIN(ExBBPostChain),
        TAG_CHAIN(ExBBMessageChain), {0, nullptr},
        {TAG_CHAIN(ExBBDateChain)},
        TAG_CHAIN(ExBBAuthorChain), {0, nullptr},
        TAG_CHAIN(ExBBQuoteHeaderChain), TAG_CHAIN(ExBBQuoteBodyChain),
        {0, nullptr},
        false, // SignatureInsideMessage
        false, // QuoteOutsideMessage
        false,
        false,
        "",
        nullptr, // SidParameterName
        {0, nullptr},  // SubforumTag
        {0, nullptr},  // TopicTag
        {0, nullptr},  // SearchTag
        {{{0, nullptr}, TS_COUNT, false, false}}, // TableItemTags
    },
    {
        TForumsHandler::InvisionPB,
        TAG_CHAIN(InvisionPBIdentChain1),
        TAG_CHAIN(InvisionPBPostChain1),
        TAG_CHAIN(InvisionPBMessageChain1), {0, nullptr},
        {TAG_CHAIN(InvisionPBDateChain1)},
        TAG_CHAIN(InvisionPBAuthorChain1), {0, nullptr},
        TAG_CHAIN(InvisionPBQuoteHeaderChain1), TAG_CHAIN(InvisionPBQuoteChain1),
        TAG_CHAIN(DivClassSignature),
        false, // SignatureInsideMessage
        true, // QuoteOutsideMessage
        false,
        false,
        "entry",
        "s", // SidParameterName
        {0, nullptr},  // SubforumTag
        {0, nullptr},  // TopicTag
        {0, nullptr},  // SearchTag
        {{{0, nullptr}, TS_COUNT, false, false}}, // TableItemTags
    },
    {
        TForumsHandler::InvisionPB,
        TAG_CHAIN(InvisionPBIdentChain2),
        TAG_CHAIN(InvisionPBPostChain2),
        TAG_CHAIN(InvisionPBMessageChain2), {0, nullptr},
        {TAG_CHAIN(InvisionPBDateChain2)},
        TAG_CHAIN(InvisionPBAuthorChain2), {0, nullptr},
        TAG_CHAIN(InvisionPBQuoteHeaderChain2), TAG_CHAIN(InvisionPBQuoteChain2),
        TAG_CHAIN(DivClassSignature),
        true, // SignatureInsideMessage
        false, // QuoteOutsideMessage
        false,
        false,
        "entry",
        "s",    // SidParameterName
        TAG_CHAIN(InvisionPBSubforumChain2),  // SubforumTag
        {0, nullptr},  // TopicTag
        {0, nullptr},  // SearchTag
        {   // TableItemTags
            {TAG_CHAIN(InvisionPBForumNameChain2), TS_FORUM_NAME, true, false},
            {TAG_CHAIN(InvisionPBForumStatChain2), TS_TWO_NUMBERS, true, false},
            {TAG_CHAIN(InvisionPBLastMessageChain2), TS_LAST_MESSAGE, true, false},
            {{0, nullptr}, TS_COUNT, false, false},
        },
    },
    {
        TForumsHandler::VBulletin,
        TAG_CHAIN(VBulletin3IdentChain),
        TAG_CHAIN(VBulletin3PostChain),
        TAG_CHAIN(VBulletin3MessageChain), {0, nullptr},
        {TAG_CHAIN(VBulletin3DateChain1), TAG_CHAIN(VBulletin3DateChain2), TAG_CHAIN(VBulletin3DateChain3)},
        TAG_CHAIN(VBulletin3AuthorChain1), TAG_CHAIN(VBulletin3AuthorChain2),
        {0, nullptr}, TAG_CHAIN(VBulletinSmallClass),
        TAG_CHAIN(VBulletinSmallClass),
        false, // SignatureInsideMessage
        false, // QuoteOutsideMessage
        true,
        false,
        "post",
        "s", // SidParameterName
        TAG_CHAIN(VBulletin3SubforumChain), // SubforumTag
        {0, nullptr},  // TopicTag
        {0, nullptr},  // SearchTag
        {   // TableItemTags
            {TAG_CHAIN(VBulletin3ForumNameChain), TS_FORUM_NAME, true, false},
            {TAG_CHAIN(VBulletin3ForumLastMsgChain), TS_LAST_MESSAGE, true, false},
            {{0, nullptr}, TS_COUNT, false, false},
        },
    },
    {
        TForumsHandler::VBulletin,
        TAG_CHAIN(VBulletin4IdentChain),
        TAG_CHAIN(VBulletin4PostChain),
        TAG_CHAIN(VBulletin4MessageChain1), TAG_CHAIN(VBulletin4MessageChain2),
        {TAG_CHAIN(SpanClassDate), TAG_CHAIN(VBulletin4DateChain2)},
        TAG_CHAIN(VBulletin4AuthorChain1), TAG_CHAIN(VBulletin4AuthorChain2),
        TAG_CHAIN(VBulletin4QuoteHeaderChain), TAG_CHAIN(VBulletin4QuoteChain),
        TAG_CHAIN(VBulletin4SignatureChain),
        false, // SignatureInsideMessage
        false, // QuoteOutsideMessage
        false,
        false,
        "post",
        "s", // SidParameterName
        TAG_CHAIN(VBulletin4SubforumChain),
        TAG_CHAIN(VBulletin4TopicChain),
        TAG_CHAIN(VBulletin4SearchChain),
        {   // TableItemTags
            {TAG_CHAIN(VBulletin4ForumNameChain), TS_FORUM_NAME, true, false},
            {TAG_CHAIN(VBulletin4ForumLastMsgChain), TS_LAST_MESSAGE, true, false},
            {TAG_CHAIN(VBulletin4ForumStatChain), TS_TWO_NUMBERS, true, false},
            {TAG_CHAIN(VBulletin4TopicNameChain), TS_TOPIC_NAME, false, true},
            {TAG_CHAIN(VBulletin4TopicLastMsgChain), TS_LAST_MESSAGE2, false, true},
            {TAG_CHAIN(VBulletin4TopicStatChain), TS_TWO_NUMBERS2, false, true},
            {{0, nullptr}, TS_COUNT, false, false}
        },
    },
    {
        TForumsHandler::SimpleMachinesForum,
        TAG_CHAIN(SMFIdentChain),
        TAG_CHAIN(SMFPostChain),
        TAG_CHAIN(SMFMessageChain), {0, nullptr},
        {TAG_CHAIN(SMFDateChain)},
        TAG_CHAIN(SMFAuthorChain), {0, nullptr},
        TAG_CHAIN(SMFQuoteHeaderChain), TAG_CHAIN(TagBlockQuote),
        TAG_CHAIN(DivClassSignature),
        false, // SignatureInsideMessage
        false, // QuoteOutsideMessage
        false,
        false,
        "msg_",
        "PHPSESSID", // SidParameterName
        {0, nullptr},  // SubforumTag
        {0, nullptr},  // TopicTag
        {0, nullptr},  // SearchTag
        {{{0, nullptr}, TS_COUNT, false, false}}, // TableItemTags
    },
    {
        TForumsHandler::SimpleMachinesForum,
        TAG_CHAIN(SMFForumListIdentChain),
        TAG_CHAIN(SMFPostChain),
        TAG_CHAIN(SMFMessageChain), {0, nullptr},
        {TAG_CHAIN(SMFDateChain)},
        TAG_CHAIN(SMFAuthorChain), {0, nullptr},
        TAG_CHAIN(SMFQuoteHeaderChain), TAG_CHAIN(TagBlockQuote),
        TAG_CHAIN(DivClassSignature),
        false, // SignatureInsideMessage
        false, // QuoteOutsideMessage
        false,
        false,
        "msg_",
        "PHPSESSID", // SidParameterName
        TAG_CHAIN(SMFSubforumChain), // SubforumTag
        {0, nullptr},  // TopicTag
        {0, nullptr},  // SearchTag
        {   // TableItemTags
            {TAG_CHAIN(SMFForumNameChain), TS_FORUM_NAME, true, false},
            {TAG_CHAIN(SMFForumStatChain), TS_TWO_NUMBERS, true, false},
            {TAG_CHAIN(SMFForumLastMsgChain), TS_LAST_MESSAGE, true, false},
            {{0, nullptr}, TS_COUNT, false, false}
        },
    },
    {
        TForumsHandler::PhpBB,
        TAG_CHAIN(PhpBBIdentChain),
        TAG_CHAIN(PhpBBPostChain),
        TAG_CHAIN(PhpBBMessageChain), TAG_CHAIN(PhpBBMessageChainAlt),
        {TAG_CHAIN(PhpBBDateChain), TAG_CHAIN(PhpBBDateChain3)},
        TAG_CHAIN(PhpBBAuthorChain), {0, nullptr},
        TAG_CHAIN(PhpBBQuoteHeaderChain), TAG_CHAIN(PhpBBQuoteChain),
        TAG_CHAIN(PhpBBSignatureChain),
        //{0, NULL},
        true, // SignatureInsideMessage
        false, // QuoteOutsideMessage
        true,
        true,
        "p",
        "sid", // SidParameterName
        TAG_CHAIN(PhpBBSubforumChain), // SubforumTag
        {0, nullptr},  // TopicTag
        {0, nullptr},  // SearchTag
        {   // TableItemTags
            {TAG_CHAIN(PhpBBForumNameChain), TS_FORUM_NAME, true, false},
            {{0, nullptr}, TS_COUNT, false, false},
        },
    },
    {
        TForumsHandler::PhpBB,
        TAG_CHAIN(PhpBBIdentChain2),
        TAG_CHAIN(PhpBBPostChain2),
        TAG_CHAIN(PhpBBMessageChain2), {0, nullptr},
        {TAG_CHAIN(PhpBBDateChain2)},
        TAG_CHAIN(PhpBBAuthorChain2a), TAG_CHAIN(PhpBBAuthorChain2b),
        TAG_CHAIN(PhpBBQuoteHeaderChain2), TAG_CHAIN(PhpBBQuoteChain2),
        TAG_CHAIN(DivClassSignature),
        false, // SignatureInsideMessage
        false, // QuoteOutsideMessage
        true,
        true,
        "p",
        nullptr, // SidParameterName
        TAG_CHAIN(PhpBBForumChain2), // SubforumTag
        TAG_CHAIN(PhpBBTopicChain2), // TopicTag
        {0, nullptr},  // SearchTag
        {   // TableItemTags
            {TAG_CHAIN(PhpBBForumNameChain2), TS_FORUM_NAME, true, false},
            {TAG_CHAIN(PhpBBNumTopicsChain2), TS_NUM_TOPICS, true, false},
            {TAG_CHAIN(PhpBBNumPostsChain2), TS_NUM_MESSAGES, true, false},
            {TAG_CHAIN(PhpBBLastMsgChain2), TS_LAST_MESSAGE, true, true},
            {TAG_CHAIN(PhpBBTopicNameChain2), TS_TOPIC_NAME, false, true},
            {TAG_CHAIN(PhpBBNumAnswersChain2), TS_NUM_MESSAGES2, false, true},
            {TAG_CHAIN(PhpBBNumViewsChain2), TS_NUM_VIEWS, false, true},
            {{0, nullptr}, TS_COUNT, false, false},
        },
    },
    {
        TForumsHandler::PhpBB,
        TAG_CHAIN(PhpBBIdentChain3),
        TAG_CHAIN(PhpBBPostChain3),
        TAG_CHAIN(PhpBBMessageChain3), {0, nullptr},
        {TAG_CHAIN(PhpBBDateChain3)},
        TAG_CHAIN(PhpBBAuthorChain3), {0, nullptr},
        TAG_CHAIN(PhpBBQuoteHeaderChain3), TAG_CHAIN(PhpBBQuoteBodyChain3),
        {0, nullptr},
        false, // SignatureInsideMessage
        false, // QuoteOutsideMessage
        false,
        false,
        "p",
        nullptr, // SidParameterName
        {0, nullptr},  // SubforumTag
        {0, nullptr},  // TopicTag
        {0, nullptr},  // SearchTag
        {{{0, nullptr}, TS_COUNT, false, false}}, // TableItemTags
    },
    {
        TForumsHandler::Ucoz,
        TAG_CHAIN(UcozIdentChain1),
        TAG_CHAIN(UcozPostChain1),
        TAG_CHAIN(UcozMessageChain1), {0, nullptr},
        {TAG_CHAIN(UcozDateChain1)},
        TAG_CHAIN(UcozAuthorChain1), {0, nullptr},
        TAG_CHAIN(UcozQuoteHeaderChain1), TAG_CHAIN(UcozQuoteBodyChain1),
        TAG_CHAIN(UcozSignatureChain1),
        false,  // SignatureInsideMessage
        false,  // QuoteOutsideMessage
        false,  // QuoteMetaInsideBody
        false,  // QuoteHeaderInsideBody
        "post",
        nullptr, // SidParameterName
        {0, nullptr},  // SubforumTag
        {0, nullptr},  // TopicTag
        {0, nullptr},  // SearchTag
        {{{0, nullptr}, TS_COUNT, false, false}}, // TableItemTags
    },
    {
        TForumsHandler::Ucoz,
        TAG_CHAIN(UcozIdentChain2),
        TAG_CHAIN(UcozPostChain1),  // same as in first markup
        TAG_CHAIN(UcozMessageChain2), {0, nullptr},
        {TAG_CHAIN(UcozDateChain2)},
        TAG_CHAIN(UcozAuthorChain2), {0, nullptr},
        TAG_CHAIN(UcozQuoteHeaderChain1), TAG_CHAIN(UcozQuoteBodyChain1),   // same as in first markup
        TAG_CHAIN(UcozSignatureChain2),
        true,   // SignatureInsideMessage
        false,  // QuoteOutsideMessage
        false,  // QuoteMetaInsideBody
        false,  // QuoteHeaderInsideBody
        "post",
        nullptr, // SidParameterName
        {0, nullptr},  // SubforumTag
        {0, nullptr},  // TopicTag
        {0, nullptr},  // SearchTag
        {{{0, nullptr}, TS_COUNT, false, false}}, // TableItemTags
    },
    {
        // INFO: signature, quote author and nested quotes (looks like single quote) don't fetch
        TForumsHandler::DonanimHaber,
        TAG_CHAIN(DonanimHaberIdentChain),
        TAG_CHAIN(DonanimHaberPostChain),
        TAG_CHAIN(DonanimHaberMessageChain), {0, nullptr},
        {TAG_CHAIN(DonanimHaberDateChain)},
        TAG_CHAIN(DonanimHaberAuthorChain), {0, nullptr},
        TAG_CHAIN(DonanimHaberQuoteHeaderChain), TAG_CHAIN(DonanimHaberQuoteBodyChain),
        {0, nullptr},  // SugnatureTag
        false,  // SignatureInsideMessage
        false,  // QuoteOutsideMessage
        false,  // QuoteMetaInsideBody
        false,  // QuoteHeaderInsideBody
        "replies_ust_",
        nullptr, // SidParameterName
        {0, nullptr},  // SubforumTag
        {0, nullptr},  // TopicTag
        {0, nullptr},  // SearchTag
        {{{0, nullptr}, TS_COUNT, false, false}}, // TableItemTags
    },
};

const int NumKnownMarkups = sizeof(KnownMarkups) / sizeof(KnownMarkups[0]);

static const TTagChainDescriptor PossibleBreadcrumbs[] = {
    TAG_CHAIN(VBulletin4BreadcrumbChain),
};

const int NumPossibleBreadcrumbs = sizeof(PossibleBreadcrumbs) / sizeof(PossibleBreadcrumbs[0]);

static const TTagChainDescriptor PossibleTitles[] = {
    TAG_CHAIN(VBulletin3TitleChain),
    TAG_CHAIN(VBulletin3TitleChainAlt),
    TAG_CHAIN(VBulletin3TitleChain2),
    TAG_CHAIN(VBulletin3TitleChain3),
    TAG_CHAIN(VBulletin3TitleChain4),
    TAG_CHAIN(VBulletin4TitleChain),
    TAG_CHAIN(VBulletin4TitleChain2),
    TAG_CHAIN(InvisionPBTitleChain1a),
    TAG_CHAIN(InvisionPBTitleChain1b),
    TAG_CHAIN(InvisionPBTitleChain1c),
    TAG_CHAIN(InvisionPBTitleChain2),
    TAG_CHAIN(InvisionPBTitleChain3),
    TAG_CHAIN(PhpBBTitleChain),
    TAG_CHAIN(PhpBBTitleChain2),
    TAG_CHAIN(PhpBBTitleChain3),
    TAG_CHAIN(SMFTitleChain),
    TAG_CHAIN(ExBBTitleChain),
    TAG_CHAIN(ExBBTitleChain2),
    TAG_CHAIN(PunBBTitleChain1),
    TAG_CHAIN(UcozTitleChain1),
    TAG_CHAIN(DonanimHaberTitleChain),
};

const int NumPossibleTitles = sizeof(PossibleTitles) / sizeof(PossibleTitles[0]);

void TTagsProcessor::ProcessOpenTag(HT_TAG tag, const THtmlChunk* chunk, int depth, const TNumerStat& numerStat, TPostDescriptor* lastPost)
{
    CurrentIterType = EOpen;
    for (CurrentIter = OpenTagHandlers[tag].begin(); CurrentIter != OpenTagHandlers[tag].end(); ) {
        THandlerInfo* info = *CurrentIter;
        ++CurrentIter;
        HT_TAG nextOpenTag, nextCloseTag;
        TTagChainTracker::EStateChange state = info->Tracker.OnOpenTag(chunk, depth, nextOpenTag, nextCloseTag);
        ChangeOpenTag(info, nextOpenTag, false);
        ChangeCloseTag(info, nextCloseTag, false);
        if (state != TTagChainTracker::NotChanged) {
            size_t markup = info - &HandlersInfo[HANDLER_MARKUP];
            if (markup < (size_t)NumKnownMarkups) // unsigned comparison
                ChosenMarkup = KnownMarkups + markup;
            info->Handler(info->Param, state == TTagChainTracker::AreaEntered, numerStat, lastPost);
        }
    }
    CurrentIterType = ENothing;
}

void TTagsProcessor::ProcessCloseTag(HT_TAG tag, const THtmlChunk* chunk, int depth, const TNumerStat& numerStat, TPostDescriptor* lastPost)
{
    CurrentIterType = EClose;
    for (CurrentIter = CloseTagHandlers[tag].begin(); CurrentIter != CloseTagHandlers[tag].end(); ) {
        THandlerInfo* info = *CurrentIter;
        ++CurrentIter;
        HT_TAG nextOpenTag, nextCloseTag;
        TTagChainTracker::EStateChange state = info->Tracker.OnCloseTag(chunk, depth, nextOpenTag, nextCloseTag);
        ChangeOpenTag(info, nextOpenTag, false);
        ChangeCloseTag(info, nextCloseTag, false);
        if (state != TTagChainTracker::NotChanged) {
            size_t markup = info - &HandlersInfo[HANDLER_MARKUP];
            if (markup < (size_t)NumKnownMarkups) // unsigned comparison
                ChosenMarkup = KnownMarkups + markup;
            info->Handler(info->Param, state == TTagChainTracker::AreaEntered, numerStat, lastPost);
        }
    }
    CurrentIterType = ENothing;
}

void TTagsProcessor::Init()
{
    CurrentIterType = ENothing;
    for (size_t i = 0; i < HT_TagCount; i++) {
        OpenTagHandlers[i].clear();
        CloseTagHandlers[i].clear();
    }
    for (int i = 0; i < NUM_HANDLERS; i++) {
        THandlerInfo& h = HandlersInfo[i];
        h.CurOpen = HT_TagCount;
        h.CurClose = HT_TagCount;
        h.Tracker.Clear();
    }
    ChosenMarkup = nullptr;
    for (int i = 0; i < NumKnownMarkups; i++) {
        THandlerId h = HANDLER_MARKUP + i;
        SetAssociatedChain(h, &KnownMarkups[i].IdentTag);
        Start(h);
    }
    for (int i = 0; i < NumPossibleBreadcrumbs; i++) {
        THandlerId h = HANDLER_BREADCRUMB_SPECIFIC + i;
        SetAssociatedChain(h, &PossibleBreadcrumbs[i]);
        Start(h);
    }
    for (int i = 0; i < NumPossibleTitles; i++) {
        THandlerId h = HANDLER_TITLE + i;
        SetAssociatedChain(h, &PossibleTitles[i]);
        Start(h);
    }
}

} // namespace NForumsImpl
