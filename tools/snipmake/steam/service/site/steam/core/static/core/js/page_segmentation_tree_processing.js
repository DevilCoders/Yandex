BLOCK_TAGS = ["address", "article", "aside", "audio",
              "blockquote", "body",
              "canvas", "center",
              "dd", "div", "dl",
              "fieldset", "figcaption", "figure", "footer", "form",
              "h1", "h2", "h3", "h4", "h5", "h6", "header", "hgroup", "hr",
              "noindex", "noscript",
              "ol", "output", "p", "pre",
              "section",
              "table", "tbody", "tfoot", "thead", "tr",
              "ul",
              "video"];
INLINE_TAGS = ["a", "abbr", "acronym",
               "b", "bdo", "big", "button",
               "cite", "code",
               "dfn",
               "em",
               "font",
               "i", "iframe", "img", "input",
               "kbd",
               "label", "li",
               "map",
               "nobr",
               "object",
               "q",
               "s", "samp", "select", "small", "strong", "span", "sub", "sup",
               "textarea", "td", "th", "time", "tt",
               "u",
               "var"];
IGNORE_EMPTY = ["a", "abbr", "acronym",
                "b", "bdo", "big",
                "cite", "code",
                "dfn",
                "em",
                "font",
                "i",
                "kbd",
                "label", "li",
                "map",
                "nobr",
                "q",
                "s", "samp", "small", "strong", "span", "sub", "sup",
                "td", "th", "time", "tt",
                "u",
                "var"];
NONTRIVIA = ["p", "table", "ul", "ol", "dl", "form",
             "h1", "h2", "h3", "h4", "h5", "h6",
             "video", "object"];

