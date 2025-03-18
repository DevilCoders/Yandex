window.addEventListener('load', initTargetsPage, false);

function initTargetsPage() {
    var MasterElement = document.getElementById('MasterElement');
    var stateurl = MasterElement.getAttribute('stateurl');
    var urlroot = MasterElement.getAttribute('urlroot');
    var worker = MasterElement.getAttribute('worker');
    var timestamp = MasterElement.getAttribute('timestamp');

    Updater.register('$#', function(v) {
        if (v != timestamp)
            window.location.reload();
    });

    Updater.register('$!', function(v) {
        CMErrorNotification(v);
    });

    if (worker) {
        var TheWorkerHttpPort = document.getElementById('$P');
        Updater.register('$P', function(v) {
            TheWorkerHttpPort.innerHTML = v || '&mdash;';
        });

        var TheWorkerState = document.getElementById('$S');
        Updater.register('$S', function(v) {
            CMPutWorkerState(TheWorkerState, v);
        });

        var TheWorkerFailure = document.getElementById('$F');
        Updater.register('$F', function(v) {
            CMPutTime(TheWorkerFailure, v);
        });

        var TheWorkerReason = document.getElementById('$R');
        Updater.register('$R', function(v) {
            TheWorkerReason.innerHTML = v || '&mdash;';
        });
    }

    var update = function(e, v) {
        if (v === null) { // Filtered out
            e.row.style.display = 'none';
        } else {
            e.row.style.display = 'table-row';
            if (worker) {
                CMPutStateLabels(e.X, v.t, urlroot + 'worker/' + worker + '/' + e.row.target + '?state=%S', 'worker=' + worker + '&target=' + e.row.target + '&state=%S', v.REP, v.CronFailed); // State
                CMPutDuration(e.dA, v.dA, 'duract'); // Act duration
                CMPutDuration(e.dt, v.dt, 'durttl'); // Ttl duration
            } else {
                CMPutStateLabels(e.X, v.t, urlroot + 'target/' + e.row.target + '?state=%S', 'target=' + e.row.target + '&state=%S', v.REP, v.CronFailed); // State
                CMPutStateCounts(e.W, v.W); // Workers
                CMPutDuration(e.dm, v.dm, 'durmin'); // Min duration
                CMPutDuration(e.da, v.da, 'duravg'); // Avg duration
                CMPutDuration(e.dM, v.dM, 'durmax'); // Max duration
            }
            CMPutStateCounts(e.t, v.t); // Tasks
            CMPutTime(e.s, v.s, 'started'); // Last started
            CMPutTime(e.f, v.f, 'finished'); // Last finished
            CMPutTime(e.S, v.S, 'success'); // Last success
            CMPutTime(e.F, v.F, 'failure'); // Last failure
            CMPutResource(e.cM, v.cM, 'cpumax'); // Cpu max
            CMPutResource(e.ca, v.ca, 'cpuavg'); // Cpu avg
            CMPutResource(e.mM, v.mM, 'memmax'); // Mem max
            CMPutResource(e.ma, v.ma, 'memavg'); // Mem avg
        }
    }

    var elements = {};

    document.foreachElementsByName('@@', function(e) {
        elements[e.target = e.getAttribute('target')] = { row: e };
    });

    for (var i in set('X', 't', 'W', 's', 'f', 'S', 'F', 'da', 'dm', 'dM', 'dA', 'dt', 'cM', 'ca', 'mM', 'ma', 'U'))
        document.foreachElementsByName('@' + i, function(e) {
            elements[e.parentNode.target][i] = e;
        });

    for (var i in elements)
        (function(i) {
            Updater.register(i, function(v) {
                update(elements[i], v);
            });
        })(i);

    Mouse.assignDocument(document, null);

    SmartHint.assignControls(document.getElementsByName('SH'), CMFormatCmnsmHint);
    SmartHint.assignControls(document.getElementsByName('IH'), CMFormatTargetInfoHint);

    ActionMenu.setUrlRoot(urlroot);
    ActionMenu.assignControls(document.getElementsByName('AM'));

    var sorter = new CM.Sort.Sorter();
    sorter.setTable(MasterElement);
    var strComparers = ['type'];
    var numComparers = ['number', 'started', 'finished', 'success', 'failure', 'durmin', 'duravg', 'durmax', 'duract', 'durttl', 'cpumax', 'cpuavg', 'memmax', 'memavg'];
    sorter.addComparers(strComparers, numComparers);
    sorter.addComparer('target',
        function (a, b) {
            return a.sortValues.target.localeCompare(b.sortValues.target);
        },
        function (a, b) {
            return b.sortValues.target.localeCompare(a.sortValues.target);
        },
        function (a, b) {
            return a.sortValues.index - b.sortValues.index;
        });
    sorter.addCallback('target', function (row, elem, clear, state) {
        if (state != 2) {
            return;
        }
        if (clear) {
            $(row).removeClass("oddnode-color");
            $(elem).find(".toggle-sign").remove();
        }
        else {
            var sign = $(document.createElement("span")).addClass("toggle-sign");
            if ($(row).hasClass("singlenode")) {
                sign.html("&#9671;&nbsp;");
            }
            else if (!$(row).hasClass("rootnode")) {
                sign.html("&nbsp;&nbsp;");
            }
            else {
                sign.html("&#9663;&nbsp;").css("cursor", "pointer");
                sign.click(function () {
                    var target = "." + $(row).attr("target");
                    if ($(target).is(":visible")) {
                        $(this).html("&#9656;&nbsp;")
                        $(target).hide();
                    }
                    else {
                        $(target).show();
                        $(this).html("&#9663;&nbsp;")
                    }
                });
            }
            $(elem).prepend(sign);
            if ($(row).hasClass("oddnode")) {
                $(row).addClass("oddnode-color");
            }
        }
    });

    document.foreachElementsByName('sortctrl', function(e) {
        var key = e.getAttribute('sortkey');
        sorter.addControl(key, e);
        e.onclick = (function(sorter, key) {
            return function() {
                sorter.sort(key);
            };
        })(sorter, key);
    });

    var rows = CM.collectTableRows(MasterElement, function(e) {
        return e.getAttribute('name') === '@@';
    });
    ////
    var root = [];
    var trgs = {};
    function Split(elem) {
        var targets = elem.attr("targets");
        if (!targets) {
            return [];
        }
        return targets.split(/,/);
    }
    CM.forEach(rows, function(row) {
        var target = row.getAttribute('target')
        row.sortValues = {
            number: row.getAttribute('number'),
            type: row.getAttribute('type'),
            target: target,
        };
        trgs[target] = {
            dependers: Split($(row).find(".dependers")),
            followers: Split($(row).find(".followers")),
            element: row,
        };
        if (trgs[target]["dependers"].length < 1) {
            root.push(target);
        }
    });

    var index = 0;
    root.forEach(function (elem) {
        if (trgs[elem].index) {
          return;
        }
        ++index;
        var rootElem = trgs[elem].element;
        if (trgs[elem].followers.length < 1) {
            $(rootElem).addClass("singlenode");
        }
        else {
            $(rootElem).addClass("rootnode");
        }

        function walk(nodes) {
            nodes.forEach(function (node) {
                if (!trgs[node] || trgs[node].index) {
                    return;
                }
                trgs[node].index = index;

                var e = trgs[node].element;
                e.sortValues.index = rows.length * index + e.sortValues.number;
                if (e != rootElem) {
                    $(e).addClass(elem);
                }
                if (index & 1) {
                    $(e).addClass("oddnode");
                }

                walk(trgs[node].dependers);
                walk(trgs[node].followers)
            })
        }
        walk([elem]);
    });
    ////

    sorter.sort('number');
    Updater.registerAfter(CM.Sort.sortIfTableUpdatedF(sorter, elements));

    Updater.serv(stateurl);
}

