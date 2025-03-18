HEADER_HEIGTH = 30;
MAX_TEXT_LENGTH = 150;
MIN_SIZE_TO_CHOOSE = 20;
SCROLL_DELTA = 20;
VIEW_CONTENT_HEIGHT = 440;
VIEW_CONTENT_MARGIN = 10;
LETTER_RE = new RegExp("[0-9a-z\\u00C0-\\u1FFF\\u2C00-\\uD7FF]", "ig");

SiteForSegmentation = {
//public:
    maxZIndex: 0,
    iframeOff: {},
    width: 0,
    height: 0,

    init: function () {
        SiteForSegmentation.preparePage();
        SiteForSegmentation.recomputeSize();
        SiteForSegmentation.iframeOff = $("#html_page").offset();
        SiteForSegmentation.maxZIndex = SiteForSegmentation.getMaxZIndex();
    },
    recomputeSize: function () {
        $("#html_page").height($(window).height() - 2 * HEADER_HEIGTH);
        var contents = $("#html_page").contents();
        var body = contents.find("body");
        SiteForSegmentation.width = Math.max(contents.width(), body.width());
        SiteForSegmentation.height = Math.max(contents.height(), body.height());
    },
//private:
    preparePage: function () {
        $("#html_page").contents().find("noscript").remove();
        $("#html_page").contents().find("[href]").attr("target", "_blank");
        var src = $("#html_page").attr("src");
        var params = src.split(/[?&]/g);
        params.shift();
        for (var i = 0; i < params.length; ++i) {
            var tokens = params[i].split("=");
            if (tokens[0] == "url") {
                src = decodeURIComponent(tokens[1]);
                break;
            }
        }
        $("#html_page").contents().find("head").prepend("<base href=\"" + src + "\" />");
    },
    getMaxZIndex: function () {
        var allChildren = $("#html_page").contents().find("body").find("*");
        var maxZIndex = 0;
        for (var i = 0; i < allChildren.length; ++i) {
            var zIndex = parseInt($(allChildren[i]).css("zIndex"));
            if (zIndex > maxZIndex) {
                maxZIndex = zIndex;
            }
        }
        return maxZIndex;
    },
};

Globals = {
    stack: [],
    markedChildren: [],
    splitResult: [],
    parents: [],
    successors: [],
    trashDivs: [],
    inlineContents: [],
    visibleFlags: [],
    chosenCount: 0,
    chosen: [],
    merges: [],
    annotations: [],
    CHOSEN_FIELDS: {"elem" : 0, "color": 1, "permitSplit": 2, "permitJoin": 3},

    init: function () {
        Globals.inlineContents = [];
        Globals.visibleFlags = [];
        Globals.clean();
    },
    clean: function () {
        Globals.stack = [];
        Globals.chosen = [];
        Globals.chosenCount = 0;
        Globals.markedChildren = [];
        Globals.splitResult = [];
        Globals.parents = [];
        Globals.successors = [];
        Globals.trashDivs = [];
        Globals.merges = [];
        Globals.annotations = [];
    }
};

function Color(string) {
    string = string || "";
    this.tokens = string.match(/rgb\((\d+), (\d+), (\d+)\)/) || [];
    this.tokens.shift();
    if (this.tokens.length < 3) {
        this.tokens = [
            Math.round(Math.random() * 255),
            Math.round(Math.random() * 255),
            Math.round(Math.random() * 255),
        ];
    }
}

Color.prototype = {
    toString: function () {
        return "rgb(" + this.tokens.join(", ") + ")";
    },
    toRGBA: function (alpha) {
        this.tokens.push(alpha);
        var res = "rgba(" + this.tokens.join(", ") + ")";
        this.tokens.pop();
        return res;
    }
}

CommonHandlers = {
    htmlPageLoad: function () {
        $("#show_form").click(CommonHandlers.showForm);
        $(window).resize(CommonHandlers.resizeMainPage);
        CommonHandlers.hideFooter();
        CommonHandlers.hideNavBar();
        //show_navbar нужен, чтобы при наведении на середину верхней панели также
        //срабатывало событие mouseover, но этот div перекрывает кнопки панели,
        //поэтому прицепиться к событию mouseout show_navbar также не получается.
        $("#show_navbar").mouseover(CommonHandlers.showNavBar);
        $("#html_page").mouseover(CommonHandlers.hideNavBar);
        $("#right_pane").mouseover(CommonHandlers.hideNavBar);
        $("body").css("overflow","hidden");

        SiteForSegmentation.init();

        TreeProcessing.putGuidsToTBody();
        TreeProcessing.processTagsWithoutGuid();
        //TreeProcessing.turnMarqueesToDivs();
        Globals.init();
        stageNamespaces[stage].init();
        CommonHandlers.resizeMainPage();
        $("#split_btn").click(CommonHandlers.makeComputeGuidHandler(EstStage.split));
        $("#join_btn").click(CommonHandlers.makeComputeGuidHandler(EstStage.join));
        $("#viewButton").click(CommonHandlers.makeComputeGuidHandler(EstStage.viewContent));
        var annotButtons = $(".annot_button");
        annotButtons.click(MergeStage.annotate);
    },
    clickColorDiv: function () {
        var guid = GuidProcessing.stripGuid(this.id);
        SelectBlock.selectBlock(guid);
        var mark = $("#html_page").contents().find("#markdiv_" + guid);
        Blinking.blink(mark);
    },
    clickMarkDiv: function () {
        var number = parseInt($(this).text().split(" ")[0]);
        var span = $($("#chosen").children()[number - 1]).find(".number");
        SelectBlock.selectBlock(GuidProcessing.stripGuid(span.parents("div")[0].id));
        Blinking.blink(span);
    },
    doNothing: function (event) {
        event.stopPropagation();
        return false;
    },
    resizeMainPage: function (event) {
        SiteForSegmentation.recomputeSize();
        $("#list_segments").css({
            height: $("#html_page").height() - $("#instrumental_pane").height(),
        });
        var paneWidth = $("#right_pane").width();
        $("#instrumental_pane").css({
            width: paneWidth + "px",
        });
        $("#split_btn").css({
            "width": $("#split_buttons").width(),
        });
        $("#join_btn").css({
            "width": $("#split_buttons").width(),
        });
    },
    hideFooter: function () {
        $(".container-footer").css({
            "visibility" : "hidden",
            "top" : "0px",
        });
    },
    showNavBar: function (event) {
        $(".navbar-static-top").css({
            "margin-top" : "0px",
        });
        $("#show_navbar").css({
            "height" : "0px",
        });
    },
    hideNavBar: function (event) {
        $(".container-padded").css({
            "padding-top" : "0px",
            "margin-top" : "-25px",
        });
        $(".navbar-static-top").css({
            "margin-top" : "-30px",
        });
        $("#show_navbar").css({
            "height" : "10px",
        });
    },
    showForm: function () {
        if ($(this).val() == "Show form") {
            $("#form").css("display", "block");
            $(this).val("Hide form");
        } else {
            $("#form").css("display", "none");
            $(this).val("Show form");
        }
    },
    makeWaitingHandler: function (handler) {
        return function () {
            var button = $(this);
            var value = button.val();
            button.val("...");
            setTimeout(CommonHandlers.makeWorkerFunction(handler, button, value), 100);
        }
    },
    makeEscapeHandler: function (handler) {
        return function (event) {
            if (event.which == 27) {
                return handler();
            }
        }
    },
    makeGuidHandler: function (guid, handler) {
        return function () {
            handler(guid);
        }
    },
    makeComputeGuidHandler: function (handler) {
        return function () {
            var guid = GuidProcessing.stripGuid($(".selected")[0].id);
            handler(guid);
        }
    },
    makeWorkerFunction: function (handler, button, value) {
        return function () {
            handler.call(button[0]);
            button.val(value);
        }
    },
};

