phantom.injectJs("jquery-1.9.1.js");
phantom.injectJs("stats.js");

var fs = require("fs");
var wp = require("webpage");
var sys = require("system");

var MNEMONICS_2_CLASSCODES = {
    AUX: 0,
    AAD: 1,
    ACP: 2,
    AIN: 3,
    ASL: 4,
    
    DMD: 10,
    DHC: 11,
    DHA: 12,
    DCT: 13,
    DCM: 14,

    LMN: 20,
    LCN: 21,
    LIN: 22,
};

var processFunction;
var mode = sys.args[1];
var estFilename = "estimations2.txt";
var applyMode = false;
var start = 0;
var end = 1;

var idx = 2;
try {
    if (sys.args[idx] == "apply") {
        applyMode = true;
        ++idx;
    }
    if (sys.args[idx] == "-f") {
        estFilename = sys.args[idx + 1];
        idx += 2;
    }
} catch (ex) {
}
try {
    var start = parseInt(sys.args[idx]) || 0;
    var end = parseInt(sys.args[idx + 1]) || 1;
} catch (ex) {
    console.log("Wrong estimations range!");
    phantom.exit(1);
}
if (end <= start) {
    console.log("Wrong estimations range!");
    phantom.exit(1);
}

if (typeof(mode) == "undefined" || mode == "basic") {
    processFunction = function (extracted, factors, estJSON, url) {
        for (var i = 0; i < extracted.guids.length; ++i) {
            var guid = extracted.guids[i];
            console.log(guid + "\t" + factors[guid].join("\t"));
        }
    }
} else if (mode == "tree") {
    processFunction = function (extracted, factors, estJSON, url) {
        for (var i = 0; i < extracted.guids.length; ++i) {
            var guid = extracted.guids[i];
            if (guid.indexOf("_") == -1) {
                console.log(guid + ":" + extracted.splits[i].join("\t"));
            }
        }
    }
} else if (mode == "explain_fake") {
    processFunction = function (extracted, factors, estJSON, url) {
        var textValues = [];
        for (var i = 0; i < estJSON.choice.length; ++i) {
            var choiceTokens = estJSON.choice[i].split("_");
            if (choiceTokens.length > 1) {
                var parentGuid = choiceTokens[0];
                var splitId = parseInt(choiceTokens[1]);
                if (extracted.markedChildren[parentGuid] == null || extracted.markedChildren[parentGuid].length <= splitId) {
                    // TODO: why?
                    console.log("NO: " + parentGuid);
                    continue;
                }
                var jointBlock = extracted.markedChildren[parentGuid][splitId];
                var children = [];

                console.log(estJSON.choice[i] + ":");

                for (var blockId = 0; blockId < jointBlock.length; ++blockId) {
                    var block = jointBlock[blockId];
                    if (block.nodeType == Node.ELEMENT_NODE) {
                        var attrId = 0;
                        for (; attrId < block.attributes.length; ++attrId) {
                            if (block.attributes[attrId].name == "data-guid") {
                                children.push(block.attributes[attrId].value);
                                break;
                            }
                        }
                        if (attrId == block.attributes.length) {
                            // TODO: why?
                            console.log("??? no data-guid: " + blockId);
                        }
                    } else if (block.nodeType == Node.TEXT_NODE) {
                        children.push("t" + textValues.length);
                        textValues.push(block.nodeValue);
                    } else {
                        // TODO: maybe use attribute node also
                        console.log("??? nodeType of " + blockId + " = " + block.nodeType);
                    }
                }
                console.log(children.join("\t") + "\n");
            }
        }
        console.log(JSON.stringify(textValues));
    }
} else if (mode == "split") {
    processFunction = function (extracted, factors, estJSON, url) {
        var chosen = [];
        var ignored = [];
        if (!applyMode) {
            for (var i = 0; i < estJSON.choice.length; ++i) {
                var guid = estJSON.choice[i];
                if (guid.indexOf("_") == -1) {
                    chosen[guid] = 1;
                    var blockIdx = extracted.guids.indexOf(guid);
                    try {
                        for (var j = 0; j < extracted.splits[blockIdx].length; ++j) {
                            if (extracted.splits[blockIdx][j].indexOf("_") == -1) {
                                ignored[extracted.splits[blockIdx][j]] = 1;
                            }
                        }
                    } catch (e) {
                      //  throw new Error(blockIdx + " " + guid + " " + url);
                    }
                }
            }
        }
        for (var i = 0; i < extracted.guids.length; ++i) {
            var guid = extracted.guids[i];
            if (guid.indexOf("_") == -1) {
                if (extracted.splits[i] === null) {
                    continue;
                }
                if (ignored[guid]) {
                    for (var j = 0; j < extracted.splits[i].length; ++j) {
                        if (extracted.splits[i][j].indexOf("_") == -1) {
                            ignored[extracted.splits[i][j]] = 1;
                        }
                    }
                } else {
                    var output = [guid, (chosen[guid]? 0: 1), url, 0];
                    for (var j = 0; j < factors[guid].length; ++j) {
                        var parentFactor = factors[guid][j];
                        output.push(Stats.safeFloat(parentFactor));
                        var childrenFactors = [];
                        for (var k = 0; k < extracted.splits[i].length; ++k) {
                            childrenFactors.push(factors[extracted.splits[i][k]][j]);
                        }
                        childrenFactors.sort(
                            function (a, b) {
                                return a - b;
                            }
                        );
                        output.push(Stats.safeFloat((Stats.max(childrenFactors) - Stats.min(childrenFactors)) / parentFactor));
                        var childrenAverage = Stats.average(childrenFactors);
                        output.push(Stats.safeFloat(childrenAverage / parentFactor));
                        output.push(Stats.safeFloat(Stats.median(childrenFactors) / parentFactor));
                        output.push(Stats.safeFloat(Stats.variance(childrenFactors, childrenAverage)));
                    }
                    var splittables = 0;
                    for (var j = 0; j < extracted.splits[i].length; ++j) {
                        if (extracted.splits[i][j].indexOf("_") == -1) {
                            ++splittables;
                        }
                    }
                    output.push(splittables);
                    output.push(extracted.splits[i].length - splittables);
                    console.log(output.join("\t"));
                }
            }
        }
    }
} else if (mode == "merge") {
    processFunction = function (extracted, factors, estJSON, url) {
        parentsSeq = function (guid) {
            var res = [];
            var curParent = parents[guid];
            while (curParent) {
                res.push(parseInt(curParent));
                curParent = parents[curParent];
            }
            return res;
        };
        printMergeFactors = function (guid1, guid2, merge, mergedBefore) {
            var output = [guid1.replace("_", ""), merge, url, 0];
            if (typeof(factors[guid1]) == "undefined" || typeof(factors[guid2]) == "undefined") {
                return;
            }
            for (var i = 0; i < factors[guid1].length; ++i) {
                output.push(Stats.safeFloat(factors[guid1][i]));
                output.push(Stats.safeFloat(factors[guid2][i]));
                output.push(Stats.safeFloat(Math.abs(factors[guid1][i] - factors[guid2][i])));
            }
            var parent1 = parents[guid1];
            var parent2 = parents[guid2];
            if (typeof(parent1) == "undefined" || typeof(parent2) == "undefined") {
                return;
            }
            output.push((parent1 == parent2? 1: 0));
            var parents1 = parentsSeq(guid1);
            var parents2 = parentsSeq(guid2);
            var dist1 = 0;
            var dist2 = 0;
            while (dist1 < parents1.length && dist2 < parents2.length) {
                if (parents1[dist1] > parents2[dist2]) {
                    ++dist1;
                    continue;
                }
                if (parents2[dist2] > parents1[dist1]) {
                    ++dist2;
                    continue;
                }
                break;
            }
            output.push(dist1 + 1);
            output.push(dist2 + 1);
            output.push(Math.abs(dist1 - dist2));
            output.push(extracted.splits[extracted.guids.indexOf(parent1)].length);
            output.push(extracted.splits[extracted.guids.indexOf(parent2)].length);
            output.push(mergedBefore);
            console.log(output.join("\t"));
        };

        var parents = [];
        for (var i = 0; i < extracted.guids.length; ++i) {
            var guid = extracted.guids[i];
            if (guid.indexOf("_") == -1) {
                if (extracted.splits[i] === null) {
                    continue;
                }
                for (var j = 0; j < extracted.splits[i].length; ++j) {
                    parents[extracted.splits[i][j]] = guid;
                }
            }
        }
        for (var i = 0; i < estJSON.merge.length; ++i) {
            var spl = estJSON.merge[i].split(",");
            var begin = parseInt(spl[0]);
            var end = parseInt(spl[1]);
            for (var j = begin; j < end - 1; ++j) {
                var guid1 = estJSON.choice[j];
                var guid2 = estJSON.choice[j + 1];
                printMergeFactors(guid1, guid2, 1, j - begin);
            }
            if (end < estJSON.choice.length) {
                var guid1 = estJSON.choice[end - 1];
                var guid2 = estJSON.choice[end];
                printMergeFactors(guid1, guid2, 0, end - 1 - begin);
            }
        }
    }
} else if (mode == "annotate") {
    processFunction = function (extracted, factors, estJSON, url) {
        for (var i = 0; i < estJSON.merge.length; ++i) {
            var spl = estJSON.merge[i].split(",");
            var mnemonics = spl[2];
            var output = [i, MNEMONICS_2_CLASSCODES[mnemonics], url, 0];
            if (!extracted.mergedFactors[i].length) {
                continue;
            }
            var mergedFactors = extracted.mergedFactors[i].split("\t");
            var factorCount = 0;
            for (var j = 0; j < mergedFactors.length; ++j) {
                output.push(Stats.safeFloat(parseFloat(mergedFactors[j])));
                ++factorCount;
            }
      //      Neighbors
      //      if (i == 0 || !extracted.mergedFactors[i - 1].length) {
      //          for (var j = 0; j < factorCount; ++j) {
      //              output.push(0);
      //          }
      //      } else {
      //          var mergedFactors = extracted.mergedFactors[i - 1].split("\t");
      //          for (var j = 0; j < mergedFactors.length; ++j) {
      //              output.push(Stats.safeFloat(parseFloat(mergedFactors[j])));
      //          }
      //      }
      //      if (i == estJSON.merge.length - 1 || !extracted.mergedFactors[i + 1].length) {
      //          for (var j = 0; j < factorCount; ++j) {
      //              output.push(0);
      //          }
      //      } else {
      //          var mergedFactors = extracted.mergedFactors[i + 1].split("\t");
      //          for (var j = 0; j < mergedFactors.length; ++j) {
      //              output.push(Stats.safeFloat(parseFloat(mergedFactors[j])));
      //          }
      //      }
            console.log(output.join("\t"));
        }
    }
} else if (mode == "labels") {
    console.log(JSON.stringify(MNEMONICS_2_CLASSCODES));
    phantom.exit(0);
} else if (mode == "sentences") {
    processFunction = function (extracted, factors, estJSON, url) {
        printSentences = function (label, extracted, guid) {
            var idx = extracted.guids.indexOf(guid);
            if (idx == -1) {
                return;
            }
            if (guid.indexOf("_") > 0) {
                for (var i = 0; i < extracted.sentences[idx].length; ++i) {
                    console.log(label + "\t" + extracted.sentences[idx][i]);
                }
            } else {
                for (var i = 0; i < extracted.splits[idx].length; ++i) {
                    printSentences(label, extracted, extracted.splits[idx][i]);
                }
            }
        }

        for (var i = 0; i < estJSON.merge.length; ++i) {
            var spl = estJSON.merge[i].split(",");
            var label = spl[2];
            var beginning = parseInt(spl[0]);
            var end = parseInt(spl[1]);
            for (var j = beginning; j < end; ++j) {
                var guid = estJSON.choice[j];
                printSentences(label, extracted, guid);
            }
        }
    }
} else if (mode == "factnames") {
    // pass
} else {
    console.log("Usage: extractor.js {basic|tree|split|merge|annotate|labels|sentences|factnames|explain_fake} [-f <estFilename>] [apply] [<start>] [<end>]");
    phantom.exit(1);
}

var estFile = fs.open(estFilename, "r");
var urlFile = fs.open("urls.txt", "r");
var estsRead = 0;
while (estsRead < start) {
    var line = estFile.readLine();
    ++estsRead;
}
var urlsRead = 0;
var urlProcessed = false;
var finished = false;
var url;
var interval = setInterval(
    function () {
        if (!urlProcessed) {
            if (estsRead >= end) {
                clearInterval(interval);
                estFile.close();
                urlFile.close();
                finished = true;
                return;
            }
            var line = estFile.readLine();
            ++estsRead;
            if (!line.length) {
                clearInterval(interval);
                estFile.close();
                urlFile.close();
                finished = true;
                return;
            }
            urlProcessed = true;
            var est = line.trim().split("\t");
            var docId = parseInt(est[0]);
            while (urlsRead < docId) {
                url = urlFile.readLine().trim();
                ++urlsRead;
            }
            var estJSON = JSON.parse(est[2]);
            if (!((applyMode && mode == "split") || mode == "tree")) {
                if (!estJSON.choice.length) {
                    urlProcessed = false;
                    return;
                }
            }
            var webPage = wp.create();
            webPage.open("https://mcquack.search.yandex.net:8043/extractor/container.html", function (status) {
                webPage.evaluate(function (url) {
                    stage = "choice";
                    $("#html_page").attr("src",
                                         "https://mcquack.search.yandex.net:8043/fetch?url=" + encodeURIComponent(url) + "&offline=1");
                }, url);
                webPage.onLoadFinished = function () {
                    webPage.injectJs("stats.js");
                    var extracted = webPage.evaluate(extractFactors, estJSON, mode);
                    webPage.close();
                    if (mode == "factnames") {
                        if (extracted.guids.length > 0) {
                            fId = -1;
                            console.log("text: " + extracted.factorNames.text.length);
                            console.log(extracted.factorNames.text.map(function(fName) {++fId; return fId + " " + fName;}).join("\n"));
                            console.log("char: " + extracted.factorNames.char.length);
                            console.log(extracted.factorNames.char.map(function(fName) {++fId; return fId + " " + fName;}).join("\n"));
                            console.log("link: " + extracted.factorNames.link.length);
                            console.log(extracted.factorNames.link.map(function(fName) {++fId; return fId + " " + fName;}).join("\n"));
                            console.log("font: " + extracted.factorNames.font.length);
                            console.log(extracted.factorNames.font.map(function(fName) {++fId; return fId + " " + fName;}).join("\n"));
                            console.log("tree: " + extracted.factorNames.tree.length);
                            console.log(extracted.factorNames.tree.map(function(fName) {++fId; return fId + " " + fName;}).join("\n"));
                        }
                        phantom.exit(0);
                    }
                    var factors = [];
                    if (mode != "sentences") {
                        for (var i = 0; i < extracted.guids.length; ++i) {
                            var guid = extracted.guids[i];
                            factors[guid] = extracted.factors[i].split("\t");
                            for (var j = 0; j < factors[guid].length; ++j) {
                                if (factors[guid][j].indexOf(",") == -1) {
                                    factors[guid][j] = parseFloat(factors[guid][j]);
                                }
                            }
                        }
                    }
                    processFunction(extracted, factors, estJSON, url);
                    if (estsRead < end && mode == "sentences") {
                        console.log("<-------------------------------------------------->");
                    }
                    if (applyMode) {
                        phantom.exit(0);
                    }
                    urlProcessed = false;
                };
            });
        }
    }, 1000
);