function toggleRestartOnSuccess(a, targetName) {
    toggleRestartOnSuccessFailure(a, targetName, true);
}

function toggleRetryOnFailure(a, targetName) {
    toggleRestartOnSuccessFailure(a, targetName, false);
}

function toggleRestartOnSuccessFailure(a, targetName, isSuccess) {
    var needToEnable = AnchorToggler.isSwitchedOff(a);

    var request = new XMLHttpRequest();
    var successFailure = isSuccess ? 'restart_on_success' : 'retry_on_failure';
    var action = needToEnable ? 'enable' : 'disable';

    request.open('GET', 'target_toggle_restart/' + targetName + '/' + successFailure + '?action=' + action, true);
    var requestHandler = function(a, needToEnable) {
        return function() {
            if (this.readyState == 4) {
                if (this.status == 204) {
                    if (needToEnable) {
                        AnchorToggler.switchOn(a);
                    } else {
                        AnchorToggler.switchOff(a);
                    }
                }
            }
        };
    }(a, needToEnable);
    request.onreadystatechange = requestHandler;

    request.send(null);
}

function AnchorRestartToggler() {
    this.switchedOffClass = "restart-disabled";
    this.switchedOnClass = "restart-enabled";

    this.isSwitchedOff = function(a) {
        return (a.className == this.switchedOffClass);
    }
    this.switchOn = function(a) {
        a.className = this.switchedOnClass;
    }
    this.switchOff = function(a) {
        a.className = this.switchedOffClass;
    }
}

AnchorToggler = new AnchorRestartToggler();