ViewContent = {
    show: function () {
        $("#content").css({"width": $("#content").parent().width() - 2 * VIEW_CONTENT_MARGIN + "px",
                           "left": ($("#content").parent().offset().left + VIEW_CONTENT_MARGIN) + "px",
                           "top": ($(window).height() - VIEW_CONTENT_HEIGHT - 4 * VIEW_CONTENT_MARGIN) + "px",
                           "display": "block"});
        $(".view_button").attr("disabled", "disabled");
        $(".view_button").blur();
    },
    close: function () {
        $("#content_container").html("");
        $("#content_container").scrollTop(0);
        $("#content").css("display", "none");
        $(".view_button").removeAttr("disabled");
        return false;
    },
    contentToView: function (elem) {
        var res = $();
        for (var i = 0; i < elem.length; ++i) {
            var child = $(elem[i]);
            if (typeof(child.prop("tagName")) == "undefined" || NONTRIVIA.indexOf(child.prop("tagName").toLowerCase()) >= 0) {
                if (child[0].nodeType != Node.TEXT_NODE || jQuery.trim(child[0].nodeValue) != "") {
                    res = res.add(child);
                }
                continue;
            }
            var blockContent = child.find(BLOCK_TAGS.join(",")).filter(
                Filters.nonEmptyChild
            ).filter(
                Filters.withoutIncludedChildren
            );

            var otherContent = child.find("*").filter(Filters.nonEmptyChild).contents().add(child.contents()).filter(
                Filters.nonEmptyTextOrBigElement
            ).filter(
                Filters.withoutIncludedChildren
            ).filter(
                Filters.makeNotIncludedFilter(blockContent)
            );
            otherContent = otherContent.filter(
                Filters.makeTopLevelFilter(otherContent)
            );

            var added = blockContent.add(otherContent);
            if (added.length) {
                res = res.add(added);
            } else {
                res = res.add(child);
            }
        }
        return res;
    },
    renderViewedContent: function (viewedContent) {
        for (var i = 0; i < viewedContent.length; ++i) {
            var inserted = $(viewedContent[i]).clone();
            inserted.css({"z-index": "1",
                          "position": "relative",
                          "left": "0px",
                          "top": "0px",
                          "color": "#ffffff"});
            inserted.find("*").css("color", "#ffffff");
            if (inserted.prop("tagName") && inserted.prop("tagName").toLowerCase() == "img") {
                inserted.attr("src", inserted.get(0).src);
            }
            inserted.click(CommonHandlers.doNothing);
            inserted.find(":input").add(inserted).attr("disabled", "disabled");
            $("#content_container").append(
                $("<div style=\"border-top: 1px solid #c0c0c0; border-bottom: 1px solid #808080\"></div>").append(
                    inserted
                )
            );
        }
    },
};

GuidProcessing = {
    stripGuid: function (elemId) {
        return elemId.substring(elemId.indexOf("_") + 1);
    },
    splitGuid: function (str) {
        var guid = str.split(" ")[0];
        var guidTokens = guid.split("_");
        guid = guidTokens[0];
        var splitId = "";
        if (guidTokens.length > 1) {
            splitId = guidTokens[1];
        }
        return {
            guid: guid,
            splitId: splitId,
        };
    },
};