var interval2 = setInterval(
    function () {
        if (finished) {
            phantom.exit(0);
        }
    }, 1000
);

extractFactors = function (estJSON, mode) {

    // Additional namespaces
    
    PARAGRAPH_BORDERS = [
        "li", "td", "th",
    ];
    TAG_SET_1 = [
        "a", "table", "p", "form",
        "article", "aside", "address", "blockquote",
        "footer", "header", "nav", "section",
        "ul", "ol", "dl",
        "h1", "h2", "h3", "h4", "h5", "h6",
    ];
    TAG_SET_2 = [
        "img", "audio", "canvas", "embed", "object",
        "map", "iframe", "video",
        "h1", "h2", "h3", "h4", "h5", "h6",
        "p",
        "form", "input", "textarea", "button", "label", "select", "progress",
        "table",
    ];
    TABLE_TAGS = [
        "table", "thead", "tbody",
    ];

    LINK_TYPES = [
        "e", "i", "s", "a", "l", "m",
    ];
    EXT_LINK_TYPES = [
        "es", "is", "eis"
    ];

    INLINE_MODIFIERS = [
        "", "i"
    ];

    FONT_SUBSCRIPTS = [
        "f", "z", "s", "v", "h", "d", "w"
    ];

    NUMERIC_FONT_SUBSCRIPTS = [
        "z"
    ];

    NON_INHERITED_FONT_SUBSCRIPTS = [
        "d"
    ];

    NON_INHERITED_BREAKERS = {
        d: [
            "inline-block", "inline-flex", "inline-table",
        ],
    };

    SUBSCRIPTS_2_CSS = {
        f: "font-family",
        z: "font-size",
        s: "font-style",
        v: "font-variant",
        h: "line-height",
        d: "text-decoration",
        w: "font-weight",
    };


    RegExps = {
        Letter: new RegExp("[a-z\\u00C0-\\u1FFF\\u2C00-\\uD7FF]", "ig"),
        Digit: new RegExp("\\d", "g"),
        Punct: new RegExp("[-\\.,!:;\\\"'()[\\]?\\u2212\\u2010\\u2011\\u2013-\\u2015«»]", "g"),
        Trash: new RegExp("[^- a-z0-9\\u00C0-\\u1FFF\\u2C00-\\uD7FF\\.,!:;\\\"'()[\\]?\\u2212\\u2010\\u2011\\u2013-\\u2015«»]", "ig"),
        Quote: new RegExp("[\\\"'«»]", "g"),
        Parentheses: new RegExp("[()]", "g"),
        Brackets: new RegExp("[[\\]]", "g"),
        Dash: new RegExp("[-\\u2212\\u2010\\u2011\\u2013-\\u2015]", "g"),
        EndPunct: new RegExp("[\\.!?]", "g"),
        MiddlePunct: new RegExp("[-,:;\\\"'()[\\]\\u2212\\u2010\\u2011\\u2013-\\u2015«»]", "g"),

        Url: new RegExp("^(?:(?:.+:)?//)?((?:[^/\\.]*?\\.)*?([^/\\.]*?\\.[^/\\.]*?))(:\d+)?/[^#]*(#.*)?"),
        Anchor: new RegExp("#.*"),
    };

    function NumericFactor() {
        this.val = 0;
    };

    NumericFactor.prototype = {
        update: function (upd) {
            this.val += upd;
        },
    };

    function ArrayFactor() {
        this.val = [];
    };

    ArrayFactor.prototype = {
        update: function (upd) {
            var i = 0;
            var j = 0;
            var res = [];
            while (i < this.val.length && j < upd.length) {
                if (this.val[i] < upd[j]) {
                    res.push(this.val[i++]);
                } else {
                    res.push(upd[j++]);
                }
            }
            while (i < this.val.length) {
                res.push(this.val[i++]);
            }
            while (j < upd.length) {
                res.push(upd[j++]);
            }
            this.val = res;
        },
    };

    function SetFactor() {
        this.val = [];
    };

    SetFactor.prototype = {
        update: function (upd) {
            var i = 0;
            var j = 0;
            var res = [];
            while (i < this.val.length && j < upd.length) {
                if (this.val[i] < upd[j]) {
                    res.push(this.val[i++]);
                } else {
                    if (this.val[i] == upd[j]) {
                        ++j;
                    } else {
                        res.push(upd[j++]);
                    }
                }
            }
            while (i < this.val.length) {
                if (!res.length || this.val[i] != res[res.length - 1]) {
                    res.push(this.val[i++]);
                } else {
                    ++i;
                }
            }
            while (j < upd.length) {
                if (!res.length || upd[j] != res[res.length - 1]) {
                    res.push(upd[j++]);
                } else {
                    ++j;
                }
            }
            this.val = res;
        },
    };

    function HashFactor() {
        this.val = [];
    };

    HashFactor.prototype = {
        update: function (upd) {
            for (var key in upd) {
                if (typeof(this.val[key]) == "undefined") {
                    this.val[key] = upd[key];
                } else {
                    this.val[key] += upd[key];
                }
            }
        },
    };

    Factors = {
    };

    FactorTypes = {
        Tw: NumericFactor,
        Ts: NumericFactor,
        Tp: NumericFactor,
        TwsArr: ArrayFactor,
        TwpArr: ArrayFactor,
        TspArr: ArrayFactor,
        Sc: NumericFactor,
        Sa: NumericFactor,
        Su: NumericFactor,
        Sd: NumericFactor,
        Sp: NumericFactor,
        St: NumericFactor,
        Sq: NumericFactor,
        Sb: NumericFactor,
        Ss: NumericFactor,
        Sh: NumericFactor,
        Se: NumericFactor,
        Sm: NumericFactor,
        SusArr: ArrayFactor,
        SupArr: ArrayFactor,
        SasArr: ArrayFactor,
        SapArr: ArrayFactor,
        SpsArr: ArrayFactor,
        SppArr: ArrayFactor,
        SqsArr: ArrayFactor,
        SqpArr: ArrayFactor,
        ShsArr: ArrayFactor,
        ShpArr: ArrayFactor,
        SesArr: ArrayFactor,
        SepArr: ArrayFactor,
        SmsArr: ArrayFactor,
        SmpArr: ArrayFactor,
        SdsArr: ArrayFactor,
        SdpArr: ArrayFactor,
        StsArr: ArrayFactor,
        StpArr: ArrayFactor,
        Li: NumericFactor,
        Le: NumericFactor,
        Ls: NumericFactor,
        La: NumericFactor,
        Ll: NumericFactor,
        Lm: NumericFactor,
        L_is_: NumericFactor,
        L_es_: NumericFactor,
        L_eis_: NumericFactor,
        Liw: NumericFactor,
        Lew: NumericFactor,
        Lsw: NumericFactor,
        Law: NumericFactor,
        Llw: NumericFactor,
        Lmw: NumericFactor,
        L_is_w: NumericFactor,
        L_es_w: NumericFactor,
        L_eis_w: NumericFactor,
        Lis: NumericFactor,
        Les: NumericFactor,
        Lss: NumericFactor,
        Las: NumericFactor,
        Lls: NumericFactor,
        Lms: NumericFactor,
        L_is_s: NumericFactor,
        L_es_s: NumericFactor,
        L_eis_s: NumericFactor,
        Lip: NumericFactor,
        Lep: NumericFactor,
        Lsp: NumericFactor,
        Lap: NumericFactor,
        Llp: NumericFactor,
        Lmp: NumericFactor,
        L_is_p: NumericFactor,
        L_es_p: NumericFactor,
        L_eis_p: NumericFactor,
        LesArr: ArrayFactor,
        LisArr: ArrayFactor,
        LssArr: ArrayFactor,
        LasArr: ArrayFactor,
        LlsArr: ArrayFactor,
        LmsArr: ArrayFactor,
        L_es_sArr: ArrayFactor,
        L_is_sArr: ArrayFactor,
        L_eis_sArr: ArrayFactor,
        LewsArr: ArrayFactor,
        LiwsArr: ArrayFactor,
        LswsArr: ArrayFactor,
        LawsArr: ArrayFactor,
        LlwsArr: ArrayFactor,
        LmwsArr: ArrayFactor,
        L_es_wsArr: ArrayFactor,
        L_is_wsArr: ArrayFactor,
        L_eis_wsArr: ArrayFactor,
        LepArr: ArrayFactor,
        LipArr: ArrayFactor,
        LspArr: ArrayFactor,
        LapArr: ArrayFactor,
        LlpArr: ArrayFactor,
        LmpArr: ArrayFactor,
        L_es_pArr: ArrayFactor,
        L_is_pArr: ArrayFactor,
        L_eis_pArr: ArrayFactor,
        LewpArr: ArrayFactor,
        LiwpArr: ArrayFactor,
        LswpArr: ArrayFactor,
        LawpArr: ArrayFactor,
        LlwpArr: ArrayFactor,
        LmwpArr: ArrayFactor,
        L_es_wpArr: ArrayFactor,
        L_is_wpArr: ArrayFactor,
        L_eis_wpArr: ArrayFactor,
        LespArr: ArrayFactor,
        LispArr: ArrayFactor,
        LsspArr: ArrayFactor,
        LaspArr: ArrayFactor,
        LlspArr: ArrayFactor,
        LmspArr: ArrayFactor,
        L_es_spArr: ArrayFactor,
        L_is_spArr: ArrayFactor,
        L_eis_spArr: ArrayFactor,
        FfSet: SetFactor,
        FfiSet: SetFactor,
        FfcHash: HashFactor,
        FficHash: HashFactor,
        FfwHash: HashFactor,
        FfiwHash: HashFactor,
        FfsHash: HashFactor,
        FfisHash: HashFactor,
        FfpHash: HashFactor,
        FfipHash: HashFactor,
        FzSet: SetFactor,
        FziSet: SetFactor,
        FzcHash: HashFactor,
        FzicHash: HashFactor,
        FzwHash: HashFactor,
        FziwHash: HashFactor,
        FzsHash: HashFactor,
        FzisHash: HashFactor,
        FzpHash: HashFactor,
        FzipHash: HashFactor,
        FsSet: SetFactor,
        FsiSet: SetFactor,
        FscHash: HashFactor,
        FsicHash: HashFactor,
        FswHash: HashFactor,
        FsiwHash: HashFactor,
        FssHash: HashFactor,
        FsisHash: HashFactor,
        FspHash: HashFactor,
        FsipHash: HashFactor,
        FvSet: SetFactor,
        FviSet: SetFactor,
        FvcHash: HashFactor,
        FvicHash: HashFactor,
        FvwHash: HashFactor,
        FviwHash: HashFactor,
        FvsHash: HashFactor,
        FvisHash: HashFactor,
        FvpHash: HashFactor,
        FvipHash: HashFactor,
        FhSet: SetFactor,
        FhiSet: SetFactor,
        FhcHash: HashFactor,
        FhicHash: HashFactor,
        FhwHash: HashFactor,
        FhiwHash: HashFactor,
        FhsHash: HashFactor,
        FhisHash: HashFactor,
        FhpHash: HashFactor,
        FhipHash: HashFactor,
        FdSet: SetFactor,
        FdiSet: SetFactor,
        FdcHash: HashFactor,
        FdicHash: HashFactor,
        FdwHash: HashFactor,
        FdiwHash: HashFactor,
        FdsHash: HashFactor,
        FdisHash: HashFactor,
        FdpHash: HashFactor,
        FdipHash: HashFactor,
        FwSet: SetFactor,
        FwiSet: SetFactor,
        FwcHash: HashFactor,
        FwicHash: HashFactor,
        FwwHash: HashFactor,
        FwiwHash: HashFactor,
        FwsHash: HashFactor,
        FwisHash: HashFactor,
        FwpHash: HashFactor,
        FwipHash: HashFactor,
    };

    
    elems = [];
    rootDists = {
        all: [],
        block: [],
    };
    depths = {
        all: [],
        block: []
    };
    tagSet1Dists = [];
    tableFactors = [];
    treeLayoutFactors = [];

    Overloads = {
        // While extracting factors we don't highlight
        // the element on top of the stack.
        // We just remember that its factors need to be collected.
        newAsk: function () {
            var elem = Globals.stack[Globals.stack.length - 1];
            var guid = elem.attr("data-guid");
            FactorsProcessing.initFactors(guid);
            elems[guid] = elem;
            NodesProcessing.calculateDistance(elem, guid);
            NodesProcessing.calculateNonPropagatedNodeFactors(elem, guid);
        },
        // while extracting sentences we extract nothing for splittable blocks
        sentencesNewAsk: function () {
            var elem = Globals.stack[Globals.stack.length - 1];
            var guid = elem.attr("data-guid");
            guids.push(guid);
            calculatedSentences[guid] = [];
        },
        // While processing children to be marked and pushed
        // we don't highlight marked children,
        // we evaluate their factors.
        // We don't support answer stack because all answers are "Yes".
        newAnswer: function () {
            var elem = Globals.stack.pop();
            var guid = elem.attr("data-guid");
            var prevTop = Globals.stack.length;
            // fills markedChildren hash and pushes splittable children to stack
            TreeProcessing.buildMarkedChildren(elem);
            splits[guid] = [];
            for (var i = 0; i < Globals.markedChildren[guid].length; ++i) {
                var marked = Globals.markedChildren[guid][i];
                splits[guid].push(guid + "_" + i);
                elems[guid + "_" + i] = marked;
                if (mode != "sentences") {
                    FactorsProcessing.calculateFactors(marked, guid + "_" + i);
                    NodesProcessing.calculateNonPropagatedNodeFactors(marked, guid + "_" + i);
                } else {
                    TextExtraction.calculateSentences(marked, guid + "_" + i);
                }
            }
            for (var i = Globals.stack.length - 1; i >= prevTop; --i) {
                var childGuid = Globals.stack[i].attr("data-guid");
                var chSucc = Globals.successors[childGuid];
                var placed = false;
                if (chSucc) {
                    var succId = splits[guid].indexOf(chSucc);
                    if (succId >= 0) {
                        splits[guid].splice(succId, 0, childGuid);
                        placed = true;
                    }
                }
                if (!placed) {
                    splits[guid].push(childGuid);
                }
            }
            if (Globals.stack.length) {
                ChoiceStage.ask();
            }
        },
    };

    FactorsProcessing = {
        initFactors: function (guid) {
            for (var factor in Factors) {
                Factors[factor][guid] = new FactorTypes[factor]();
            }
            guids.push(guid);
        },
        calculateFactors: function (elem, guid) {
            FactorsProcessing.initFactors(guid);
            NodesProcessing.calculateDistance(elem, guid);
            var parInfo = TextExtraction.paragraphs(elem, guid);
            var paragraphs = parInfo.pArray;
            for (var i = 0; i < paragraphs.length; ++i) {
                var pLinks = [];
                var pOuterLinks = [];
                for (var j = 0; j < LINK_TYPES.length; ++j) {
                    pLinks[LINK_TYPES[j]] = 0;
                    pOuterLinks[LINK_TYPES[j]] = 0;
                }
                for (var j = 0; j < parInfo.containment[i].length; ++j) {
                    var link = parInfo.links[parInfo.containment[i][j]];
                    ++pLinks[link.type];
                    if (
                        (link.beginParagraph < i || link.beginOffset == 0) &&
                        (link.endParagraph > i || link.endOffset >= paragraphs[i].length) &&
                        pOuterLinks[link.type] == 0
                    ) {
                        Factors["L" + link.type + "p"][guid].update(1);
                        pOuterLinks[link.type] = 1;
                    }
                }
                for (var j = 0; j < LINK_TYPES.length; ++j) {
                    Factors["L" + LINK_TYPES[j] + "pArr"][guid].update([pLinks[LINK_TYPES[j]]]);
                }
                for (var j = 0; j < EXT_LINK_TYPES.length; ++j) {
                    var sum = 0;
                    var disj = 0;
                    for (var k = 0; k < EXT_LINK_TYPES[j].length; ++k) {
                        sum += pLinks[EXT_LINK_TYPES[j].charAt(k)];
                        disj |= pOuterLinks[EXT_LINK_TYPES[j].charAt(k)];
                    }
                    Factors["L_" + EXT_LINK_TYPES[j] + "_p"][guid].update(disj);
                    Factors["L_" + EXT_LINK_TYPES[j] + "_pArr"][guid].update([sum]);
                }
                for (var j = 0; j < INLINE_MODIFIERS.length; ++j) {
                    for (var k = 0; k < FONT_SUBSCRIPTS.length; ++k) {
                        var chunks = parInfo[INLINE_MODIFIERS[j] + "chunks"][FONT_SUBSCRIPTS[k]][i];
                        var setName = "F" + FONT_SUBSCRIPTS[k] + INLINE_MODIFIERS[j] + "Set";
                        var hashName = "F" + FONT_SUBSCRIPTS[k] + INLINE_MODIFIERS[j] + "pHash";
                        if (chunks.length && chunks[0].begin == 0 && chunks[0].end == paragraphs[i].length) {
                            var hash = [];
                            hash[chunks[0].prop] = 1;
                            Factors[hashName][guid].update(hash);
                        }
                        for (var l = 0; l < chunks.length; ++l) {
                            var hash = [];
                            hash[chunks[l].prop] = chunks[l].end - chunks[l].begin;
                            Factors["F" + FONT_SUBSCRIPTS[k] + INLINE_MODIFIERS[j] + "cHash"][guid].update(hash);
                            Factors[setName][guid].update([chunks[l].prop]);
                        }
                    }
                }
            }
            FactorsProcessing.calculateSimpleLinkFactors(guid, parInfo);
            Factors.Tp[guid].update(paragraphs.length);
            var charArraySubscripts = ["u", "a", "p", "q", "h", "e", "m"];
            for (var i = 0; i < paragraphs.length; ++i) {
                var sentences = TextExtraction.sentences(parInfo, i);
                Factors.TspArr[guid].update([sentences.bounds.length]);
                var wordsInParagraph = {w: 0};
                for (var j = 0; j < LINK_TYPES.length; ++j) {
                    wordsInParagraph[LINK_TYPES[j]] = 0;
                }
                for (var j = 0; j < EXT_LINK_TYPES.length; ++j) {
                    wordsInParagraph[EXT_LINK_TYPES[j]] = 0;
                }
                var paragraphCharStats = {
                    a: 0,
                    u: 0,
                    d: 0,
                    p: 0,
                    t: 0,
                    q: 0,
                    b: 0,
                    s: 0,
                    h: 0,
                    e: 0,
                    m: 0,
                };
                for (var j = 0; j < sentences.bounds.length; ++j) {
                    var sentence = paragraphs[i].slice(sentences.bounds[j].begin, sentences.bounds[j].end);
                    var wcRes = TextExtraction.wordCount(parInfo, i, sentences.bounds[j]);
                    var wc = wcRes.counts;
                    for (var wcProp in wc) {
                        wordsInParagraph[wcProp] += wc[wcProp];
                    }
                    for (var wcHashName in wcRes.hashes) {
                        Factors["F" + wcHashName + "wHash"][guid].update(wcRes.hashes[wcHashName]);
                    }
                    Factors.TwsArr[guid].update([wc.w]);
                    Factors.Tw[guid].update(wc.w);
                    for (var k = 0; k < LINK_TYPES.length; ++k) {
                        Factors["L" + LINK_TYPES[k] + "w"][guid].update(wc[LINK_TYPES[k]]);
                        Factors["L" + LINK_TYPES[k] + "wsArr"][guid].update([wc[LINK_TYPES[k]]]);
                        Factors["L" + LINK_TYPES[k] + "sArr"][guid].update([sentences.bounds[j][LINK_TYPES[k]]]);
                    }
                    for (var k = 0; k < EXT_LINK_TYPES.length; ++k) {
                        var sum = 0;
                        for (var l = 0; l < EXT_LINK_TYPES[k].length; ++l) {
                            sum += sentences.bounds[j][EXT_LINK_TYPES[k].charAt(l)];
                        }
                        Factors["L_" + EXT_LINK_TYPES[k] + "_w"][guid].update(wc[EXT_LINK_TYPES[k]]);
                        Factors["L_" + EXT_LINK_TYPES[k] + "_sArr"][guid].update([sum]);
                        Factors["L_" + EXT_LINK_TYPES[k] + "_wsArr"][guid].update([wc[EXT_LINK_TYPES[k]]]);
                    }
                    var stats = CharExtraction.charStats(sentence);
                    for (var charClass in stats) {
                        if (charArraySubscripts.indexOf(charClass) >= 0) {
                            Factors["S" + charClass + "sArr"][guid].update([stats[charClass]]);
                        }
                        paragraphCharStats[charClass] += stats[charClass];
                    }
                }
                Factors.TwpArr[guid].update([wordsInParagraph.w]);
                for (var j = 0; j < LINK_TYPES.length; ++j) {
                    Factors["L" + LINK_TYPES[j] + "s"][guid].update(sentences[LINK_TYPES[j]]);
                    Factors["L" + LINK_TYPES[j] + "spArr"][guid].update([sentences[LINK_TYPES[j]]]);
                    Factors["L" + LINK_TYPES[j] + "wpArr"][guid].update([wordsInParagraph[LINK_TYPES[j]]]);
                }
                for (var j = 0; j < EXT_LINK_TYPES.length; ++j) {
                    Factors["L_" + EXT_LINK_TYPES[j] + "_s"][guid].update(sentences[EXT_LINK_TYPES[j]]);
                    Factors["L_" + EXT_LINK_TYPES[j] + "_spArr"][guid].update([sentences[EXT_LINK_TYPES[j]]]);
                    Factors["L_" + EXT_LINK_TYPES[j] + "_wpArr"][guid].update([wordsInParagraph[EXT_LINK_TYPES[j]]]);
                }
                Factors.Ts[guid].update(sentences.bounds.length);
                for (var j = 0; j < FONT_SUBSCRIPTS.length; ++j) {
                    Factors["F" + FONT_SUBSCRIPTS[j] + "sHash"][guid].update(sentences[FONT_SUBSCRIPTS[j] + "Hash"]);
                    Factors["F" + FONT_SUBSCRIPTS[j] + "isHash"][guid].update(sentences[FONT_SUBSCRIPTS[j] + "iHash"]);
                }
                for (var charClass in paragraphCharStats) {
                    if (charArraySubscripts.indexOf(charClass) >= 0) {
                        Factors["S" + charClass + "pArr"][guid].update([paragraphCharStats[charClass]]);
                    }
                    Factors["S" + charClass][guid].update(paragraphCharStats[charClass]);
                }
                Factors.Sc[guid].update(paragraphs[i].length);
            }
            FactorsProcessing.updateParents(elem, guid);
            NodesProcessing.updateDepths(elem, guid);
        },
        calculateSimpleLinkFactors: function (guid, parInfo) {
            for (var i = 0; i < parInfo.links.length; ++i) {
                Factors["L" + parInfo.links[i].type][guid].update(1);
            }
            for (var i = 0; i < EXT_LINK_TYPES.length; ++i) {
                for (var j = 0; j < EXT_LINK_TYPES[i].length; ++j) {
                    Factors["L_" + EXT_LINK_TYPES[i] + "_"][guid].update(Factors["L" + EXT_LINK_TYPES[i].charAt(j)][guid].val);
                }
            }
        },
        updateParents: function (elem, guid) {
            var parents = elem.parents();
            for (var i = 0; i < parents.length; ++i) {
                var parentGuid = $(parents[i]).attr("data-guid");
                if (typeof(parentGuid) != "undefined" && guids.indexOf(parentGuid) >= 0) {
                    for (var factor in Factors) {
                        Factors[factor][parentGuid].update(Factors[factor][guid].val);
                    }
                }
            }
        },
        textFactorsArray: function (guid) {
            var subscripts = ["w", "s", "p"];
            var res = [];
            var names = [];
            // include factors
            for (var i = 0; i < subscripts.length; ++i) {
                res.push(Factors["T" + subscripts[i]][guid].val);
                names.push("T" + subscripts[i]);
            }
            // include (factor / document factor)
            for (var i = 0; i < subscripts.length; ++i) {
                res.push(res[i] / Factors["T" + subscripts[i]][guids[0]].val);
                names.push("T" + subscripts[i] + "/guid_0");
            }
            var groupSubscripts = ["ws", "wp", "sp"];
            var bTextAverages = FactorsProcessing.groupQuotients(guid, "T", groupSubscripts);
            var dTextAverages = FactorsProcessing.groupQuotients(guids[0], "T", groupSubscripts);
            res = res.concat(bTextAverages);
            for (var i = 0; i < bTextAverages.length; ++i) {
                names.push("textAverages" + bTextAverages[i]);
            }
            for (var i = 0; i < bTextAverages.length; ++i) {
                res.push(bTextAverages[i] / dTextAverages[i]);
                names.push("avgT" + groupSubscripts[i] + "/guid_0");
            }
            res.push((bTextAverages[1] * bTextAverages[2]) / (dTextAverages[1] * dTextAverages[2]));
            names.push("avgT" + groupSubscripts[1] + "*avgT" + groupSubscripts[2] + "/guid_0");
            var bTextMedians = FactorsProcessing.medians(guid, "T", groupSubscripts);
            var dTextMedians = FactorsProcessing.medians(guids[0], "T", groupSubscripts);
            for (var i = 0; i < bTextMedians.length; ++i) {
                res.push(bTextMedians[i]);
                names.push("medT" + groupSubscripts[i]);
                res.push(bTextMedians[i] / dTextMedians[i]);
                names.push("medT" + groupSubscripts[i] + "/guid_0");
            }
            res.push((bTextMedians[1] * bTextMedians[2]) / (dTextMedians[1] * dTextMedians[2]));
            names.push("medT" + groupSubscripts[1] + "*medT" + groupSubscripts[2] + "/guid_0");
            var bTextVariances = FactorsProcessing.textVariances(guid, bTextAverages);
            var dTextVariances = FactorsProcessing.textVariances(guids[0], dTextAverages);
            for (var i = 0; i < bTextVariances.length; ++i) {
                res.push(bTextVariances[i]);
                names.push("varT" + groupSubscripts[i]);
                res.push(bTextVariances[i] / dTextVariances[i]);
                names.push("varT" + groupSubscripts[i] + "/guid_0");
            }
            var bTextMins = FactorsProcessing.textMins(guid);
            var dTextMins = FactorsProcessing.textMins(guids[0]);
            for (var i = 0; i < bTextMins.length; ++i) {
                res.push(bTextMins[i]);
                names.push("minT" + groupSubscripts[i]);
                res.push(bTextMins[i] / dTextMins[i]);
                names.push("minT" + groupSubscripts[i] + "/guid_0");
            }
            var bTextMaxs = FactorsProcessing.textMaxs(guid);
            var dTextMaxs = FactorsProcessing.textMaxs(guids[0]);
            for (var i = 0; i < bTextMaxs.length; ++i) {
                res.push(bTextMaxs[i]);
                names.push("maxT" + groupSubscripts[i]);
                res.push(bTextMaxs[i] / dTextMaxs[i]);
                names.push("maxT" + groupSubscripts[i] + "/guid_0");
            }
            for (var i = 0; i < bTextMins.length; ++i) {
                res.push(bTextMins[i] / bTextMaxs[i]);
                names.push("minT" + groupSubscripts[i] + "/maxT" + groupSubscripts[i]);
            }
            for (var i = 0; i < bTextMins.length; ++i) {
                res.push((bTextMins[i] * dTextMaxs[i]) / (bTextMaxs[i] * dTextMins[i]));
                names.push("minT" + groupSubscripts[i] + "*maxguid_0" + "/(maxT" + groupSubscripts[i] + "*minguid_0)");
            }
            for (var i = 0; i < bTextMins.length; ++i) {
                res.push((bTextMaxs[i] - bTextMins[i]) * dTextAverages[i] / (bTextAverages[i] * (dTextMaxs[i] - dTextMins[i])));
                names.push("(maxT" + groupSubscripts[i] + "-" + "minT" + groupSubscripts[i] + ")*avgguid_0" + "/((maxguid_0" + "-minguid_0" + ")*" + "avgT" + groupSubscripts[i] + ")");
            }
            for (var i = 0; i < bTextMins.length; ++i) {
                res.push((bTextMaxs[i] - bTextMins[i]) * dTextMedians[i] / (bTextMedians[i] * (dTextMaxs[i] - dTextMins[i])));
                names.push("(maxT" + groupSubscripts[i] + "-" + "minT" + groupSubscripts[i] + ")*medguid_0" + "/((maxguid_0" + "-minguid_0" + ")*" + "medT" + groupSubscripts[i] + ")");
            }
            return [res, names];
        },
        textVariances: function (guid, averages) {
            var subscripts = ["ws", "wp", "sp"];
            var res = [];
            for (var i = 0; i < subscripts.length; ++i) {
                res.push(Stats.variance(Factors["T" + subscripts[i] + "Arr"][guid].val, averages[i]));
            }
            return res;
        },
        textMins: function (guid) {
            var subscripts = ["ws", "wp", "sp"];
            var res = [];
            for (var i = 0; i < subscripts.length; ++i) {
                res.push(Stats.min(Factors["T" + subscripts[i] + "Arr"][guid].val));
            }
            return res;
        },
        textMaxs: function (guid) {
            var subscripts = ["ws", "wp", "sp"];
            var res = [];
            for (var i = 0; i < subscripts.length; ++i) {
                res.push(Stats.max(Factors["T" + subscripts[i] + "Arr"][guid].val));
            }
            return res;
        },
        charFactorsArray: function (guid) {
            var res = [];
            var names = [];
            var subscripts = ["c", "a", "u", "d", "p", "t", "q", "b", "s", "h", "e", "m"];
            var bFactors = [];
            var dFactors = [];
            for (var i = 0; i < subscripts.length; ++i) {
                var bFactor = Factors["S" + subscripts[i]][guid].val;
                var dFactor = Factors["S" + subscripts[i]][guids[0]].val;
                bFactors.push(bFactor);
                dFactors.push(dFactor);
                res.push(bFactor);
                names.push("S" + subscripts[i]);
                res.push(bFactor / dFactor);
                names.push("S" + subscripts[i] + "/guid_0");
            }
            var groupSubscripts = ["ua", "ac", "da", "pa",
                                   "tc", "ta", "qc", "qa",
                                   "ba", "sa", "ha", "ea",
                                   "eu", "ma", "mu", "me"];
            var bQuotients = FactorsProcessing.groupQuotients(guid, "S", groupSubscripts);
            var dQuotients = FactorsProcessing.groupQuotients(guids[0], "S", groupSubscripts);
            for (var i = 0; i < bQuotients.length; ++i) {
                res.push(bQuotients[i]);
                names.push("S" + groupSubscripts[i]);
                res.push(bQuotients[i] / dQuotients[i]);
                names.push("S" + groupSubscripts[i] + "/guid_0");
            }
            var numericTextFactors = ["Tw", "Ts", "Tp"];
            for (var i = 1; i < subscripts.length; ++i) {
                for (var j = 0; j < numericTextFactors.length; ++j) {
                    var bQuot = bFactors[i] / Factors[numericTextFactors[j]][guid].val;
                    var dQuot = dFactors[i] / Factors[numericTextFactors[j]][guids[0]].val;
                    res.push(bQuot);
                    names.push("S" + subscripts[i] + "/" + numericTextFactors[j]);
                    res.push(bQuot / dQuot);
                    names.push("S" + subscripts[i] + "/" + numericTextFactors[j] + "/guid_0");
                }
            }
            var charArraySubscripts = ["u", "a", "p", "q", "h", "e", "m"];
            var areaSubscripts = ["s", "p"];
            for (var i = 0; i < areaSubscripts.length; ++i) {
                var areaGroupSubscripts = [];
                for (var j = 0; j < charArraySubscripts.length; ++j) {
                    areaGroupSubscripts.push(charArraySubscripts[j] + areaSubscripts[i]);
                }
                var bAreaMedians = FactorsProcessing.medians(guid, "S", areaGroupSubscripts);
                var dAreaMedians = FactorsProcessing.medians(guids[0], "S", areaGroupSubscripts);
                for (var j = 0; j < charArraySubscripts.length; ++j) {
                    res.push(bAreaMedians[j]);
                    names.push("medS" + areaGroupSubscripts[j]);
                    res.push(bAreaMedians[j] / dAreaMedians[j]);
                    names.push("medS" + areaGroupSubscripts[j] + "/guid_0");
                }
            }
            var charArraySubscripts = ["a", "u", "d", "p", "q", "e", "m", "t"];
            var textSubscripts = ["ws", "sp"];
            var bTextMedians = FactorsProcessing.medians(guid, "T", textSubscripts);
            var dTextMedians = FactorsProcessing.medians(guids[0], "T", textSubscripts);
            var bAreaMedians = [];
            var dAreaMedians = [];
            for (var i = 0; i < areaSubscripts.length; ++i) {
                var areaGroupSubscripts = [];
                for (var j = 0; j < charArraySubscripts.length; ++j) {
                    areaGroupSubscripts.push(charArraySubscripts[j] + areaSubscripts[i]);
                }
                bAreaMedians.push(FactorsProcessing.medians(guid, "S", areaGroupSubscripts));
                dAreaMedians.push(FactorsProcessing.medians(guids[0], "S", areaGroupSubscripts));
            }
            for (var i = 0; i < charArraySubscripts.length; ++i) {
                res.push(bAreaMedians[0][i] / bTextMedians[0]);
                names.push("medS" + charArraySubscripts[i] + "/" + "medT" + textSubscripts[0] + "_0");
                res.push(bAreaMedians[1][i] / bTextMedians[1]);
                names.push("medS" + charArraySubscripts[i] + "/" + "medT" + textSubscripts[0] + "_1");
                res.push((bAreaMedians[0][i] / bTextMedians[0]) / (dAreaMedians[0][i] / dTextMedians[0]));
                names.push("(medS" + charArraySubscripts[i] + "/" + "medT" + textSubscripts[0] + "_0)" + "/" + "guid_0");
                res.push((bAreaMedians[1][i] / bTextMedians[1]) / (dAreaMedians[1][i] / dTextMedians[1]));
                names.push("(medS" + charArraySubscripts[i] + "/" + "medT" + textSubscripts[0] + "_1)" + "/" + "guid_0_1");
            }
            return [res, names];
        },
        linkFactorsArray: function (guid) {
            var res = [];
            var names = [];
            var subscripts = ["", "w", "s", "p"];
            var bFactors = [];
            var dFactors = [];
            for (var i = 0; i < subscripts.length; ++i) {
                for (var j = 0; j < LINK_TYPES.length; ++j) {
                    var fName = "L" + LINK_TYPES[j] + subscripts[i];
                    bFactors[fName] = Factors[fName][guid].val;
                    dFactors[fName] = Factors[fName][guids[0]].val;
                    res.push(bFactors[fName]);
                    names.push(fName);
                }
                for (var j = 0; j < EXT_LINK_TYPES.length; ++j) {
                    var fName = "L_" + EXT_LINK_TYPES[j] + "_" + subscripts[i];
                    bFactors[fName] = Factors[fName][guid].val;
                    dFactors[fName] = Factors[fName][guids[0]].val;
                }
            }
            for (var i = 0; i < subscripts.length; ++i) {
                for (var j = 0; j < LINK_TYPES.length; ++j) {
                    var fName = "L" + LINK_TYPES[j] + subscripts[i];
                    res.push(bFactors[fName] / dFactors[fName]);
                    names.push(fName + "/" + "guid_0");
                }
                for (var j = 0; j < EXT_LINK_TYPES.length; ++j) {
                    var fName = "L_" + EXT_LINK_TYPES[j] + "_" + subscripts[i];
                    res.push(bFactors[fName] / dFactors[fName]);
                    names.push(fName + "/" + "guid_0");
                }
            }
            var arraySubscripts = ["s", "ws", "p", "wp", "sp"];
            var bArrays = [];
            var dArrays = [];
            var bAverages = [];
            var dAverages = [];
            for (var i = 0; i < arraySubscripts.length; ++i) {
                for (var j = 0; j < LINK_TYPES.length; ++j) {
                    var fName = "L" + LINK_TYPES[j] + arraySubscripts[i] + "Arr";
                    bArrays[fName] = Factors[fName][guid].val;
                    dArrays[fName] = Factors[fName][guids[0]].val;
                    bAverages[fName] = Stats.average(bArrays[fName]);
                    dAverages[fName] = Stats.average(dArrays[fName]);
                    res.push(bAverages[fName] / dAverages[fName]);
                    names.push("avg" + fName + "/" + "guid_0");
                }
                for (var j = 0; j < EXT_LINK_TYPES.length; ++j) {
                    var fName = "L_" + EXT_LINK_TYPES[j] + "_" + arraySubscripts[i] + "Arr";
                    bArrays[fName] = Factors[fName][guid].val;
                    dArrays[fName] = Factors[fName][guids[0]].val;
                    bAverages[fName] = Stats.average(bArrays[fName]);
                    dAverages[fName] = Stats.average(dArrays[fName]);
                    res.push(bAverages[fName] / dAverages[fName]);
                    names.push("avg" + fName + "/" + "guid_0");
                }
            }
            for (var i = 0; i < arraySubscripts.length; ++i) {
                for (var j = 0; j < LINK_TYPES.length; ++j) {
                    var aName = "L" + LINK_TYPES[j] + arraySubscripts[i] + "Arr";
                    res.push(Stats.median(bArrays[aName]) / Stats.median(dArrays[aName]));
                    names.push("med" + aName + "/" + "guid_0");
                }
                for (var j = 0; j < EXT_LINK_TYPES.length; ++j) {
                    var aName = "L_" + EXT_LINK_TYPES[j] + "_" + arraySubscripts[i] + "Arr";
                    res.push(Stats.median(bArrays[aName]) / Stats.median(dArrays[aName]));
                    names.push("med" + aName + "/" + "guid_0");
                }
            }
            for (var i = 0; i < arraySubscripts.length; ++i) {
                for (var j = 0; j < LINK_TYPES.length; ++j) {
                    var aName = "L" + LINK_TYPES[j] + arraySubscripts[i] + "Arr";
                    res.push(Stats.variance(bArrays[aName], bAverages[aName]) /
                             Stats.variance(dArrays[aName], dAverages[aName]));
                    names.push("var" + aName + "/" + "guid_0");
                }
                for (var j = 0; j < EXT_LINK_TYPES.length; ++j) {
                    var aName = "L_" + EXT_LINK_TYPES[j] + "_" + arraySubscripts[i] + "Arr";
                    res.push(Stats.variance(bArrays[aName], bAverages[aName]) /
                             Stats.variance(dArrays[aName], dAverages[aName]));
                    names.push("var" + aName + "/" + "guid_0");
                }
            }
            var charArraySubscripts = ["c", "a"];
            for (var i = 0; i < charArraySubscripts.length; ++i) {
                var charFactor = Factors["S" + charArraySubscripts[i]][guid].val;
                for (var j = 0; j < subscripts.length; ++j) {
                    for (var k = 0; k < LINK_TYPES.length; ++k) {
                        var fName = "L" + LINK_TYPES[k] + subscripts[j];
                        res.push(Factors[fName][guid].val / charFactor);
                        names.push(fName + "/" + "S" + charArraySubscripts[i]);
                    }
                    for (var k = 0; k < EXT_LINK_TYPES.length; ++k) {
                        var fName = "L_" + EXT_LINK_TYPES[k] + "_" + subscripts[j];
                        res.push(Factors[fName][guid].val / charFactor);
                        names.push(fName + "/" + "S" + charArraySubscripts[i]);
                    }
                }
            }
            var textSubscripts = ["w", "s", "p"];
            for (var i = 0; i < textSubscripts.length; ++i) {
                var linkDomains = ["", textSubscripts[i]];
                var bTextFactor = Factors["T" + textSubscripts[i]][guid].val;
                var dTextFactor = Factors["T" + textSubscripts[i]][guids[0]].val;
                for (var j = 0; j < linkDomains.length; ++j) {
                    for (var k = 0; k < LINK_TYPES.length; ++k) {
                        var fName = "L" + LINK_TYPES[k] + linkDomains[j];
                        var factor = Factors[fName][guid].val / bTextFactor;
                        res.push(factor);
                        names.push(fName + "/" + "T" + textSubscripts[i]);
                        res.push(factor * dTextFactor / Factors[fName][guids[0]].val);
                        names.push(fName + "/" + "T" + textSubscripts[i] + "/" + "guid_0");
                    }
                    for (var k = 0; k < EXT_LINK_TYPES.length; ++k) {
                        var fName = "L_" + EXT_LINK_TYPES[k] + "_" + linkDomains[j];
                        var factor = Factors[fName][guid].val / bTextFactor;
                        res.push(factor);
                        names.push(fName + "/" + "T" + textSubscripts[i]);
                        res.push(factor * dTextFactor / Factors[fName][guids[0]].val);
                        names.push(fName + "/" + "T" + textSubscripts[i] + "/" + "guid_0");
                    }
                }
            }
            return [res, names];
        },
        fontFactorsArray: function (guid) {
            var res = [];
            var names = [];
            for (var i = 0; i < FONT_SUBSCRIPTS.length; ++i) {
                for (var j = 0; j < INLINE_MODIFIERS.length; ++j) {
                    var fName = FONT_SUBSCRIPTS[i] + INLINE_MODIFIERS[j];
                    var setFactor = "F" + fName + "Set";
                    var bFontsNumber = Factors[setFactor][guid].val.length;
                    var dFontsNumber = Factors[setFactor][guids[0]].val.length;
                    res.push(bFontsNumber);
                    names.push(setFactor);
                    res.push((bFontsNumber == 1? 1: 0));
                    names.push(setFactor + "==1");
                    res.push((bFontsNumber > 1? 1: 0));
                    names.push(setFactor + ">1");
                    res.push(bFontsNumber / dFontsNumber);
                    names.push(setFactor + "/" + "guid_0");
                    var bArrays = [];
                    var dArrays = [];
                    var bAverages = [];
                    var dAverages = [];
                    var textSubscripts = ["w", "s", "p"];
                    for (var k = 0; k < textSubscripts.length; ++k) {
                        bArrays[textSubscripts[k]] = new ArrayFactor();
                        dArrays[textSubscripts[k]] = new ArrayFactor();
                    }
                    for (var k = 0; k < textSubscripts.length; ++k) {
                        var hashFactor = "F" + fName + textSubscripts[k] + "Hash";
                        var bHash = Factors[hashFactor][guid].val;
                        var dHash = Factors[hashFactor][guids[0]].val;
                        for (var key in bHash) {
                            bArrays[textSubscripts[k]].update([bHash[key]]);
                        }
                        bAverages[textSubscripts[k]] = Stats.average(bArrays[textSubscripts[k]].val);
                        for (var key in dHash) {
                            dArrays[textSubscripts[k]].update([dHash[key]]);
                        }
                        dAverages[textSubscripts[k]] = Stats.average(dArrays[textSubscripts[k]].val);
                    }
                    res.push(bAverages.s / dAverages.s);
                    names.push("avgS" + "/" + "guid_0");
                    res.push(bAverages.p / dAverages.p);
                    names.push("avgP" + "/" + "guid_0");
                    res.push(Stats.median(bArrays.s.val) / Stats.median(dArrays.s.val));
                    names.push("medS" + "/" + "guid_0");
                    res.push(Stats.median(bArrays.p.val) / Stats.median(dArrays.p.val));
                    names.push("medP" + "/" + "guid_0");
                    res.push(Stats.variance(bArrays.s.val, bAverages.s) / Stats.variance(dArrays.s.val, dAverages.s));
                    names.push("varS" + "/" + "guid_0");
                    res.push(Stats.variance(bArrays.p.val, bAverages.p) / Stats.variance(dArrays.p.val, dAverages.p));
                    names.push("varP" + "/" + "guid_0");
                    for (var k = 0; k < textSubscripts.length; ++k) {
                        var bTextFactor = Factors["T" + textSubscripts[k]][guid].val;
                        var dTextFactor = Factors["T" + textSubscripts[k]][guids[0]].val;
                        res.push(Stats.max(bArrays[textSubscripts[k]].val) / bTextFactor);
                        names.push("max" + textSubscripts[k] + "/" + "T" + textSubscripts[k]);
                        res.push(bFontsNumber / bTextFactor);
                        names.push(setFactor + "/" + "T" + textSubscripts[k]);
                        res.push((bFontsNumber * dTextFactor) / (bTextFactor * dFontsNumber));
                        names.push(setFactor + "/" + "T" + textSubscripts[k] + "/" + "guid_0");
                    }
                    var hashFactor = "F" + fName + "cHash";
                    var bFreq = Stats.freq(Factors[hashFactor][guid].val);
                    var dFreq = Stats.freq(Factors[hashFactor][guids[0]].val);
                    res.push((bFreq == dFreq? 1: 0));
                    names.push("freq" + hashFactor + "==" + "freq" + "guid_0");
                    var charSubscripts = ["a", "c"];
                    for (var k = 0; k < charSubscripts.length; ++k) {
                        var bCharFactor = Factors["S" + charSubscripts[k]][guid].val;
                        var dCharFactor = Factors["S" + charSubscripts[k]][guids[0]].val;
                        res.push(bFontsNumber / bCharFactor);
                        names.push(setFactor + "/" + "S" + charSubscripts[k]);
                        res.push((bFontsNumber * dCharFactor) / (bCharFactor * dFontsNumber));
                        names.push(setFactor + "/" + "S" + charSubscripts[k] + "/" + "guid_0");
                    }
                    for (var k = 0; k < LINK_TYPES.length; ++k) {
                        var bLinkFactor = Factors["L" + LINK_TYPES[k]][guid].val;
                        var dLinkFactor = Factors["L" + LINK_TYPES[k]][guids[0]].val;
                        res.push(bFontsNumber / bLinkFactor);
                        names.push(setFactor + "/" + "L" + LINK_TYPES[k]);
                        res.push((bFontsNumber * dLinkFactor) / (bLinkFactor * dFontsNumber));
                        names.push(setFactor + "/" + "L" + LINK_TYPES[k] + "/" + "guid_0");
                    }
                    for (var k = 0; k < EXT_LINK_TYPES.length; ++k) {
                        var bLinkFactor = Factors["L_" + EXT_LINK_TYPES[k] + "_"][guid].val;
                        var dLinkFactor = Factors["L_" + EXT_LINK_TYPES[k] + "_"][guids[0]].val;
                        res.push(bFontsNumber / bLinkFactor);
                        names.push(setFactor + "/" + "L_" + EXT_LINK_TYPES[k] + "_");
                        res.push((bFontsNumber * dLinkFactor) / (bLinkFactor * dFontsNumber));
                        names.push(setFactor + "/" + "L_" + EXT_LINK_TYPES[k] + "_" + "/" + "guid_0");
                    }
                }
                var fName = "F" + FONT_SUBSCRIPTS[i] + "Set";
                var fIName = "F" + FONT_SUBSCRIPTS[i] + "iSet";
                var bFontsNumber = Factors[fName][guid].val.length;
                var dFontsNumber = Factors[fName][guids[0]].val.length;
                var bInlineFontsNumber = Factors[fIName][guid].val.length;
                var dInlineFontsNumber = Factors[fIName][guids[0]].val.length;
                res.push(bInlineFontsNumber / bFontsNumber);
                names.push(fIName + "/" + setFactor);
                res.push((bInlineFontsNumber * dFontsNumber) / (bFontsNumber * dInlineFontsNumber));
                names.push(fIName + "/" + setFactor + "/" + "guid_0");
                if (NUMERIC_FONT_SUBSCRIPTS.indexOf(FONT_SUBSCRIPTS[i]) >= 0) {
                    var hashFactor = "F" + FONT_SUBSCRIPTS[i] + "cHash";
                    var setFactor = "F" + FONT_SUBSCRIPTS[i] + "Set";
                    var bFreq = Stats.freq(Factors[hashFactor][guid].val);
                    var dFreq = Stats.freq(Factors[hashFactor][guids[0]].val);
                    var max = Stats.max(Factors[setFactor][guid].val);
                    var min = Stats.min(Factors[setFactor][guid].val);
                    res.push((min < bFreq? 1: 0));
                    names.push("min" + setFactor + "<" + "freq" + hashFactor);
                    res.push((min < dFreq? 1: 0));
                    names.push("min" + setFactor + "<" + "freq" + hashFactor + "_guid_0");
                    res.push((max > bFreq? 1: 0));
                    names.push("max" + setFactor + "<" + "freq" + hashFactor);
                    res.push((max > dFreq? 1: 0));
                    names.push("max" + setFactor + "<" + "freq" + hashFactor + "_guid_0");
                }
            }
            return [res, names];
        },
        treeFactorsArray: function (guid) {
            var res = [];
            var names = [];
            res.push(rootDists.all[guid]);
            names.push("rootDists");
            res.push(rootDists.all[guid] / (Stats.average(depths.all[guids[0]].val) + rootDists.all[guids[0]]));
            names.push("rootDists" + "/" + "(" + "avgDepths_guid_0" + "+" + "rootDicts_guid_0" + ")");
            res.push(rootDists.all[guid] / (Stats.median(depths.all[guids[0]].val) + rootDists.all[guids[0]]));
            names.push("rootDists" + "/" + "(" + "medDepths_guid_0" + "+" + "rootDicts_guid_0" + ")");
            res.push(rootDists.all[guid] / (Stats.max(depths.all[guids[0]].val) + rootDists.all[guids[0]]));
            names.push("rootDists" + "/" + "(" + "maxDepths_guid_0" + "+" + "rootDicts_guid_0" + ")");
            var depthKeys = ["block", "all"];
            for (var i = 0; i < depthKeys.length; ++i) {
                res.push(Stats.max(depths[depthKeys[i]][guid].val));
                names.push("maxDepths" + depthKeys[i]);
                res.push(Stats.average(depths[depthKeys[i]][guid].val));
                names.push("avgDepths" + depthKeys[i]);
                res.push(Stats.median(depths[depthKeys[i]][guid].val));
                names.push("medDepths" + depthKeys[i]);
            }
            for (var i = 0; i < depthKeys.length; ++i) {
                res.push(rootDists.all[guid] / Stats.max(depths[depthKeys[i]][guid].val));
                names.push("rootDists" + "/" + "maxDepths" + depthKeys[i]);
                res.push(rootDists.all[guid] / Stats.median(depths[depthKeys[i]][guid].val));
                names.push("rootDists" + "/" + "medDepths" + depthKeys[i]);
                res.push(rootDists.all[guid] / Stats.average(depths[depthKeys[i]][guid].val));
                names.push("rootDists" + "/" + "avgDepths" + depthKeys[i]);
            }
            for (var i = 0; i < TAG_SET_1.length; ++i) {
                var dist = tagSet1Dists[guid][TAG_SET_1[i]];
                if (typeof(dist) == "undefined") {
                    res.push(0);
                    res.push(NaN);
                    res.push(NaN);
                } else {
                    res.push(1);
                    res.push(dist);
                    res.push(rootDists.all[guid] - dist);
                }
                names.push("undef?");
                names.push("dist_" + TAG_SET_1[i]);
                names.push("rootDists" + "-" + "dist_" + TAG_SET_1[i]);
            }
            if (typeof(tableFactors[guid]) != "undefined") {
                res = res.concat(tableFactors[guid]);
            } else {
                for (var i = 0; i < 12; ++i) {
                    res.push(0);
                }
            }
            for (var i = 0; i < 12; ++i) {
                names.push("tableFactor_" + i);
            }
            var layout = treeLayoutFactors[guid];
            res.push(layout.siblings_count);
            names.push("siblings");
            var textSiblings = layout.siblings_count;
            var elemKinds = ["blocks", "nonBlockElements"];
            for (var i = 0; i < elemKinds.length; ++i) {
                res.push(layout["siblings_" + elemKinds[i]]);
                names.push("siblings_" + elemKinds[i]);
                res.push(layout["siblings_" + elemKinds[i]] / layout.siblings_count);
                names.push("siblings_" + elemKinds[i] + "/" + "siblings");
                textSiblings -= layout["siblings_" + elemKinds[i]];
            }
            res.push(textSiblings);
            names.push("textSiblings");
            res.push(textSiblings / layout.siblings_count);
            names.push("textSiblings" + "/" + "siblings");
            res.push(layout.position);
            names.push("position");
            res.push(layout.isFirst);
            names.push("first?");
            res.push(layout.isLast);
            names.push("last?");
            var texts = new ArrayFactor();
            texts.update(layout.siblings_texts.val);
            texts.update([Factors.Sc[guid].val]);
            res.push(Factors.Sc[guid].val / Stats.average(texts.val));
            names.push("FactorsSc" + "/" + "avgtexts");
            res.push(Factors.Sc[guid].val / Stats.median(texts.val));
            names.push("FactorsSc" + "/" + "medtexts");
            res.push(Factors.Sc[guid].val / Stats.sum(texts.val));
            names.push("FactorsSc" + "/" + "sumtexts");
            for (var i = 0; i < TAG_SET_2.length; ++i) {
                res.push(layout.siblings_tagSet2Counts[TAG_SET_2[i]]);
                names.push("siblings_tagSet2Count" + TAG_SET_2[i]);
                res.push(layout.siblings_tagSet2Counts[TAG_SET_2[i]] / layout.siblings_count);
                names.push("siblings_tagSet2Count" + TAG_SET_2[i] + "/" + "siblings");
            }
            var textChildren = layout.children_count;
            for (var i = 0; i < elemKinds.length; ++i) {
                res.push(layout["children_" + elemKinds[i]]);
                names.push("children_" + elemKinds[i]);
                res.push(layout["children_" + elemKinds[i]] / layout.children_count);
                names.push("children_" + elemKinds[i] + "/" + "children");
                textChildren -= layout["children_" + elemKinds[i]];
            }
            res.push(textChildren);
            names.push("textChildren");
            res.push(textChildren / layout.children_count);
            names.push("textChildren" + "/" + "children");
            var children = new ArrayFactor();
            children.update(layout.siblings_children.val);
            children.update([layout.children_count]);
            res.push(layout.children_count / Stats.average(children.val));
            names.push("children" + "/" + "avgChildren");
            res.push(layout.children_count / Stats.median(children.val));
            names.push("children" + "/" + "medChildren");
            res.push(layout.children_count / Stats.max(children.val));
            names.push("children" + "/" + "maxChildren");
            var grandChildrenAvg = Stats.average(layout.children_children.val);
            res.push(grandChildrenAvg);
            names.push("avgGrandChildren");
            res.push(Stats.median(layout.children_children.val));
            names.push("medChildren_children");
            res.push(Stats.max(layout.children_children.val));
            names.push("maxChildren_children");
            res.push(Stats.variance(layout.children_children.val, grandChildrenAvg));
            names.push("varChildren_children");
            for (var i = 0; i < TAG_SET_2.length; ++i) {
                res.push(layout.children_tagSet2Counts[TAG_SET_2[i]]);
                names.push("children_tagSet2Count" + TAG_SET_2[i]);
            }
            for (var i = 0; i < TAG_SET_2.length; ++i) {
                res.push(layout.grandChildrenTagSet2[TAG_SET_2[i]]);
                names.push("grandChildren_tagSet2" + TAG_SET_2[i]);
            }
            for (var i = 0; i < TAG_SET_2.length; ++i) {
                res.push(layout.children_tagSet2Counts[TAG_SET_2[i]] / layout.children_count);
                names.push("children_tagSet2Count" + TAG_SET_2[i] + "/" + "children");
            }
            for (var i = 0; i < TAG_SET_2.length; ++i) {
                res.push(layout.grandChildrenTagSet2[TAG_SET_2[i]] / Stats.sum(layout.children_children.val));
                names.push("grandChildren_tagSet2" + TAG_SET_2[i] + "/" + "sumChildren_children");
            }
            for (var i = 0; i < TAG_SET_2.length; ++i) {
                res.push(layout.subtreeTagSet2[TAG_SET_2[i]] / layout.subtreeBlocks);
                names.push("subTree_tagSet2" + TAG_SET_2[i] + "/" + "subtreeBlocks");
            }
            var childrenTextsAvg = Stats.average(layout.children_texts.val);
            res.push(Stats.variance(layout.children_texts.val, childrenTextsAvg));
            names.push("varChildren_texts");
            res.push(layout.self_count);
            names.push("self_count");
            var selfTexts = layout.self_count;
            for (var i = 0; i < elemKinds.length; ++i) {
                res.push(layout["self_" + elemKinds[i]]);
                names.push("self_" + elemKinds[i]);
                res.push(layout["self_" + elemKinds[i]] / layout.self_count);
                names.push("self_" + elemKinds[i] + "/" + "self_count");
                selfTexts -= layout["self_" + elemKinds[i]];
            }
            res.push(selfTexts);
            names.push("selfTexts");
            res.push(selfTexts / layout.self_count);
            names.push("selfTexts" + "/" + "self_count");
            for (var i = 0; i < TAG_SET_2.length; ++i) {
                res.push(layout.self_tagSet2Counts[TAG_SET_2[i]]);
                names.push("self_tagSet2Counts" + TAG_SET_2[i]);
            }
            for (var i = 0; i < TAG_SET_2.length; ++i) {
                res.push(layout.self_tagSet2Counts[TAG_SET_2[i]] / layout.self_count);
                names.push("self_tagSet2Counts" + TAG_SET_2[i] + "/" + "self_count");
            }
            return [res, names];
        },
        groupQuotients: function (guid, prefix, groups) {
            var res = [];
            for (var i = 0; i < groups.length; ++i) {
                res.push(Factors[prefix + groups[i].charAt(0)][guid].val /
                         Factors[prefix + groups[i].charAt(1)][guid].val);
            }
            return res;
        },
        medians: function (guid, prefix, subscripts) {
            var res = [];
            for (var i = 0; i < subscripts.length; ++i) {
                res.push(Stats.median(Factors[prefix + subscripts[i] + "Arr"][guid].val));
            }
            return res;
        },
    };

    TextExtraction = {
        paragraphs: function(elem, guid) {
            var res = [];
            var pText = "";
            var containmentStack = [];
            var containment = [];
            var currentContainment = [];
            var stack = [];
            var links = [];
            var chunks = {};
            var currentChunk = {};
            var iChunks = {};
            var currentIChunk = {};
            for (var i = 0; i < FONT_SUBSCRIPTS.length; ++i) {
                chunks[FONT_SUBSCRIPTS[i]] = [];
                iChunks[FONT_SUBSCRIPTS[i]] = [];
                currentChunk[FONT_SUBSCRIPTS[i]] = [];
                currentIChunk[FONT_SUBSCRIPTS[i]] = [];
            }
            for (var i = elem.length - 1; i >= 0; --i) {
                stack.push(elem[i]);
            }
            while (stack.length) {
                var node = stack.pop();
                if (node.nodeType == "LINK_END") {
                    var linkIdx = containmentStack.pop();
                    links[linkIdx].endParagraph = res.length;
                    links[linkIdx].endOffset = pText.replace(/\s+/g, " ").replace(/^\s/g, "").length;
                    var beginParagraph;
                    if (links[linkIdx].beginParagraph < res.length) {
                        beginParagraph = res[links[linkIdx].beginParagraph];
                    } else {
                        beginParagraph = pText.replace(/\s+/g, " ").replace(/^\s/g, "");
                    }
                    if (
                        (
                            links[linkIdx].beginParagraph < res.length ||
                            links[linkIdx].beginOffset != links[linkIdx].endOffset
                        ) &&
                        beginParagraph.charAt(links[linkIdx].beginOffset) == " "
                    ) {
                        ++links[linkIdx].beginOffset;
                    }
                    continue;
                }
                if (node.nodeType != Node.TEXT_NODE && node.nodeType != Node.ELEMENT_NODE) {
                    continue;
                }
                if (node.nodeType == Node.TEXT_NODE) {
                    if (mode != "sentences" && jQuery.trim(node.nodeValue).length) {
                        NodesProcessing.addDepth($(node), elem, guid);
                    }
                    var textParent = $(node).parent();
                    var chunkBegin = pText.replace(/\s+/g, " ").replace(/^\s/g, "").length;
                    var newPText = (pText + node.nodeValue).replace(/\s+/g, " ").replace(/^\s/g, "");
                    var chunkEnd = newPText.length;
                    if (chunkEnd - chunkBegin) {
                        for (var key in SUBSCRIPTS_2_CSS) {
                            var fontProp;
                            if (NON_INHERITED_FONT_SUBSCRIPTS.indexOf(key) >= 0) {
                                fontProp = TextExtraction.nonInheritedCss(textParent, key);
                            } else {
                                fontProp = textParent.css(SUBSCRIPTS_2_CSS[key]);
                            }
                            if (jQuery.trim(fontProp) != "") {
                                fontProp = fontProp.toString().toLowerCase().replace(/\s*,\s*/g, ",");
                                if (NUMERIC_FONT_SUBSCRIPTS.indexOf(key) >= 0) {
                                    fontProp = parseInt(fontProp.replace("px", ""));
                                }
                                var newChunk = {
                                    begin: chunkBegin,
                                    end: chunkEnd,
                                    prop: fontProp,
                                };
                                TextExtraction.addChunk(key, newChunk, currentChunk);
                                if (
                                    textParent.css("display") == "inline" && (
                                        (
                                            NON_INHERITED_FONT_SUBSCRIPTS.indexOf(key) >= 0 &&
                                            TextExtraction.nonInheritedCss(textParent, key) !=
                                            TextExtraction.nonInheritedCss(textParent.parent(), key)
                                        ) ||
                                        (
                                            NON_INHERITED_FONT_SUBSCRIPTS.indexOf(key) == -1 &&
                                            textParent.css(SUBSCRIPTS_2_CSS[key]) !=
                                            textParent.parent().css(SUBSCRIPTS_2_CSS[key])
                                        )
                                    )
                                ) {
                                    TextExtraction.addChunk(key, newChunk, currentIChunk);
                                }
                            }
                        }
                    }
                    pText += node.nodeValue;
                    continue;
                }
                var tagName = $(node).prop("tagName").toLowerCase();
                if (
                    tagName == "br" ||
                    PARAGRAPH_BORDERS.indexOf(tagName) >= 0 ||
                    BLOCK_TAGS.indexOf(tagName) >= 0
                ) {
                    pText = jQuery.trim(pText.replace(/\s+/g, " "));
                    if (pText != "") {
                        res.push(pText);
                        TextExtraction.cutLastChunk(currentChunk, pText.length);
                        TextExtraction.cutLastChunk(currentIChunk, pText.length);
                        for (var key in currentChunk) {
                            chunks[key].push(currentChunk[key].slice(0));
                            currentChunk[key] = [];
                        }
                        for (var key in currentIChunk) {
                            iChunks[key].push(currentIChunk[key].slice(0));
                            currentIChunk[key] = [];
                        }
                        containment.push(currentContainment.slice(0));
                        pText = "";
                    }
                    currentContainment = containmentStack.slice(0);
                }
                if (
                    PARAGRAPH_BORDERS.indexOf(tagName) >= 0 ||
                    BLOCK_TAGS.indexOf(tagName) >= 0
                ) {
                    stack.push($("<br />")[0]);
                }
                if (tagName == "a" && typeof($(node).attr("href")) != "undefined") {
                    stack.push({
                        nodeType: "LINK_END",
                    });
                    currentContainment.push(links.length);
                    containmentStack.push(links.length);
                    var linkType = LinkProcessing.linkType($(node));
                    links.push({
                        type: linkType,
                        beginParagraph: res.length,
                        beginOffset: pText.replace(/\s+/g, " ").replace(/^\s/g, "").length,
                    });
                }
                if (!$(node).is(":visible")) {
                    continue;
                }
                if (tagName == "iframe") {
                    if (mode != "sentences") {
                        NodesProcessing.addDepth($(node), elem, guid);
                    }
                    continue;
                }
                var contents = [];
                try {
                    contents = $(node).contents();
                } catch (e) {
                }
                if (mode != "sentences" && (!contents.length || !(
                    $(node).children().length || jQuery.trim($(node).text()).length
                ))) {
                    NodesProcessing.addDepth($(node), elem, guid);
                }
                for (var i = contents.length - 1; i >= 0; --i) {
                    var type = contents[i].nodeType;
                    if (type == Node.TEXT_NODE || type == Node.ELEMENT_NODE) {
                        stack.push(contents[i]);
                    }
                }
            }
            pText = jQuery.trim(pText.replace(/\s+/g, " "));
            if (pText != "") {
                res.push(pText);
                TextExtraction.cutLastChunk(currentChunk, pText.length);
                TextExtraction.cutLastChunk(currentIChunk, pText.length);
                for (var key in currentChunk) {
                    chunks[key].push(currentChunk[key].slice(0));
                }
                for (var key in currentIChunk) {
                    iChunks[key].push(currentIChunk[key].slice(0));
                }
                containment.push(currentContainment.slice(0));
            }
            return {
                pArray: res,
                links: links,
                containment: containment,
                chunks: chunks,
                ichunks: iChunks,
            };
        },
        nonInheritedCss: function(elem, key) {
            var ancestor = elem;
            var set = new SetFactor();
            do {
                var cssProp = ancestor.css(SUBSCRIPTS_2_CSS[key]);
                var propValues = [];
                if (cssProp) {
                    propValues = cssProp.toLowerCase().split(/\s+/g);
                }
                for (var i = 0; i < propValues.length; ++i) {
                    if (set.val.length == 0) {
                        set.update([propValues[i]]);
                    } else {
                        if (propValues[i] != "none") {
                            if (set.val[0] == "none") {
                                set.val.shift();
                            }
                            set.update([propValues[i]]);
                        }
                    }
                }
                ancestor = ancestor.parent();
            } while (
                ancestor[0].nodeType == Node.ELEMENT_NODE &&
                ancestor.prop("tagName").toLowerCase() != "body" &&
                NON_INHERITED_BREAKERS[key].indexOf(ancestor.css("display")) == -1
            );
            return set.val.join(" ");
        },
        addChunk: function (key, chunk, chunkSet) {
            if (chunkSet[key].length) {
                var lastChunk = chunkSet[key][chunkSet[key].length - 1];
                if (lastChunk.prop == chunk.prop && lastChunk.end == chunk.begin) {
                    chunkSet[key][chunkSet[key].length - 1].end = chunk.end;
                } else {
                    chunkSet[key].push(chunk);
                }
            } else {
                chunkSet[key].push(chunk);
            }
        },
        cutLastChunk: function (chunkSet, cutLength) {
            for (var key in chunkSet) {
                if (chunkSet[key].length) {
                    var lastChunk = chunkSet[key][chunkSet[key].length - 1];
                    if (lastChunk.end > cutLength) {
                        lastChunk.end = cutLength;
                        if (lastChunk.end - lastChunk.begin <= 0) {
                            chunkSet[key].pop();
                        }
                    }
                }
            }
        },
        sentences: function (parInfo, idx) {
            var paragraph = parInfo.pArray[idx];
            var indexes = {
                link: 0,
            };
            var split = paragraph.split(/([\.?!] )/g);
            var res = {
                bounds: [],
            };
            for (var i = 0; i < LINK_TYPES.length; ++i) {
                res[LINK_TYPES[i]] = 0;
            }
            for (var i = 0; i < EXT_LINK_TYPES.length; ++i) {
                res[EXT_LINK_TYPES[i]] = 0;
            }
            for (var i = 0; i < FONT_SUBSCRIPTS.length; ++i) {
                indexes[FONT_SUBSCRIPTS[i]] = 0;
                indexes[FONT_SUBSCRIPTS[i] + "i"] = 0;
                res[FONT_SUBSCRIPTS[i] + "Hash"] = [];
                res[FONT_SUBSCRIPTS[i] + "iHash"] = [];
            }
            var sText = "";
            var read = "";
            var curBegin = 0;
            for (var i = 0; i < split.length; ++i) {
                read += split[i];
                if (split[i].match(/^[\.?!] $/)) {
                    if (i == split.length - 1) {
                        sText = jQuery.trim(sText);
                        if (sText != "") {
                            TextExtraction.placeSentence(curBegin, read, parInfo, idx, indexes, res);
                            curBegin = read.length;
                            sText = "";
                        }
                        break;
                    }
                    var firstChar = split[i + 1].charAt(0);
                    if (firstChar == firstChar.toLocaleUpperCase()) {
                        sText = jQuery.trim(sText);
                        if (sText != "") {
                            TextExtraction.placeSentence(curBegin, read, parInfo, idx, indexes, res);
                            curBegin = read.length;
                            sText = "";
                        }
                    } else {
                        sText += split[i];
                    }
                } else {
                    sText += split[i];
                }
            }
            sText = jQuery.trim(sText);
            if (sText != "") {
                TextExtraction.placeSentence(curBegin, read, parInfo, idx, indexes, res);
            }
            return res;
        },
        placeSentence: function (curBegin, read, parInfo, idx, indexes, res) {
            var sBounds = {
                begin: curBegin,
                end: jQuery.trim(read).length,
            };
            for (var i = 0; i < LINK_TYPES.length; ++i) {
                sBounds[LINK_TYPES[i]] = 0;
            }
            var sLinks = {e: 0, i: 0, s: 0};
            for (var i = indexes.link; i < parInfo.containment[idx].length; ++i) {
                var link = parInfo.links[parInfo.containment[idx][i]];
                if (link.beginParagraph < idx || link.beginOffset <= sBounds.begin) {
                    if (link.endParagraph > idx || link.endOffset >= sBounds.end) {
                        ++res[link.type];
                        ++sLinks[link.type];
                        ++sBounds[link.type];
                    } else {
                        if (link.endOffset > sBounds.begin) {
                            ++sBounds[link.type];
                        }
                        ++indexes.link;
                    }
                } else {
                    if (link.beginOffset < sBounds.end) {
                        ++sBounds[link.type];
                    } else {
                        break;
                    }
                }
            }
            for (var i = 0; i < EXT_LINK_TYPES.length; ++i) {
                for (var j = 0; j < EXT_LINK_TYPES[i].length; ++j) {
                    if (sLinks[EXT_LINK_TYPES[i].charAt(j)]) {
                        ++res[EXT_LINK_TYPES[i]];
                        break;
                    }
                }
            }
            for (var i = 0; i < INLINE_MODIFIERS.length; ++i) {
                for (var j = 0; j < FONT_SUBSCRIPTS.length; ++j) {
                    var key = FONT_SUBSCRIPTS[j] + INLINE_MODIFIERS[i];
                    var hashName = key + "Hash";
                    var chunks = parInfo[INLINE_MODIFIERS[i] + "chunks"][FONT_SUBSCRIPTS[j]][idx];
                    for (var k = indexes[key]; k < chunks.length; ++k) {
                        var chunk = chunks[k];
                        if (chunk.begin <= sBounds.begin) {
                            if (chunk.end >= sBounds.end) {
                                var fontProp = chunk.prop;
                                if (typeof(res[hashName][fontProp]) == "undefined") {
                                    res[hashName][fontProp] = 1;
                                } else {
                                    ++res[hashName][fontProp];
                                }
                            } else {
                                ++indexes[key];
                            }
                        } else {
                            break;
                        }
                    }
                }
            }
            res.bounds.push(sBounds);
        },
        wordCount: function (parInfo, idx, sBounds) {
            var res = {
                counts: {
                    w: 0,
                },
                hashes: {
                },
            };
            for (var i = 0; i < LINK_TYPES.length; ++i) {
                res.counts[LINK_TYPES[i]] = 0;
            }
            for (var i = 0; i < EXT_LINK_TYPES.length; ++i) {
                res.counts[EXT_LINK_TYPES[i]] = 0;
            }
            var sentence = parInfo.pArray[idx].slice(sBounds.begin, sBounds.end);
            var indexes = {
                link: 0,
            };
            for (var i = 0; i < FONT_SUBSCRIPTS.length; ++i) {
                indexes[FONT_SUBSCRIPTS[i]] = 0;
                indexes[FONT_SUBSCRIPTS[i] + "i"] = 0;
                res.hashes[FONT_SUBSCRIPTS[i]] = [];
                res.hashes[FONT_SUBSCRIPTS[i] + "i"] = [];
            }
            var split = sentence.split(/(\s)/g);
            var curBegin = sBounds.begin;
            var read = "";
            for (var i = 0; i < split.length; ++i) {
                read += split[i];
                if (split[i] == " " ||
                    split[i].replace(RegExps.Trash, "").replace(RegExps.Punct, "") == "") {
                    curBegin = read.length + sBounds.begin;
                    continue;
                }
                var word = split[i];
                var wBounds = {
                    begin: curBegin,
                    end: read.length + sBounds.begin,
                }
                while (word.charAt(0).match(RegExps.Trash) || word.charAt(0).match(RegExps.Punct)) {
                    word = word.substr(1);
                    ++wBounds.begin;
                }
                while (word.substr(-1).match(RegExps.Trash) || word.substr(-1).match(RegExps.Punct)) {
                    word = word.slice(0, -1);
                    --wBounds.end;
                }
                var wordLinks = {e: 0, i: 0, s: 0};
                for (var j = indexes.link; j < parInfo.containment[idx].length; ++j) {
                    var link = parInfo.links[parInfo.containment[idx][j]];
                    if (link.beginParagraph < idx || link.beginOffset <= wBounds.begin) {
                        if (link.endParagraph > idx || link.endOffset >= wBounds.end) {
                            ++res.counts[link.type];
                            ++wordLinks[link.type];
                        } else {
                            ++indexes.link;
                        }
                    } else {
                        break;
                    }
                }
                for (var j = 0; j < EXT_LINK_TYPES.length; ++j) {
                    for (var k = 0; k < EXT_LINK_TYPES[j].length; ++k) {
                        if (wordLinks[EXT_LINK_TYPES[j].charAt(k)]) {
                            ++res.counts[EXT_LINK_TYPES[j]];
                            break;
                        }
                    }
                }
                for (var j = 0; j < INLINE_MODIFIERS.length; ++j) {
                    for (var k = 0; k < FONT_SUBSCRIPTS.length; ++k) {
                        var chunks = parInfo[INLINE_MODIFIERS[j] + "chunks"][FONT_SUBSCRIPTS[k]][idx];
                        var key = FONT_SUBSCRIPTS[k] + INLINE_MODIFIERS[j];
                        var hash = res.hashes[key];
                        for (var l = indexes[key]; l < chunks.length; ++l) {
                            if (chunks[l].begin <= wBounds.begin) {
                                if (chunks[l].end >= wBounds.end) {
                                    var fontProp = chunks[l].prop;
                                    if (typeof(hash[fontProp]) == "undefined") {
                                        hash[fontProp] = 1;
                                    } else {
                                        ++hash[fontProp];
                                    }
                                } else {
                                    ++indexes[key];
                                }
                            } else {
                                break;
                            }
                        }
                    }
                }
                ++res.counts.w;
                curBegin = read.length + sBounds.begin;
            }
            return res;
        },
        calculateSentences: function (elem, guid) {
            guids.push(guid);
            var res = [];
            var parInfo = TextExtraction.paragraphs(elem, guid);
            var paragraphs = parInfo.pArray;
            for (var i = 0; i < paragraphs.length; ++i) {
                var sentences = TextExtraction.sentences(parInfo, i);
                for (var j = 0; j < sentences.bounds.length; ++j) {
                    var sentence = paragraphs[i].slice(sentences.bounds[j].begin, sentences.bounds[j].end);
                    res.push(sentence);
                }
            }
            calculatedSentences[guid] = res;
        },
    };

    CharExtraction = {
        charStats: function (str) {
            var letters = str.match(RegExps.Letter) || [];
            var uppercaseCount = 0;
            for (var i = 0; i < letters.length; ++i) {
                if (letters[i] == letters[i].toLocaleUpperCase()) {
                    ++uppercaseCount;
                }
            }
            return {
                a: letters.length,
                u: uppercaseCount,
                d: (str.match(RegExps.Digit) || []).length,
                p: (str.match(RegExps.Punct) || []).length,
                t: (str.match(RegExps.Trash) || []).length,
                q: (str.match(RegExps.Quote) || []).length,
                b: (str.match(RegExps.Parentheses) || []).length,
                s: (str.match(RegExps.Brackets) || []).length,
                h: (str.match(RegExps.Dash) || []).length,
                e: (str.match(RegExps.EndPunct) || []).length,
                m: (str.match(RegExps.MiddlePunct) || []).length,
            };
        },
    };

    LinkProcessing = {
        linkType: function (link) {
            var url = link.prop("href");
            if (
                url.slice(0, "mailto:".length).toLowerCase() == "mailto:" ||
                url.slice(0, "skype:".length).toLowerCase() == "skype:"
            ) {
                return "m";
            }
            if (url.slice(0, "javascript:".length).toLowerCase() == "javascript:") {
                return "a";
            }
            if (
                url.slice(0, "steam:".length).toLowerCase() == "steam:"
            ) {
                return "e";
            }
            var hd = LinkProcessing.hostAndDomain(url);
            if (url.replace(RegExps.Anchor, "") == baseUrl) {
                if (typeof(link.attr("onclick")) != "undefined") {
                    return "a";
                }
                if (
                    hd.anchor.length && hd.anchor != "#" &&
                    $("#html_page").contents().find("a[link=\"" + hd.anchor.slice(1) + "\"]").length
                ) {
                    return "l";
                } else {
                    return "a";
                }
            }
            if (hd.host == baseHostAndDomain.host) {
                if (hd.port == baseHostAndDomain.port) {
                    return "i";
                } else {
                    return "s";
                }
            }
            if (hd.domain == baseHostAndDomain.domain) {
                return "s";
            }
            return "e";
        },
        hostAndDomain: function (url) {
            var match = url.match(RegExps.Url);
            if (!match && url.substr(0, "http:".length) == "http:") {
                return {
                    host: "",
                    domain: "",
                    port: "",
                    anchor: "",
                };
            }
            return {
                host: match[1],
                domain: match[2],
                port: match[3] || "",
                anchor: match[4] || "",
            };
        },
    };

    NodesProcessing = {
        calculateDistance: function (elem, guid) {
            var all = 0;
            var block = 0;
            tagSet1Dists[guid] = [];
            var tagName = elem.prop("tagName");
            if (tagName) {
                tagName = tagName.toLowerCase();
            }
            while (tagName != "body") {
                if (TAG_SET_1.indexOf(tagName) >= 0) {
                    if (typeof(tagSet1Dists[guid][tagName]) == "undefined") {
                        tagSet1Dists[guid][tagName] = all;
                    }
                }
                ++all;
                if (BLOCK_TAGS.indexOf(tagName) >= 0) {
                    ++block;
                }
                elem = elem.parent();
                tagName = elem.prop("tagName").toLowerCase();
                var parGuid = elem.attr("data-guid");
                if (typeof(parGuid) != "undefined" && guids.indexOf(parGuid) >= 0) {
                    for (var i = 0; i < TAG_SET_1.length; ++i) {
                        if (
                            typeof(tagSet1Dists[guid][TAG_SET_1[i]]) == "undefined" &&
                            typeof(tagSet1Dists[parGuid][TAG_SET_1[i]]) != "undefined"
                        ) {
                            tagSet1Dists[guid][TAG_SET_1[i]] = all + tagSet1Dists[parGuid][TAG_SET_1[i]];
                        }
                    }
                    all += rootDists.all[parGuid];
                    block += rootDists.block[parGuid];
                    break;
                }
            }
            rootDists.all[guid] = all;
            rootDists.block[guid] = block;
            depths.all[guid] = new ArrayFactor();
            depths.block[guid] = new ArrayFactor();
        },
        addDepth: function (elem, block, guid) {
            var dists = NodesProcessing.distancesToParent(elem, block);
            for (distKey in dists) {
                depths[distKey][guid].update([dists[distKey]]);
            }
        },
        distancesToParent: function (elem, parents) {
            var all = 0;
            var block = 0;
            while (parents.index(elem) == -1) {
                ++all;
                var tagName = elem.prop("tagName");
                if (tagName && BLOCK_TAGS.indexOf(tagName.toLowerCase()) >= 0) {
                    ++block;
                }
                elem = elem.parent();
            }
            return {
                all: all,
                block: block,
            };
        },
        updateDepths: function (elem, guid) {
            var parents = elem.parents();
            for (depthKey in depths) {
                var curGuid = guid;
                var upd = depths[depthKey][guid].val.slice(0);
                for (var i = 0; i < parents.length; ++i) {
                    var parentGuid = $(parents[i]).attr("data-guid");
                    if (typeof(parentGuid) != "undefined" && guids.indexOf(parentGuid) >= 0) {
                        var depthDiff = rootDists[depthKey][curGuid] - rootDists[depthKey][parentGuid];
                        for (var j = 0; j < upd.length; ++j) {
                            upd[j] += depthDiff;
                        }
                        depths[depthKey][parentGuid].update(upd);
                        curGuid = parentGuid;
                    }
                }
            }
        },
        calculateNonPropagatedNodeFactors: function (elem, guid) {
            if (
                elem.length == 1 && elem.prop("tagName") &&
                (
                    TABLE_TAGS.indexOf(elem.prop("tagName").toLowerCase()) >= 0 || (
                        elem.prop("tagName").toLowerCase() == "tr" &&
                        guids.indexOf($(elem.parents("tbody,thead")[0]).attr("data-guid")) == -1 &&
                        guids.indexOf($(elem.parents("table")[0]).attr("data-guid")) == -1
                    )
                )
            ) {
                NodesProcessing.calculateTableFactors(elem, guid);
            }
            NodesProcessing.calculateTreeLayoutFactors(elem, guid);
        },
        calculateTableFactors: function (elem, guid) {
            var tdCounts = new HashFactor();
            var tdDepths = new ArrayFactor();
            var tdLengths = new ArrayFactor();
            var tdImages = new ArrayFactor();
            var totalTds = 0;
            var colspan = 0;
            var rowspan = 0;
            var trs = (elem.prop("tagName").toLowerCase() == "tr"? elem: elem.children("tr"));
            if (!trs.length && elem.children("thead,tbody").length) {
                trs = elem.children("thead,tbody").children("tr");
            }
            for (var i = 0; i < trs.length; ++i) {
                var tdCount = 0;
                var hash = {};
                var tds = $(trs[i]).children("td,th");
                totalTds += tds.length;
                for (var j = 0; j < tds.length; ++j) {
                    var td = $(tds[j]);
                    if (td.attr("rowspan")) {
                        rowspan = 1;
                    }
                    if (td.attr("colspan")) {
                        colspan = 1;
                        tdCount += parseInt(td.attr("colspan"));
                    } else {
                        ++tdCount;
                    }
                    tdDepths.update([NodesProcessing.maxDepth(td[0])]);
                    tdLengths.update([jQuery.trim(td.text().replace(/\s+/g, " ")).length]);
                    tdImages.update([td.find("img").length]);
                }
                hash[tdCount] = 1;
                tdCounts.update(hash);
            }
            var cols = Stats.freq(tdCounts.val);
            var tdDepthsAvg = Stats.average(tdDepths.val);
            var tdLengthsAvg = Stats.average(tdLengths.val);
            var tdImagesAvg = Stats.average(tdImages.val);
            var noTextTds = 0;
            while (noTextTds < tdLengths.val.length && !tdLengths.val[noTextTds]) {
                ++noTextTds;
            }
            var noImgTds = 0;
            while (noImgTds < tdImages.val.length && !tdImages.val[noImgTds]) {
                ++noImgTds;
            }
            tableFactors[guid] = [
                trs.length,
                cols,
                totalTds,
                (cols == 2? 1: 0),
                colspan,
                rowspan,
                Stats.variance(tdDepths.val, tdDepthsAvg),
                Stats.variance(tdLengths.val, tdLengthsAvg),
                Stats.variance(tdImages.val, tdImagesAvg),
                noTextTds,
                noImgTds,
                (elem.find("table").length? 1: 0),
            ];
        },
        calculateMergedBlocksTableFactors: function(mergedGuids, mergeId) {
            var trs = $();
            for (var i = 0; i < mergedGuids.length; ++i) {
                var elem = elems[mergedGuids[i]];
                if (elem.length != 1) {
                    return;
                }
                var tagName;
                try {
                    tagName = elem.prop("tagName").toLowerCase();
                } catch (e) {
                    return;
                }
                if (tagName == "tr") {
                    trs = trs.add(elem);
                    continue;
                }
                if (tagName == "thead" || tagName == "tbody") {
                    trs = trs.add(elem.children("tr").filter(NodesProcessing.filterFunc));
                    continue;
                }
                return;
            }
            var trParent = trs.parent();
            if (
                trParent.children("tr").filter(NodesProcessing.filterFunc).length == trs.length && (
                    trParent.length == 1 &&
                    TABLE_TAGS.indexOf(trParent.prop("tagName").toLowerCase()) >= 0
                ) || (
                    trParent.length == 2 &&
                    trParent.filter(
                        function () {
                            var tagName = this.nodeName.toLowerCase();
                            return tagName == "thead" || tagName == "tbody"
                        }
                    ).length == 2 &&
                    trParent.parent().length == 1
                )
            ) {
                NodesProcessing.calculateTableFactors(trs, mergeId);
            }
        },
        filterFunc: function () {
            return (
                this.nodeType == Node.ELEMENT_NODE && TreeProcessing.visibleAndBig($(this))
            ) || (
                this.nodeType == Node.TEXT_NODE && jQuery.trim(this.nodeValue).length
            );
        },
        calculateTreeLayoutFactors: function (elem, guid) {
            var sameLevel = elem.parent().contents().filter(
                function () {
                    return elem.index(this) >= 0 || (
                        (this.nodeType == Node.ELEMENT_NODE && TreeProcessing.visibleAndBig($(this))) ||
                        (this.nodeType == Node.TEXT_NODE && jQuery.trim(this.nodeValue).length)
                    );
                }
            );
            var position = sameLevel.index(elem[0]);
            var res = {
                isFirst: (position? 0: 1),
                isLast: (sameLevel.index(elem[elem.length - 1]) == sameLevel.length - 1? 1: 0),
                position: position / sameLevel.length,
            };
            var siblingStats = NodesProcessing.nodeSetStats(sameLevel.filter(
                function () {
                    return elem.index(this) == -1;
                }
            ));
            NodesProcessing.updateLayout(res, siblingStats, "siblings");
            var children = elem.contents().filter(NodesProcessing.filterFunc);
            var childrenStats = NodesProcessing.nodeSetStats(children);
            NodesProcessing.updateLayout(res, childrenStats, "children");
            res.grandChildrenTagSet2 = [];
            res.subtreeTagSet2 = [];
            res.subtreeBlocks = 0;
            for (var i = 0; i < TAG_SET_2.length; ++i) {
                res.grandChildrenTagSet2[TAG_SET_2[i]] = 0;
                res.subtreeTagSet2[TAG_SET_2[i]] = 0;
            }
            var grandChildrenElems = elem.children().filter(NodesProcessing.filterFunc).children().filter(NodesProcessing.filterFunc);
            for (var i = 0; i < grandChildrenElems.length; ++i) {
                var tagName = grandChildrenElems[i].nodeName.toLowerCase();
                if (TAG_SET_2.indexOf(tagName) >= 0) {
                    ++res.grandChildrenTagSet2[tagName];
                }
            }
            var subtreeElems = elem.find("*").filter(NodesProcessing.filterFunc);
            for (var i = 0; i < subtreeElems.length; ++i) {
                var tagName = subtreeElems[i].nodeName.toLowerCase();
                if (TAG_SET_2.indexOf(tagName) >= 0) {
                    ++res.subtreeTagSet2[tagName];
                }
                if (BLOCK_TAGS.indexOf(tagName) >= 0) {
                    ++res.subtreeBlocks;
                }
            }
            var selfElems = elem.filter(NodesProcessing.filterFunc);
            var selfStats = NodesProcessing.nodeSetStats(selfElems);
            NodesProcessing.updateLayout(res, selfStats, "self");
            treeLayoutFactors[guid] = res;
        },
        calculateMergedBlocksTreeLayoutFactors: function(mergedGuids, mergeId) {
            treeLayoutFactors[mergeId] = treeLayoutFactors[mergedGuids[0]];
            var merged = $();
            for (var i = 0; i < mergedGuids.length; ++i) {
                merged = merged.add(elems[mergedGuids[i]]);
            }
            for (var key in treeLayoutFactors[mergeId]) {
                if (key.substr(0, "siblings_".length) == "siblings") {
                    treeLayoutFactors[mergeId][key] = undefined;
                }
            }
            treeLayoutFactors[mergeId].isLast = treeLayoutFactors[mergedGuids[mergedGuids.length - 1]].isLast;
            var siblings = merged.parent().contents().filter(
                function () {
                    return merged.index(this) == -1 && (
                        (this.nodeType == Node.ELEMENT_NODE && TreeProcessing.visibleAndBig($(this))) ||
                        (this.nodeType == Node.TEXT_NODE && jQuery.trim(this.nodeValue).length)
                    );
                }
            );
            var siblingStats = NodesProcessing.nodeSetStats(siblings);
            NodesProcessing.updateLayout(treeLayoutFactors[mergeId], siblingStats, "siblings");
            var prefixes = ["children", "self"];
            for (var i = 1; i < mergedGuids.length; ++i) {
                for (var j = 0; j < prefixes.length; ++j) {
                    treeLayoutFactors[mergeId][prefixes[j] + "_count"] +=
                        treeLayoutFactors[mergedGuids[i]][prefixes[j] + "_count"];
                    treeLayoutFactors[mergeId][prefixes[j] + "_blocks"] +=
                        treeLayoutFactors[mergedGuids[i]][prefixes[j] + "_blocks"];
                    treeLayoutFactors[mergeId][prefixes[j] + "_nonBlockElements"] +=
                        treeLayoutFactors[mergedGuids[i]][prefixes[j] + "_nonBlockElements"];
                    treeLayoutFactors[mergeId][prefixes[j] + "_texts"].update(
                        treeLayoutFactors[mergedGuids[i]][prefixes[j] + "_texts"].val
                    );
                    treeLayoutFactors[mergeId][prefixes[j] + "_children"].update(
                        treeLayoutFactors[mergedGuids[i]][prefixes[j] + "_children"].val
                    );
                    for (var k = 0; k < TAG_SET_2.length; ++k) {
                        treeLayoutFactors[mergeId][prefixes[j] + "_tagSet2Counts"][TAG_SET_2[k]] +=
                            treeLayoutFactors[mergedGuids[i]][prefixes[j] + "_tagSet2Counts"][TAG_SET_2[k]];
                    }
                }
                treeLayoutFactors[mergeId].subtreeBlocks += treeLayoutFactors[mergedGuids[i]].subTreeBlocks;
                for (var j = 0; j < TAG_SET_2.length; ++j) {
                    treeLayoutFactors[mergeId].grandChildrenTagSet2[TAG_SET_2[j]] +=
                        treeLayoutFactors[mergedGuids[i]].grandChildrenTagSet2[TAG_SET_2[j]];
                    treeLayoutFactors[mergeId].subtreeTagSet2[TAG_SET_2[j]] +=
                        treeLayoutFactors[mergedGuids[i]].subtreeTagSet2[TAG_SET_2[j]];
                }
            }
        },
        updateLayout: function (layout, stats, prefix) {
            for (var key in stats) {
                layout[prefix + "_" + key] = stats[key];
            }
        },
        nodeSetStats: function (nodeSet) {
            var blocks = 0;
            var nonBlockElements = 0;
            var tagSet2Nodes = [];
            for (var i = 0; i < TAG_SET_2.length; ++i) {
                tagSet2Nodes[TAG_SET_2[i]] = 0;
            }
            var texts = new ArrayFactor();
            var children = new ArrayFactor();
            for (var i = 0; i < nodeSet.length; ++i) {
                var node = $(nodeSet[i]);
                texts.update([jQuery.trim(node.text().replace(/\s+/g, " ")).length]);
                if (node[0].nodeType == Node.TEXT_NODE) {
                    continue;
                }
                var tagName = node.prop("tagName").toLowerCase();
                if (tagName != "iframe") {
                    children.update([node.contents().filter(NodesProcessing.filterFunc).length]);
                }
                if (BLOCK_TAGS.indexOf(tagName) >= 0) {
                    ++blocks;
                } else {
                    ++nonBlockElements;
                }
                var tagSet2Idx = TAG_SET_2.indexOf(tagName);
                if (tagSet2Idx >= 0) {
                    ++tagSet2Nodes[TAG_SET_2[tagSet2Idx]];
                }
            }
            var res = {
                count: nodeSet.length,
                blocks: blocks,
                nonBlockElements: nonBlockElements,
                texts: texts,
                children: children,
                tagSet2Counts: [],
            };
            for (var i = 0; i < TAG_SET_2.length; ++i) {
                res.tagSet2Counts[TAG_SET_2[i]] = tagSet2Nodes[TAG_SET_2[i]];
            }
            return res;
        },
        maxDepth: function (node) {
            var localDepths = new ArrayFactor();
            var stack = [];
            stack.push(node);
            while (stack.length) {
                var curNode = stack.pop();
                if (curNode.nodeType != Node.TEXT_NODE && curNode.nodeType != Node.ELEMENT_NODE) {
                    continue;
                }
                if (curNode.nodeType == Node.TEXT_NODE) {
                    if (jQuery.trim(curNode.nodeValue).length) {
                        var parents = $(curNode).parents();
                        localDepths.update([parents.index(node) + 1]);
                    }
                    continue;
                }
                var jq = $(curNode);
                if (
                    jq.prop("tagName").toLowerCase() == "iframe" || (
                        !jq.children().length && !jQuery.trim(jq.text()).length
                    )
                ) {
                    var parents = $(curNode).parents();
                    localDepths.update([parents.index(node) + 1]);
                    continue;
                }
                var contents = jq.contents();
                for (var i = 0; i < contents.length; ++i) {
                    stack.push(contents[i]);
                }
            }
            return Stats.max(localDepths.val) || 0;
        },
    };

    // Code
    var guids = [];
    var splits = [];
    var calculatedSentences = [];
    for (var factor in FactorTypes) {
        Factors[factor] = [];
    }
    stageNamespaces.choice = ChoiceStage;
    CommonHandlers.htmlPageLoad();
    var baseUrl = $("#html_page").contents().find("base").attr("href").replace(RegExps.Anchor, "");
    var baseHostAndDomain = LinkProcessing.hostAndDomain(baseUrl);
    if (mode != "sentences") {
        ChoiceStage.ask = Overloads.newAsk;
    } else {
        ChoiceStage.ask = Overloads.sentencesNewAsk;
    }
    ChoiceStage.answerYes = Overloads.newAnswer;
    ChoiceStage.start();
    while (Globals.stack.length) {
        var ans = ChoiceStage.answerYes();
    }
    if (mode == "sentences") {
        var res = {
            guids: guids.slice(0),
            splits: [],
            sentences: [],
        };
        for (var i = 0; i < guids.length; ++i) {
            if (guids[i].indexOf("_") == -1) {
                res.splits.push(splits[guids[i]]);
            } else {
                res.splits.push([]);
            }
            res.sentences.push(calculatedSentences[guids[i]]);
        }
        return res;
    }
    var res = {
        guids: guids.slice(0),
        factors: [],
        factorNames: {},
        splits: [],
        mergedFactors: [],
        markedChildren: [],
    };

    $.each(Globals.markedChildren, function(guid, children) {
        if (typeof(children) == "undefined") {
            res.markedChildren.push(null);
        } else {
            res.markedChildren.push([]);
            $.each(children, function(i, block) {
                res.markedChildren[guid].push(block.clone());
            });
        }
    });


    for (var i = 0; i < guids.length; ++i) {
        var textFactors = FactorsProcessing.textFactorsArray(guids[i]);
        var charFactors = FactorsProcessing.charFactorsArray(guids[i]);
        var linkFactors = FactorsProcessing.linkFactorsArray(guids[i]);
        var fontFactors = FactorsProcessing.fontFactorsArray(guids[i]);
        var treeFactors = FactorsProcessing.treeFactorsArray(guids[i]);
        res.factors.push(textFactors[0].concat(charFactors[0]).concat(linkFactors[0]).concat(fontFactors[0]).concat(treeFactors[0]).join("\t"));
        res.factorNames.text = textFactors[1];
        res.factorNames.char = charFactors[1];
        res.factorNames.link = linkFactors[1];
        res.factorNames.font = fontFactors[1];
        res.factorNames.tree = treeFactors[1];
        if (guids[i].indexOf("_") == -1) {
            res.splits.push(splits[guids[i]]);
        } else {
            res.splits.push([]);
        }
    }
    for (var i = 0; i < estJSON.merge.length; ++i) {
        var spl = estJSON.merge[i].split(",");
        var begin = parseInt(spl[0]);
        var end = parseInt(spl[1]);
        var mergeId = begin + "-" + end;
        FactorsProcessing.initFactors(mergeId);
        var flag = false;
        for (var j = begin; j < end; ++j) {
            var guid = estJSON.choice[j];
            if (guids.indexOf(guid) != -1) {
                flag = true;
                break;
            }
        }
        if (!flag) {
            res.mergedFactors.push("");
            continue;
        }
        var mergedGuids = [];
        for (var j = begin; j < end; ++j) {
            var guid = estJSON.choice[j];
            if (guids.indexOf(guid) == -1) {
                continue;
            }
            for (var factor in Factors) {
                Factors[factor][mergeId].update(Factors[factor][guid].val);
            }
            mergedGuids.push(guid);
            tagSet1Dists[mergeId] = [];
            for (var key in rootDists) {
                if (typeof(rootDists[key][mergeId]) != "undefined") {
                    rootDists[key][mergeId] = Math.min(rootDists[key][mergeId], rootDists[key][guid]);
                } else {
                    rootDists[key][mergeId] = rootDists[key][guid];
                }
                if (typeof(depths[key][mergeId]) != "undefined") {
                    depths[key][mergeId].update(depths[key][guid].val);
                } else {
                    depths[key][mergeId] = new ArrayFactor();
                    depths[key][mergeId].update(depths[key][guid].val);
                }
                for (var k = 0; k < TAG_SET_1.length; ++k) {
                    if (typeof(tagSet1Dists[guid][TAG_SET_1[k]]) != "undefined") {
                        if (typeof(tagSet1Dists[mergeId][TAG_SET_1[k]]) != "undefined") {
                            tagSet1Dists[mergeId][TAG_SET_1[k]] = Math.min(tagSet1Dists[mergeId][TAG_SET_1[k]],
                                                                           tagSet1Dists[guid][TAG_SET_1[k]]);
                        } else {
                            tagSet1Dists[mergeId][TAG_SET_1[k]] = tagSet1Dists[guid][TAG_SET_1[k]];
                        }
                    }
                }
            }
        }
        if (end - begin > 1) {
            NodesProcessing.calculateMergedBlocksTableFactors(mergedGuids, mergeId);
            NodesProcessing.calculateMergedBlocksTreeLayoutFactors(mergedGuids, mergeId);
        } else {
            var guid = estJSON.choice[begin];
            tableFactors[mergeId] = tableFactors[guid];
            treeLayoutFactors[mergeId] = treeLayoutFactors[guid];
        }
        res.mergedFactors.push(FactorsProcessing.textFactorsArray(mergeId)[0].concat(
            FactorsProcessing.charFactorsArray(mergeId)[0]
        ).concat(
            FactorsProcessing.linkFactorsArray(mergeId)[0]
        ).concat(
            FactorsProcessing.fontFactorsArray(mergeId)[0]
        ).concat(
            FactorsProcessing.treeFactorsArray(mergeId)[0]
        ).join("\t"));
    }
    return res;
}