TreeProcessing = {
    description: function (elem, topNode) {
        if (elem[0].nodeType == Node.TEXT_NODE) {
            return (topNode? jQuery.trim(elem[0].nodeValue).substr(0, MAX_TEXT_LENGTH) + "…": elem[0].nodeValue);
        }
        var elemContent = elem.contents().filter(Filters.nonEmptyTextOrBigElement);
        var text = "";
        for (var i = 0; i < elemContent.length; ++i) {
            if (elemContent[i].nodeType == Node.TEXT_NODE) {
                text = elemContent[i].nodeValue;
            } else {
                var child = $(elemContent[i]);
                var tagName = child.prop("tagName").toLowerCase();
                if (
                    BLOCK_TAGS.indexOf(tagName) >= 0 ||
                    child.find(BLOCK_TAGS.join(",")).filter(Filters.nonEmptyChild).length
                ) {
                    text = TreeProcessing.description(child, false);
                } else {
                    switch (tagName) {
                        case "iframe":
                            text = "&lt;iframe&gt;";
                            break;
                        case "select":
                            var options = child.find("option").filter(
                                function () {
                                    return $(this).prop("selected");
                                }
                            );
                            if (options.length) {
                                text = $(options[0]).text();
                                break;
                            }
                        default:
                            text = TreeProcessing.description(child, false);
                    }
                }
            }
            if (text != "") {
                break;
            }
        }
        var descr = "";
        if (text == "" && elem.find("img").length) {
            descr = "<img src=\"" + elem.find("img").attr("src") + "\" style=\"max-width: 200px;\" />";
        } else {
            var imgDescr = $();
            try {
                imgDescr = $(text);
            } catch (e) {
            }
            if (!(imgDescr.length && imgDescr.prop("tagName") && imgDescr.prop("tagName").toLowerCase() == "img")) {
                descr = text.substr(0, MAX_TEXT_LENGTH);
                if (descr != text) {
                    descr += "…";
                }
            } else {
                descr = text;
            }
        }
        return descr;
    },
    isLinearSubset: function (set, superset) {
        if (!superset.length) {
            return (set.length == 0);
        }
        var k = 0;
        for (var i = 0; i < set.length; ++i) {
            while (set[i] != superset[k]) {
                ++k;
                if (k == superset.length) {
                    return false;
                }
            }
        }
        return true;
    },
    inlineContent: function (elem) {
        var guid = elem.attr("data-guid");
        if (typeof guid == "undefined" || typeof Globals.inlineContents[guid] == "undefined") {
            var result = $();
            if (!elem.prop("tagName") || elem.prop("tagName").toLowerCase() != "iframe") {
                var result = elem.contents().filter(Filters.nonEmptyText).add(
                    elem.find(INLINE_TAGS.join(",")).filter(
                        function () {
                            var jq = $(this);
                            return TreeProcessing.visibleAndBig(jq) &&
                                   !jq.find(BLOCK_TAGS.join(",")).filter(Filters.nonEmptyChild).length && !(
                                       IGNORE_EMPTY.indexOf(jq.prop("tagName").toLowerCase()) >= 0 &&
                                       !TreeProcessing.inlineContent(jq).length
                                   );
                        }
                    )
                );
            }
            if (typeof guid == "undefined") {
                return result;
            } else {
                Globals.inlineContents[guid] = result;
            }
        }
        return Globals.inlineContents[guid];
    },
    equivalent: function (elem) {
        while (1) {
            var elemContent = TreeProcessing.inlineContent(elem);
            var children = elem.children().filter(
                function () {
                    var tagName = $(this).prop("tagName").toLowerCase();
                    if (
                        TreeProcessing.visibleAndBig($(this)) &&
                        TreeProcessing.isLinearSubset(elemContent, TreeProcessing.inlineContent($(this))) &&
                        (BLOCK_TAGS.indexOf(tagName) >= 0 ||
                         $(this).find(BLOCK_TAGS.join(",")).filter(Filters.nonEmptyChild).length)
                    ) {
                        var otherChildren = elem.children().filter(
                            function () {
                                return TreeProcessing.visibleAndBig($(this));
                            }
                        );
                        for (var i = 0; i < otherChildren.length; ++i) {
                            if (otherChildren[i] != this) {
                                var otherChild = $(otherChildren[i]);
                                var text;
                                if (
                                    TreeProcessing.inlineContent(otherChild).length ||
                                    jQuery.trim(otherChild.find("*").filter(
                                        function () {
                                            var jq = $(this);
                                            return jq.contents().filter(Filters.nonEmptyText).length &&
                                                   TreeProcessing.visibleAndBig(jq);
                                        }
                                    ).text()) != ""
                                ) {
                                    return false;
                                };
                            }
                        }
                        return true;
                    } else {
                        return false;
                    }
                }
            );
            if (children.length) {
                elem = $(children[0]);
            } else {
                return elem;
            }
        }
    },
    buildMarkedChildren: function (elem) {
        var guid = elem.attr("data-guid");
        var successorGuid = Globals.successors[guid];
        Globals.markedChildren[guid] = [];
        var childrenWithText = elem.contents();
        var pushedChildren = [];
        var jointBlock = $();
        var undefinedSuccessors = 0;
        for (var i = 0; i < childrenWithText.length; ++i) {
            var child = $(childrenWithText[i]);
            if (child[0].nodeType != Node.TEXT_NODE && child[0].nodeType != Node.ELEMENT_NODE) {
                continue;
            }
            if (
                child[0].nodeType == Node.TEXT_NODE || (
                    !((BLOCK_TAGS.indexOf(child.prop("tagName").toLowerCase()) >= 0 ||
                       child.find(BLOCK_TAGS.join(",")).filter(Filters.nonEmptyChild).length) &&
                      TreeProcessing.visibleAndBig(child))
                )
            ) {
                if (
                    (child[0].nodeType == Node.TEXT_NODE && jQuery.trim(child[0].nodeValue) != "") ||
                    (child[0].nodeType == Node.ELEMENT_NODE && TreeProcessing.visibleAndBig(child) && !(
                        IGNORE_EMPTY.indexOf(child.prop("tagName").toLowerCase()) >= 0 &&
                        TreeProcessing.inlineContent(child).length == 0
                    ))
                ) {
                    if (jointBlock.length) {
                        var copy = false;
                        for (var j = 0; j <= i; ++j) {
                            if (childrenWithText[j] == jointBlock[jointBlock.length - 1]) {
                                copy = true;
                            } else {
                                if (copy) {
                                    jointBlock = jointBlock.add($(childrenWithText[j]));
                                }
                            }
                        }
                    } else {
                        jointBlock = child;
                    }
                }
            } else {
                if (jointBlock.length) {
                    Globals.markedChildren[guid].push(jointBlock);
                    TreeProcessing.defineSuccessors(guid, pushedChildren, undefinedSuccessors);
                    undefinedSuccessors = pushedChildren.length;
                }
                var childEq = TreeProcessing.equivalent(child);
                if (TreeProcessing.isSplittable(childEq)) {
                    pushedChildren.push(childEq);
                } else {
                    var childContent = TreeProcessing.inlineContent(childEq);
                    // don't mark empty blocks automatically
                    if (childContent.length) {
                        Globals.markedChildren[guid].push(childEq);
                        TreeProcessing.defineSuccessors(guid, pushedChildren, undefinedSuccessors);
                        undefinedSuccessors = pushedChildren.length;
                    }
                }
                jointBlock = $();
            }
        }
        if (jointBlock.length) {
            Globals.markedChildren[guid].push(jointBlock);
            TreeProcessing.defineSuccessors(guid, pushedChildren, undefinedSuccessors);
            undefinedSuccessors = pushedChildren.length;
        } else {
            while (undefinedSuccessors < pushedChildren.length) {
                Globals.successors[$(pushedChildren[undefinedSuccessors++]).attr("data-guid")] = successorGuid;
            }
        }
        for (var i = pushedChildren.length - 1; i >= 0; --i) {
            Globals.stack.push(pushedChildren[i]);
        }
    },
    defineSuccessors: function(guid, pushedChildren, undefinedSuccessors) {
        for (var i = undefinedSuccessors; i < pushedChildren.length; ++i) {
            Globals.successors[$(pushedChildren[i]).attr("data-guid")] = guid + "_" + (Globals.markedChildren[guid].length - 1);
        }
    },
    isSplittable: function (elem) {
        return (
            elem.children().filter(Filters.nonEmptyRecursivelyBlockChild).length != 0
        );
    },
    visibleAndBig: function (elem) {
        var guid = elem.attr("data-guid");
        if (typeof guid == "undefined" || typeof Globals.visibleFlags[guid] == "undefined") {
            var elOff = elem.offset();
            var elWidth = elem.width();
            var elHeight = elem.height();
            var brComp = 0;
            var tagName = elem.prop("tagName").toLowerCase();
            if (
                navigator.userAgent.indexOf("PhantomJS") >= 0 &&
                tagName == "img" &&
                elem[0].naturalWidth == 0 &&
                elWidth == 4 &&
                elHeight == 4
            ) {
                elWidth = 24;
                elHeight = 24;
            }
            if (
                navigator.userAgent.indexOf("PhantomJS") >= 0 &&
                tagName == "br" &&
                elHeight == 0 &&
                elem.css("line-height").indexOf("px") > 0
            ) {
                elHeight = parseInt(elem.css("lineHeight").replace("px", ""));
                brComp = 1;
            }
            if (
                navigator.userAgent.indexOf("PhantomJS") >= 0 &&
                tagName == "object" &&
                (elHeight < elem[0].height || elWidth < elem[0].width) &&
                elem.parent().children().length != 1
            ) {
                elWidth = 0;
                elHeight = 0;
            }
            var imgs = elem.find("img").filter(
                function () {
                    return TreeProcessing.visibleAndBig($(this));
                }
            ).length;
            if (
                elWidth == 0 && elHeight == 0 && navigator.userAgent.indexOf("PhantomJS") >= 0 && (
                    tagName == "noindex" ||
                    elem.css("display") == "inline"
                ) && tagName != "li"
            ) {
                var blockChildren = elem.find(BLOCK_TAGS.join(",")).filter(Filters.nonEmptyChild);
                if (!imgs) {
                    imgs = blockChildren.length;
                }
                if (blockChildren.length) {
                    elWidth = $(blockChildren[0]).width();
                    elHeight = $(blockChildren[0]).height();
                }
            }
            var match = elem.text().match(LETTER_RE);
            var ic = TreeProcessing.inlineContent(elem);
            if (
                navigator.userAgent.indexOf("PhantomJS") >= 0 &&
                ["noindex", "h1", "h2", "h3", "h4", "h5", "h6"].indexOf(tagName) >= 0 &&
                elWidth == 0 &&
                elHeight == 0 &&
                match &&
                ic.length
            ) {
                try {
                    elWidth = $(ic[0]).width();
                } catch (ex) {
                }
                brComp = 1;
            }
            var res =  elem.css("display") != "none" &&
                       (elem.is(":visible") || brComp || imgs) &&
                       elem.css("visibility") != "hidden" &&
                       (elWidth > MIN_SIZE_TO_CHOOSE || elHeight > MIN_SIZE_TO_CHOOSE ||
                       match || imgs) &&
                       elOff.left + elWidth >= 0 &&
                       elOff.left <= SiteForSegmentation.width &&
                       elOff.top + elHeight >= 0 &&
                       elOff.top <= SiteForSegmentation.height;
            if (typeof(guid) == "undefined") {
                return res;
            } else {
                Globals.visibleFlags[guid] = res;
            }
        }
        return Globals.visibleFlags[guid];
    },
    partOfListOrTable: function (elem) {
        if (elem.filter(Filters.nonEmptyText).length) {
            return false;
        }
        elem = elem.filter(Filters.element);
        var tagNames = ["tr", "td", "th", "li"];
        for (var i = 0; i < tagNames.length; ++i) {
            if (elem.length == elem.filter(tagNames[i]).length) {
                return true;
            }
        }
        return false;
    },
    putGuidsToTBody: function () {
        var fictives = $("#html_page").contents().find("tbody:not([data-guid])");
        for (var i = 0; i < fictives.length; ++i) {
            var parentWithGuid = $($(fictives[i]).parents("[data-guid]")[0]);
            var tagName = parentWithGuid.prop("tagName").toLowerCase();
            if (tagName == "table" && (
                parentWithGuid.children("thead").length || parentWithGuid.children("caption").length
            )) {
                $(fictives[i]).before($(fictives[i]).contents());
                $(fictives[i]).remove();
            } else {
                if (
                    tagName == "table" && parentWithGuid.children("tbody").length > 1 &&
                    parentWithGuid.children("tbody").filter("[data-guid]").length == 1
                ) {
                    var tbody = parentWithGuid.children("tbody").filter("[data-guid]");
                    if (tbody.prevAll().index($(fictives[i])) >= 0) {
                        tbody.prepend($(fictives[i]).contents());
                    } else {
                        tbody.append($(fictives[i]).contents());
                    }
                    $(fictives[i]).remove();
                    continue;
                }
                var guid = parentWithGuid.attr("data-guid");
                $(fictives[i]).attr("data-guid", guid);
                // will never be chosen because TBody is its equivalent.
                // we need guid to optimize inlineContent calculations
                parentWithGuid.attr("data-guid", guid + "_old");
            }
        }
    },
    processTagsWithoutGuid: function () {
        var tags = $("#html_page").contents().find("body :not([data-guid])").filter(Filters.nonEmptyWithoutGuid);
        while (tags.length) {
            var tag = $(tags[0]);
            tag.before(tag.contents());
            tag.remove();
            tags = $("#html_page").contents().find("body :not([data-guid])").filter(Filters.nonEmptyWithoutGuid);
        }
    },
};