ChoicesProcessing = {
    processInsertion: function(guid) {
        var newChoice = $("#chosen_" + guid);
        var number = parseInt(newChoice.next().find(".number").html());
        newChoice.find(".number").html(number);
        $("#html_page").contents().find("#markdiv_" + guid + " div").html(number);
        var allSuccessors = newChoice.nextAll();
        for (var i = 0; i < allSuccessors.length; ++i) {
            var newNumber = number + i + 1;
            $(allSuccessors[i]).find(".number").html(newNumber);
            $("#html_page").contents().find("#markdiv_" + GuidProcessing.stripGuid(allSuccessors[i].id) + " div").html(newNumber);
        }
    },
    makeChoice: function (elem, guid, color) {
        try {
            Highlight.addMark({
                el: elem,
                id: "mark_" + guid,
                html: HtmlBlocks.markHtml(guid, color),
                events: {"click": CommonHandlers.clickMarkDiv}
            });
        } catch(e) {
            if (TreeProcessing.partOfListOrTable(elem)) {
                elem = elem.filter(Filters.element);
                Highlight.addMark({
                    el: elem,
                    id: "mark_" + guid,
                    html: HtmlBlocks.markHtml(guid, color),
                    events: {"click": CommonHandlers.clickMarkDiv}
                });
            } else {
                $(elem[0]).before(HtmlBlocks.divInline());
                var trashDiv = $(elem[0]).prev();
                trashDiv.prepend(elem);
                Globals.trashDivs[guid] = trashDiv;
                Highlight.addMark({
                    el: trashDiv,
                    id: "mark_" + guid,
                    html: HtmlBlocks.markHtml(guid, color),
                    events: {"click": CommonHandlers.clickMarkDiv}
                });
            }
        }
        Globals.chosen[guid] = [elem, color, true, false];
    },
    removeChoice: function (guid) {
        var chosenDivs = $("#chosen").children("div");
        for(var i = 0; i < chosenDivs.length; ++i) {
            if (chosenDivs[i].id.indexOf("chosen_" + guid) == 0) {
                var choiceId = GuidProcessing.stripGuid(chosenDivs[i].id);
                ChoicesProcessing.processDeletion(choiceId);
                Globals.chosen[choiceId] = undefined;
                --Globals.chosenCount;
                if (typeof Globals.trashDivs[choiceId] != "undefined") {
                    var trashDiv = Globals.trashDivs[choiceId];
                    trashDiv.after(trashDiv.html());
                    trashDiv.remove();
                    Globals.trashDivs[choiceId] = undefined;
                }
                Highlight.removeMark("mark_" + choiceId);
                $("#id_choice_" + choiceId).remove();
                $(chosenDivs[i]).remove();
            }
        }
    },
    processDeletion: function (guid) {
        var deletedChoice = $("#chosen_" + guid);
        var number = parseInt(deletedChoice.find(".number").html());
        var allSuccessors = deletedChoice.nextAll();
        for (var i = 0; i < allSuccessors.length; ++i) {
            var newNumber = number + i;
            $(allSuccessors[i]).find(".number").html(newNumber);
            $("#html_page").contents().find("#markdiv_" + GuidProcessing.stripGuid(allSuccessors[i].id) + " div").html(newNumber);
        }
    },
};

Blinking = {
    blink: function (elem) {
        for(var i = 0; i < 3; ++i) {
            elem.fadeTo("slow", 0).fadeTo("slow", 1.0);
        }
    },
};

EstStage = {
    init: function () {
        $(".close").click(ViewContent.close);
        $(document).keydown(CommonHandlers.makeEscapeHandler(ViewContent.close));
        Globals.clean();
        var body = $("#html_page").contents().find("body");
        var bodyEq = TreeProcessing.equivalent(body);
        EstStage.addBlock(bodyEq, bodyEq.attr("data-guid"), 0);
        MergeStage.renderMerge(0, 1);
        if (TreeProcessing.isSplittable(bodyEq)) {
            EstStage.permitSplit(bodyEq.attr("data-guid"));
        }
        var initChoice = $("[name=\"choice\"]");
        if (initChoice.length) {
            var ic = [];
            for (var i = 0; i < initChoice.length; ++i) {
                ic.push(initChoice[i].value);
            }
            EstStage.renderInitChoice(ic);
            initChoice.remove();
        } else {
            if (choices.length) {
                EstStage.renderInitChoice(choices);
            }
        }
        var initMerge = $("[name=\"merge\"]");
        if (initMerge.length) {
            var im = [];
            for (var i = 0; i < initMerge.length; ++i) {
                im.push(initMerge[i].value);
            }
            EstStage.renderInitMerge(im);
            initMerge.remove();
        } else {
            if (merged.length) {
                EstStage.renderInitMerge(merged);
            }
        }
        SelectBlock.selectBlock(GuidProcessing.stripGuid($("#chosen").children()[0].id));
        $(document).keydown(EstStage.keyHandler);
        $("#submit").click(EstStage.submit);
    },
    renderInitChoice: function (initChoice) {
        var icIdx = 0;
        var chosenIdx = 0;
        var removedCount = 0;
        while (icIdx < initChoice.length) {
            var chosenDivs = $("#chosen").children();
            if (chosenIdx >= chosenDivs.length) {
                EstStage.removeFromInitMerge(icIdx - removedCount);
                ++removedCount;
                ++icIdx;
                continue;
            }
            var curGuid = GuidProcessing.stripGuid(chosenDivs[chosenIdx].id);
            while (curGuid == initChoice[icIdx]) {
                ++chosenIdx;
                ++icIdx;
                if (icIdx == initChoice.length) {
                    return;
                }
                if (chosenIdx >= chosenDivs.length) {
                    EstStage.removeFromInitMerge(icIdx - removedCount);
                    ++removedCount;
                    ++icIdx;
                    break;
                }
                curGuid = GuidProcessing.stripGuid(chosenDivs[chosenIdx].id);
            }
            if (chosenIdx >= chosenDivs.length) {
                continue;
            }
            if (icIdx < initChoice.length) {
                if (curGuid.indexOf("_") == -1) {
                    EstStage.split(curGuid);
                } else {
                    if (initChoice[icIdx].indexOf("_") > 0) {
                        var icMainGuid = initChoice[icIdx].split("_")[0];
                        var chMainGuid = curGuid.split("_")[0];
                        if (parseInt(icMainGuid) > parseInt(chMainGuid)) {
                            if ($("#html_page").contents().find("[data-guid=\"" +
                                                                chMainGuid +
                                                                "\"]").find("[data-guid=\"" + icMainGuid + "\"]").length) {
                                if (initChoice.indexOf(curGuid) > icIdx) {
                                    EstStage.removeFromInitMerge(icIdx - removedCount);
                                    ++removedCount;
                                    ++icIdx;
                                } else {
                                    EstStage.undefineInitMerge(icIdx - removedCount);
                                    ++chosenIdx;
                                    ++icIdx;
                                }
                            } else {
                                --removedCount;
                                EstStage.insertToInitMerge(icIdx - removedCount);
                                ++chosenIdx;
                            }
                        } else {
                            EstStage.removeFromInitMerge(icIdx - removedCount);
                            ++removedCount;
                            ++icIdx;
                        }
                    } else {
                        EstStage.undefineInitMerge(icIdx - removedCount);
                        ++chosenIdx;
                        ++icIdx;
                    }
                }
            }
        }
    },
    renderInitMerge: function (initMerge) {
        var chosenDivs = $("#chosen").children();
        for (var i = 0; i < initMerge.length; ++i) {
            var spl = initMerge[i].split(",");
            var beginning = parseInt(spl[0]);
            var end = parseInt(spl[1]);
            for (var j = beginning + 1; j < end; ++j) {
                EstStage.merge(GuidProcessing.stripGuid(chosenDivs[j].id));
            }
            if (spl.length > 2) {
                for (var j = beginning; j < end; ++j) {
                    var guid = GuidProcessing.stripGuid(chosenDivs[j].id);
                    MergeStage.markAnnotated(Globals.merges[j], Globals.merges[j], "(" + spl[2] + ")");
                }
                Globals.annotations[beginning] = spl[2];
            }
        }
    },
    undefineInitMerge: function (idx) {
        for (var i = 0; i < merged.length; ++i) {
            var spl = merged[i].split(",");
            var beginning = parseInt(spl[0]);
            var end = parseInt(spl[1]);
            if (beginning <= idx && end > idx) {
                var ranges = [[beginning, idx], [idx, idx + 1], [idx + 1, end]];
                for (var j = ranges.length - 1; j >= 0; --j) {
                    if (ranges[j][1] - ranges[j][0]) {
                        merged.splice(i + 1, 0, ranges[j].join(","));
                    }
                }
                merged.splice(i, 1);
                break;
            }
        }
    },
    removeFromInitMerge: function (idx) {
        EstStage.undefineInitMerge(idx);
        var delIdx;
        for (var i = 0; i < merged.length; ++i) {
            var spl = merged[i].split(",");
            var beginning = parseInt(spl[0]);
            var end = parseInt(spl[1]);
            if (beginning == idx) {
                delIdx = i;
                continue;
            }
            if (beginning > idx) {
                --beginning;
            }
            if (end > idx) {
                --end;
            }
            merged[i] = [beginning, end].join(",");
            if (spl.length > 2) {
                merged[i] += "," + spl[2];
            }
        }
        merged.splice(delIdx, 1);
    },
    insertToInitMerge: function (idx) {
        var insIdx;
        for (var i = 0; i < merged.length; ++i) {
            var spl = merged[i].split(",");
            var beginning = parseInt(spl[0]);
            var end = parseInt(spl[1]);
            if (beginning <= idx && end > idx) {
                if (end - beginning > 1) {
                    var ranges = [[idx, idx + 1], [idx + 1, end]];
                    for (var j = ranges.length - 1; j >= 0; --j) {
                        if (ranges[j][1] - ranges[j][0]) {
                            merged.splice(i + 1, 0, ranges[j].join(","));
                        }
                    }
                    if (idx > beginning) {
                        merged.splice(i, 0, [beginning, idx].join(","));
                        ++i;
                    }
                } else {
                    merged.splice(i + 1, 0, [beginning, end].join(","));
                }
                insIdx = i + 1;
                break;
            }
        }
        for (var i = insIdx + 1; i < merged.length; ++i) {
            var spl = merged[i].split(",");
            var newBeginning = parseInt(spl[0]) + 1;
            var newEnd = parseInt(spl[1]) + 1;
            merged[i] = [newBeginning, newEnd].join(",");
            if (spl.length > 2) {
                merged[i] += "," + spl[2];
            }
        }
    },
    submit: function () {
        var chosenDivs = $("#chosen").children();
        for (var i = 0; i < chosenDivs.length; ++i) {
            var guid = GuidProcessing.stripGuid(chosenDivs[i].id);
            $("#form").append(HtmlBlocks.submitInput(guid));
        }
        var beginning = 0;
        var end = 0;
        while (end < Globals.chosenCount) {
            while (Globals.merges[beginning] == Globals.merges[end]) {
                ++end;
            }
            var segType = Globals.annotations[beginning];
            if (typeof(segType) == "undefined" && $("#id_skipped").val() == "False") {
                $("[name=\"choice\"]").remove();
                $("[name=\"merge\"]").remove();
                alert(ANNOTATION_ERROR);
                SelectBlock.selectBlock(GuidProcessing.stripGuid(chosenDivs[beginning].id));
                Blinking.blinkMainPageElement($("#selector_" + beginning));
                return false;
            }
            $("#form").append(HtmlBlocks.segTypeInput(beginning, end, segType));
            beginning = end;
        }
    },
    keyHandler: function (event) {
        if (event.ctrlKey || event.altKey || event.shiftKey || event.target.tagName.toLowerCase() == "textarea") {
            return;
        }
        var chosenDivs = $("#chosen").children();
        var idx = chosenDivs.index($(".selected"));
        switch (event.which) {
            case 40:    // down arrow
                if (idx < chosenDivs.length - 1) {
                    var nextGuid = GuidProcessing.stripGuid(chosenDivs[idx + 1].id);
                    SelectBlock.selectBlock(nextGuid);
                    var mark = $("#html_page").contents().find("#markdiv_" + nextGuid);
                    Blinking.blink(mark);
                }
                return false;
            case 38:    // up error
                if (idx > 0) {
                    var prevGuid = GuidProcessing.stripGuid(chosenDivs[idx - 1].id);
                    SelectBlock.selectBlock(prevGuid);
                    var mark = $("#html_page").contents().find("#markdiv_" + prevGuid);
                    Blinking.blink(mark);
                }
                return false;
            case 83:    // s
                var button = $(chosenDivs[idx]).find(".split_btn");
                if (button.length) {
                    button.click();
                }
                break;
            case 74:    // j
                var button = $(chosenDivs[idx]).find(".join_btn");
                if (button.length) {
                    button.click();
                }
                break;
            default:
                for (var keyCode in HOTKEYS) {
                    if (event.which == keyCode) {
                        $("#annot_" + HOTKEYS[keyCode] + "_" + Globals.merges[idx]).click();
                        return false;
                    }
                }
        }
    },
    addBlock: function (elem, guid, idx) {
        var color = new Color();
        ChoicesProcessing.makeChoice(elem, guid, color);
        var chosenDivs = $("#chosen").children();
        if (chosenDivs.length > idx) {
            $(chosenDivs[idx]).before(HtmlBlocks.insertedBlock(guid, elem));
            ChoicesProcessing.processInsertion(guid);
        } else {
            $("#chosen").append(HtmlBlocks.insertedBlock(guid, elem));
            idx = chosenDivs.length;
        }
        $("#chosen_" + guid).click(CommonHandlers.clickColorDiv);
        ++Globals.chosenCount;
        var chosenDiv = $("#chosen_" + guid);
        if (idx) {
            chosenDiv.prepend(HtmlBlocks.mergeButton());
            chosenDiv.find(".merge_btn").click(CommonHandlers.makeGuidHandler(guid, EstStage.merge));
        }
    },
    split: function (guid) {
        var elem = Globals.chosen[guid][Globals.CHOSEN_FIELDS["elem"]];
        var idx = $("#chosen").children().index($("#chosen_" + guid));
        MergeStage.clearSelector(idx);

        ChoicesProcessing.removeChoice(guid);

        var parentGuid = Globals.parents[guid];
        EstStage.forbidJoin(parentGuid);

        if (typeof(Globals.splitResult[guid]) == "undefined") {
            EstStage.buildSplitResult(elem);
        } else {
            EstStage.renewSplitResult(guid);
        }
        var splitResult = Globals.splitResult[guid];
        var newBlockIndex = idx;
        for (var i = 0; i < splitResult.length; ++i) {
            var childGuid = splitResult[i][0];
            var child;
            if (childGuid.indexOf("_") >= 0) {
                child = Globals.markedChildren[guid][parseInt(childGuid.split("_")[1])];
            } else {
                child = $("#html_page").contents().find("[data-guid=\"" + childGuid + "\"]");
            }
            EstStage.addBlock(child, childGuid, newBlockIndex++);
        }
        EstStage.ajustMerge(idx + 1, splitResult.length - 1);
        MergeStage.renderMerge(idx, idx + 1);
        for (var i = 0; i < splitResult.length; ++i) {
            EstStage.permitSplit(splitResult[i][0]);
        }
        EstStage.permitJoin(guid);
        SelectBlock.selectBlock(splitResult[0][0]);
    },
    buildSplitResult: function (elem) {
        var guid = elem.attr("data-guid");
        Globals.splitResult[guid] = [];
        TreeProcessing.buildMarkedChildren(elem);
        var res = [];
        for (var i = 0; i < Globals.markedChildren[guid].length; ++i) {
            res.push(guid + "_" + i);
        }
        while (Globals.stack.length) {
            var childGuid = Globals.stack.pop().attr("data-guid");
            var chSucc = Globals.successors[childGuid];
            var placed = false;
            if (chSucc) {
                var succId = res.indexOf(chSucc);
                if (succId >= 0) {
                    res.splice(succId, 0, childGuid);
                    placed = true;
                }
            }
            if (!placed) {
                res.push(childGuid);
            }
        }
        for (var i = 0; i < res.length; ++i) {
            Globals.splitResult[guid].push([res[i]]);
            Globals.parents[res[i]] = guid;
        }
    },
    renewSplitResult: function (guid) {
        var res = [];
        for (var i = 0; i < Globals.splitResult[guid].length; ++i) {
            for (var j = 0; j < Globals.splitResult[guid][i].length; ++j) {
                res.push([Globals.splitResult[guid][i][j]]);
            }
        }
        Globals.splitResult[guid] = res;
    },
    join: function (guid) {
        var parentGuid = Globals.parents[guid];
        var parentElem = $("#html_page").contents().find("[data-guid=\"" + parentGuid + "\"]");
        var idx = $("#chosen").children().index($("#chosen_" + Globals.splitResult[parentGuid][0][0]));
        MergeStage.clearSelector(Globals.merges[idx]);
        var splLength = 0;
        for (var i = 0; i < Globals.splitResult[parentGuid].length; ++i) {
            var splChunk = Globals.splitResult[parentGuid][i];
            splLength += splChunk.length;
            for (var j = 0; j < splChunk.length; ++j) {
                ChoicesProcessing.removeChoice(splChunk[j]);
            }
        }
        EstStage.addBlock(parentElem, parentGuid, idx);
        EstStage.ajustMerge(idx + 1, 1 - splLength);
        MergeStage.renderMerge(idx, idx + 1);
        EstStage.permitSplit(parentGuid);
        EstStage.permitJoin(Globals.parents[parentGuid]);

        SelectBlock.selectBlock(parentGuid);
    },
    forbidSplit: function (guid) {
        $("#chosen_" + guid).find(".split_btn").parent().remove();
        Globals.chosen[guid][Globals.CHOSEN_FIELDS["permitSplit"]] = false;
    },
    //название скорее - определить можно ли разрешать
    permitSplit: function (guid) {
        if (guid.indexOf("_") >= 0) {
            //для корневых неделимых элементов
            EstStage.forbidSplit(guid);
            return;
        }
        var chosenDivs = $("#chosen").children();
        var chosenDiv = $("#chosen_" + guid);
        var idx = chosenDivs.index(chosenDiv);
        if (Globals.merges[idx] != idx) {
            EstStage.forbidSplit(guid);
            return;
        }
        if (idx + 1 < Globals.chosenCount && Globals.merges[idx + 1] != idx + 1) {
            EstStage.forbidSplit(guid);
            return;
        }
        Globals.chosen[guid][Globals.CHOSEN_FIELDS["permitSplit"]] = true;
    },
    forbidJoin: function (guid) {
        if (typeof(guid) == "undefined") {
            return;
        }
        var splitResult = Globals.splitResult[guid];
        if (typeof(splitResult) == "undefined") {
            return;
        }
        for (var i = 0; i < splitResult.length; ++i) {
            for (var j = 0; j < splitResult[i].length; ++j) {
                $("#chosen_" + splitResult[i][j]).find(".join_btn").remove();
                if (Globals.chosen[splitResult[i][j]]) {
                    Globals.chosen[splitResult[i][j]][Globals.CHOSEN_FIELDS["permitJoin"]] = false;
                }
            }
        }
    },
    permitJoin: function (guid) {
        if (typeof(guid) == "undefined") {
            return;
        }
        var chosenDivs = $("#chosen").children();
        var siblings = Globals.splitResult[guid];
        var firstSiblingIdx = chosenDivs.index($("#chosen_" + siblings[0][0]));
        if (firstSiblingIdx == -1 || Globals.merges[firstSiblingIdx] != firstSiblingIdx) {
            return;
        }
        var lastSiblingSegment = siblings[siblings.length - 1];
        var lastSiblingIdx = chosenDivs.index($("#chosen_" + lastSiblingSegment[lastSiblingSegment.length - 1]));
        if (lastSiblingIdx == -1 || (
            Globals.chosenCount > lastSiblingIdx + 1 && Globals.merges[lastSiblingIdx + 1] != lastSiblingIdx + 1
        )) {
            return;
        }
        for (var i = 0; i < siblings.length; ++i) {
            for (var j = 0; j < siblings[i].length; ++j) {
                if (chosenDivs.index($("#chosen_" + siblings[i][j])) == -1) {
                    if (Globals.chosen[guid])
                        Globals.chosen[guid][Globals.CHOSEN_FIELDS["permitJoin"]] = false;
                    return;
                }
            }
        }
        for (var i = 0; i < siblings.length; ++i) {
            for (var j = 0; j < siblings[i].length; ++j) {
                var chosenDiv = $("#chosen_" + siblings[i][j]);
                Globals.chosen[siblings[i][j]][Globals.CHOSEN_FIELDS["permitJoin"]] = true;
            }
        }
    },
    merge: function (guid) {
        var chosenDivs = $("#chosen").children();
        var idx = chosenDivs.index($("#chosen_" + guid));
        MergeStage.clearSelector(Globals.merges[idx]);
        EstStage.forbidSplit(guid);

        var prevGuid = GuidProcessing.stripGuid(chosenDivs[idx - 1].id);
        MergeStage.clearSelector(Globals.merges[idx - 1]);
        EstStage.forbidSplit(prevGuid);

        if (Globals.parents[guid] != Globals.parents[prevGuid]) {
            EstStage.forbidJoin(Globals.parents[guid]);
            EstStage.forbidJoin(Globals.parents[prevGuid]);
        } else {
            EstStage.mergeAndJoin(guid, prevGuid);
        }

        if (Globals.chosenCount == chosenDivs.length) {
            var end = idx;
            while (Globals.merges[end] == Globals.merges[idx]) {
                ++end;
            }
            for (var i = idx; i < end; ++i) {
                MergeStage.mergeToPrevious(i);
            }
            var mergeBtn = $(chosenDivs[idx]).find(".merge_btn");
            EstStage.makeUnmergeBtn(guid, mergeBtn);
            $("#selector_" + idx).attr("id", "selector_" + Globals.merges[idx]);
            SelectBlock.selectBlock(guid);
        }
    },
    mergeAndJoin: function (guid, prevGuid) {
        var parentGuid = Globals.parents[guid];
        if (typeof(parentGuid) == "undefined") {
            return;
        }
        var splitResult = Globals.splitResult[parentGuid];
        for (var i = 0; i < splitResult.length; ++i) {
            if (splitResult[i][splitResult[i].length - 1] == prevGuid) {
                for (var j = 0; j < splitResult[i + 1].length; ++j) {
                    splitResult[i].push(splitResult[i + 1][j]);
                }
                splitResult.splice(i + 1, 1);
                break;
            }
        }
        if (splitResult.length == 1) {
            var chosenDivs = $("#chosen").children();
            var beginningIdx = chosenDivs.index($("#chosen_" + splitResult[0][0]));
            var beginningMerged = (Globals.merges[beginningIdx] != beginningIdx);
            var endIdx = chosenDivs.index($("#chosen_" + splitResult[0][splitResult[0].length - 1]));
            var endMerged = (endIdx < chosenDivs.length - 1 && Globals.merges[endIdx + 1] != endIdx + 1);
            EstStage.join(guid);
            if (beginningMerged) {
                EstStage.merge(parentGuid);
            }
            if (endMerged) {
                var nextGuid = GuidProcessing.stripGuid(chosenDivs[endIdx + 1].id);
                EstStage.merge(nextGuid);
            }
        }
    },
    makeMergeBtn: function (guid, btn) {
        btn.val(MERGE_TEXT);
        btn.unbind();
        btn.click(CommonHandlers.makeGuidHandler(guid, EstStage.merge));
    },
    makeUnmergeBtn: function (guid, btn) {
        btn.val(UNMERGE_TEXT);
        btn.unbind();
        btn.click(CommonHandlers.makeGuidHandler(guid, EstStage.unmerge));
    },
    unmerge: function (guid) {
        var chosenDivs = $("#chosen").children();
        var chosenDiv = $("#chosen_" + guid);
        var idx = chosenDivs.index(chosenDiv);
        var oldMerge = Globals.merges[idx];
        MergeStage.clearSelector(oldMerge);
        var end = idx;
        while (Globals.merges[idx] == Globals.merges[end]) {
            ++end;
        }
        Globals.merges[idx] = idx;
        MergeStage.recolor(idx);
        MergeStage.clearSelector(idx);
        var cssVal = "1px solid #808080";
        chosenDiv.css("border-top", cssVal);
        $(chosenDivs[idx - 1]).css("border-bottom", cssVal);
        for (var i = idx + 1; i < end; ++i) {
            MergeStage.mergeToPrevious(i);
        }
        var prevGuid = GuidProcessing.stripGuid(chosenDivs[idx - 1].id);
        EstStage.permitSplit(guid);
        EstStage.permitSplit(prevGuid);
        EstStage.permitJoin(Globals.parents[guid]);
        EstStage.permitJoin(Globals.parents[prevGuid]);
        var btn = chosenDiv.find(".merge_btn");
        EstStage.makeMergeBtn(guid, btn);
        $("#selector_" + oldMerge).attr("id", "selector_" + idx);
        SelectBlock.selectBlock(guid);
    },
    ajustMerge: function (start, delta) {
        var chosenDivs = $("#chosen").children();
        var startRewriting = start;
        if (delta > 0) {
            for (var i = 0; i < delta; ++i) {
                Globals.merges.splice(start + i, 0, start + i);
                Globals.annotations.splice(start + i, 0, undefined);
                MergeStage.renderMerge(start + i, start + i + 1);
            }
            startRewriting += delta;
        } else {
            Globals.merges.splice(start, -delta);
            Globals.annotations.splice(start, -delta);
        }
        //не нашел сценария использования этого for
        for (var i = startRewriting; i < Globals.chosenCount; ++i) {
            if (Globals.merges[i] >= start) {
                if (Globals.merges[i] < start - delta) {
                    Globals.merges[i] = start - 1;
                } else {
                    Globals.merges[i] += delta;
                }
            }
            var segType = Globals.annotations[Globals.merges[i]];
            if (typeof(segType) != "undefined") {
                var guid = GuidProcessing.stripGuid(chosenDivs[i].id);
                MergeStage.markAnnotated(Globals.merges[i], Globals.merges[i], segType);
            }
        }
        Globals.merges = Globals.merges.slice(0, Globals.chosenCount);
    },
    viewContent: function (guid) {
        ViewContent.show();
        var elem = Globals.chosen[guid][Globals.CHOSEN_FIELDS["elem"]];
        ViewContent.renderViewedContent(ViewContent.contentToView(elem));
    },
};

SelectBlock = {
//public:
    selectBlock: function (guid) {
        var currentChoice = $("#chosen .selected");
        SelectBlock.clearSelectionOfBlock(currentChoice);
        var chosenDiv = $("#chosen_" + guid);
        chosenDiv.toggleClass("selected");
        SelectBlock.setColorToChosen(chosenDiv, guid);
        SelectBlock.scrollListSegments(chosenDiv);
        SelectBlock.scrollHtmlPage(guid);
        var chosenDivs = $("#chosen").children();
        SelectBlock.hideAllMergeButtons(chosenDivs);
        SelectBlock.showChosenMergeButtons(chosenDiv, chosenDivs);
        SelectBlock.selectAnnotationButtons(chosenDivs.index(chosenDiv));
        SelectBlock.selectSplitButton(guid);
        SelectBlock.selectJoinButton(guid);
    },
//private:
    hideAllMergeButtons: function (chosenDivs) {
        $(".merge_btn").css({
            "visibility" : "hidden",
        });
        $(chosenDivs).css({
            "margin-bottom" : "0px",
        });
    },
    clearSelectionOfBlock: function (currentChoice) {
        if (currentChoice.length) {
            currentChoice.css("background-color", "transparent");
            var curGuid = GuidProcessing.stripGuid(currentChoice[0].id);
            $("#html_page").contents().find("#markdiv_" + curGuid).css("background-color", "transparent");
            currentChoice.removeClass("selected");
        }
    },
    showChosenMergeButtons: function (chosenDiv, chosenDivs) {
        chosenDiv.find(".merge_btn").css({
            "visibility" : "visible",
        });
        $(chosenDiv).css({
            "margin-bottom" : "10px",
        });
        var curIdx = chosenDivs.index(chosenDiv);
        if (curIdx + 1 < chosenDivs.length) {
            $(chosenDivs[curIdx + 1]).find(".merge_btn").css({
                "visibility" : "visible",
            });
        }
        if (curIdx > 0) {
            $(chosenDivs[curIdx - 1]).css({
                "margin-bottom" : "10px",
            });
        }
    },
    scrollListSegments: function (chosenDiv) {
        var chOff = $("#list_segments").offset();
        $("#list_segments").scrollTop(0);
        var elOff = chosenDiv.offset();
        $("#list_segments").scrollTop(elOff.top - chOff.top - SCROLL_DELTA);
    },
    scrollHtmlPage: function (guid) {
        var off = Globals.chosen[guid][Globals.CHOSEN_FIELDS["elem"]].offset();
        $("#html_page").contents().scrollTop(off.top - SCROLL_DELTA);
        $("#html_page").contents().scrollLeft(off.left - SCROLL_DELTA);
    },
    setColorToChosen: function (chosenDiv, guid) {
        var color = new Color(chosenDiv.children("div").css("background-color"));
        chosenDiv.css("background-color", color.toRGBA(0.2));
        $("#html_page").contents().find("#markdiv_" + guid).css("background-color", color.toRGBA(0.2));
    },
    selectAnnotationButtons: function (idx) {
        var mergeIdx = Globals.merges[idx];
        var segType = Globals.annotations[mergeIdx];
        if (typeof(segType) != "undefined") {
            $("#annot_" + segType).click();
        }
        else {
            MergeStage.clearSelectedAnnotationButton();
        }
    },
    selectSplitButton: function (guid) {
        if (Globals.chosen[guid][Globals.CHOSEN_FIELDS["permitSplit"]]) {
            $("#split_btn").prop("disabled", false);
        }
        else {
            $("#split_btn").prop("disabled", true);
        }
    },
    selectJoinButton: function (guid) {
        if (Globals.chosen[guid][Globals.CHOSEN_FIELDS["permitJoin"]]) {
            $("#join_btn").prop("disabled", false);
        }
        else {
            $("#join_btn").prop("disabled", true);
        }
    },
};

MergeStage = {
    mergeToPrevious: function(index) {
        var chosenDivs = $("#chosen").children();
        Globals.merges[index] = Globals.merges[index - 1];
        MergeStage.clearSelector(index);
        MergeStage.recolor(index);
        var cssVal = "1px dashed #808080";
        $(chosenDivs[index]).css("border-top", cssVal);
        $(chosenDivs[index - 1]).css("border-bottom", cssVal);
    },
    renderMerge: function (beginning, end) {
        var chosenDivs = $("#chosen").children();
        if (beginning >= chosenDivs.length || beginning == end) {
            return;
        }
        var cssVal = "1px solid #808080";
        $(chosenDivs[beginning]).css("border-top", cssVal);
        for (var i = beginning; i < end; ++i) {
            Globals.merges[i] = beginning;
            var guid = GuidProcessing.stripGuid(chosenDivs[i].id);
            $(chosenDivs[i]).css({
                "border-left": cssVal,
                "border-right": cssVal
            });
            MergeStage.recolor(i);
        }
        $(chosenDivs[end - 1]).css("border-bottom", cssVal);
    },
    recolor: function (index) {
        var chosenDivs = $("#chosen").children();
        var guid = GuidProcessing.stripGuid(chosenDivs[index].id);
        var mergeGuid = GuidProcessing.stripGuid(chosenDivs[Globals.merges[index]].id);
        var color = Globals.chosen[mergeGuid][Globals.CHOSEN_FIELDS["color"]];
        $($("#chosen_" + guid).children("div")[0]).css("background-color", color.toString());
        $("#html_page").contents().find("#markdiv_" + guid).css("outline-color", color.toString());
    },
    //Унифицировать название функции: она делает довольно много и связана с функцией annotate
    //возможно, стоит назвать ее  deannotate
    clearSelector: function (index) {
        var chosenDivs = $("#chosen").children();
        Globals.annotations[Globals.merges[index]] = undefined;
        MergeStage.markAnnotated(Globals.merges[index], Globals.merges[index], "");
        MergeStage.clearSelectedAnnotationButton();
    },
    clearSelectedAnnotationButton : function () {
        var selButton = $("#instrumental_pane .selectedAnnotated");
        selButton.removeClass("selectedAnnotated");
        selButton.css("color", "#000000");
        selButton.css("opacity", "0.5");
        selButton.css("background-image", "linear-gradient(to top, " +
                                          selButton.css("background-color") +
                                          ", #d0d0d0)");
    },
    annotate: function () {
        var split = this.id.split("_");
        var segName = split[1];

        var chosenDivs = $("#chosen").children();
        var currentChoice = $("#chosen .selected");
        var segIdx = chosenDivs.index(currentChoice);

        MergeStage.clearSelector(segIdx);
        Globals.annotations[segIdx] = segName;
        MergeStage.markAnnotated(segIdx, Globals.merges[segIdx], "(" + segName + ")");
        var button = $(this);
        button.toggleClass("selectedAnnotated");
        button.css("color", "#ffffff");
        button.css("opacity", "1");
        button.css("background-image", "none");
        return false;
    },
    //очень странные параметры: они разные для вызовов из annotate и clearSelector
    markAnnotated: function (start, end, segMark) {
        var current = start;
        var chosenDivs = $("#chosen").children();
        while (Globals.merges[current] == Globals.merges[end]) {
            var markDiv = $("#html_page").contents().find(
                "#markdiv_" + GuidProcessing.stripGuid(chosenDivs[current].id) + " div"
            );
            if (markDiv.length) {
                markDiv.html(markDiv.html().split(" ")[0] + " " + segMark);
                $("#annotate_mark_" + GuidProcessing.stripGuid(chosenDivs[current].id)).html(segMark);
            }
            current++;
        }
    },
};

stageNamespaces = {
    choice: EstStage,
    //merge: MergeStage,
    view: EstStage,
    //diff: DiffStage,
};

Filters = {
    makeNotIncludedFilter: function (included) {
        return function () {
            var parents = $(this).parents();
            var children = $(this).children();
            for (var i = 0; i < included.length; ++i) {
                if (included[i] == this) {
                    return false;
                }
                for (var j = 0; j < parents.length; ++j) {
                    if (included[i] == parents[j]) {
                        return false;
                    }
                }
                for (var j = 0; j < children.length; ++j) {
                    if (included[i] == children[j]) {
                        return false;
                    }
                }
            }
            return true;
        }
    },
    makeTopLevelFilter: function (included) {
        return function () {
            var parents = $(this).parents();
            for (var i = 0; i < included.length; ++i) {
                for (var j = 0; j < parents.length; ++j) {
                    if (included[i] == parents[j]) {
                        return false;
                    }
                }
            }
            return true;
        }
    },
    element: function () {
        return this.nodeType == Node.ELEMENT_NODE;
    },
    withoutIncludedChildren: function () {
        return $(this).find(BLOCK_TAGS.join(",")).filter(Filters.nonEmptyChild).length == 0;
    },
    nonEmptyChild: function () {
        var jq = $(this);
        return TreeProcessing.visibleAndBig(jq) && TreeProcessing.inlineContent(jq).length;
    },
    nonEmptyRecursivelyBlockChild: function () {
        var jq = $(this);
        var tagName = jq.prop("tagName").toLowerCase();
        return TreeProcessing.visibleAndBig(jq) && (
            (BLOCK_TAGS.indexOf(tagName) >= 0 && TreeProcessing.inlineContent(jq).length) ||
            jq.find(BLOCK_TAGS.join(",")).filter(Filters.nonEmptyChild).length
        );
    },
    nonEmptyText: function () {
        return this.nodeType == Node.TEXT_NODE && jQuery.trim(this.nodeValue) != "";
    },
    nonEmptyTextOrBigElement: function () {
        var jq = $(this);
        var tagName = (jq.prop("tagName")? jq.prop("tagName").toLowerCase(): "");
        return (this.nodeType == Node.TEXT_NODE && jQuery.trim(this.nodeValue) != "") ||
                (this.nodeType == Node.ELEMENT_NODE &&
                 TreeProcessing.visibleAndBig(jq) && (
                     (INLINE_TAGS.indexOf(tagName) >= 0 && IGNORE_EMPTY.indexOf(tagName) == -1) ||
                     TreeProcessing.inlineContent(jq).length
                 ));
    },
    nonEmptyWithoutGuid: function () {
        return (
            $(this).find(BLOCK_TAGS.join(",")).filter(Filters.nonEmptyChild).length ||
            TreeProcessing.inlineContent($(this)).length
        );
    },
};
