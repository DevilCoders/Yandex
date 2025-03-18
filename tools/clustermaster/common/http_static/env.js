function set() {
    var result = {};
    for (var i = 0, n = arguments.length; i < n; ++i)
        result[arguments[i]] = true;
    return result;
}

document.foreachElementsByName = function(name, f) {
    for (var e = this.getElementsByName(name), n = e.length, i = 0; i < n; ++i)
        f(e[i]);
}

Number.prototype.to2DigitsString = function() {
    return (this.valueOf() % 100 > 9 ? '' : '0') + this.valueOf() % 100;
}

Date.prototype.toDateString = function() {
    return /*this.getFullYear().toString() + '-' +*/ (this.getMonth() + 1).to2DigitsString() + '-' + this.getDate().to2DigitsString();
}

Date.prototype.toTimeString = function() {
    return this.getHours().to2DigitsString() + ':' + this.getMinutes().to2DigitsString() + ':' + this.getSeconds().to2DigitsString();
}

Date.prototype.toDateTimeString = function() {
    return this.toDateString() + ' ' + this.toTimeString();
}

Date.prototype.toDurationString = function() {
    var hours = Math.floor(this.getTime() / 3600000);
    var minutes = Math.floor((this.getTime() % 3600000) / 60000);
    var seconds = Math.floor((this.getTime() % 60000) / 1000);
    return hours.to2DigitsString() + ':' + minutes.to2DigitsString() + ':' + seconds.to2DigitsString();
}

CMWorkerStates = {0: 'disconnected', 1: 'connecting', 2: 'connected', 3: 'authenticating', 4: 'initializing', 5: 'active', t: 'total'};
CMWorkerStatesTitles = {0: 'Disconnected', 1: 'Connecting', 2: 'Connected', 3: 'Authenticating', 4: 'Initializing', 5: 'Active', t: 'Total'};
CMWorkerStatesTitles2 = {0: 'Disconnected', 1: 'Connecting...', 2: 'Connected...', 3: 'Authenticating...', 4: 'Initializing...', 5: 'Active'};
CMWorkerStatesTitles3 = {0: 'D', 1: 'C', 2: 'C', 3: 'A', 4: 'I', 5: 'A', t: 'T'};

CMTargetStates = {
    i: 'idle',
    p: 'pending',
    e: 'ready',
    r: 'running',
    L: 'stopping',
    S: 'stopped',
    C: 'continuing',
    U: 'suspended',
    s: 'success',
    k: 'skipped',
    f: 'failed',
    fk: 'killed',
    fa: 'aborted',
    fv: 'violated',
    d: 'depfailed',
    l: 'canceling',
    c: 'canceled',
    u: 'unknown',
    t: 'total'
};

CMTargetStatesTitles = {
    i: 'Idle',
    p: 'Pending',
    e: 'Ready',
    r: 'Running',
    L: 'Stopping',
    S: 'Stopped',
    C: 'Continuing',
    U: 'Suspended',
    s: 'Success',
    k: 'Skipped',
    f: 'Failed',
    fk: 'Killed',
    fa: 'Aborted',
    fv: 'Violated',
    d: 'Depfailed',
    l: 'Canceling',
    c: 'Canceled',
    u: 'Unknown',
    t: 'Total'
};

function CMErrorNotification(message) {
    var element = document.getElementById('notification');
    var innerHTML = message ? '<span>' + message.toString().split('\n').join('<br>') + '</span>' : '';
    if (element.innerHTML != innerHTML)
        element.innerHTML = innerHTML;
}

function CMPutTime(e, v, name) {
    e.innerHTML = (v == 0 ? '&mdash;' : new Date(v * 1000).toDateTimeString());

    if (typeof name !== 'undefined')
        e.parentNode.sortValues[name] = v;
}

function CMPutDuration(e, v, name) {
    e.innerHTML = (v < 0 ? '&mdash;' : new Date(v * 1000).toDurationString());

    if (typeof name !== 'undefined')
        e.parentNode.sortValues[name] = v;
}

function CMPutResource(e, v, name) {
    e.innerHTML = (v * 100).toFixed(2) + '%';

    if (typeof name !== 'undefined')
        e.parentNode.sortValues[name] = v;
}

function CMPutWorkerState(e, v) {
    e.innerHTML = '<div class="worker_' + (CMWorkerStates[v] || 'unknown') + '">' + (CMWorkerStatesTitles2[v] || 'Unknown') + '</div>';
}

function CMPutShortWorkerState(e, v) {
    e.innerHTML = '<div class="worker_' + (CMWorkerStates[v] || 'unknown') + '">' + (CMWorkerStatesTitles3[v] || '?') + '</div>';
}

function CMPutDiskspace(e, v, name) {
    if (!v)
        e.innerHTML = '?';
    else if (v < 1073741824)
        e.innerHTML = Math.ceil(v / 1048576) + ' MB';
    else if (v < 1099511627776)
        e.innerHTML = Math.ceil(v / 1073741824) + ' GB';
    else
        e.innerHTML = (v / 1099511627776).toFixed(1) + ' TB';

    if (typeof name !== 'undefined')
        e.parentNode.sortValues[name] = v;
}

function CMPutStateLabels(e, v, href, cmdparams, repeat, cronFailed) {
    var usecmdparams = cmdparams && !Env.ReadOnly;

    var pattern = '<a class="target_%S" %H %M>%S</a>'.replace(/%H/, href ? 'href="' + href + '"' : '').replace(/%M/, usecmdparams ? ActionMenu.controlAttributes(cmdparams) : '');

    var items = [];

    var cronFailedMessage = 'Cron (restart_on_success): something bad has happened (host is unavailable or user has changed target state via web interface)';

    if (cronFailed)
        items.push('<span title="' + cronFailedMessage + '" class="cron-failed">[CF]</span>');

    for (var i in CMTargetStates)
        if (i in v && i !== 't')
            items.push(pattern.replace(/%S/g, CMTargetStates[i]));

    if (repeat.REPM !== 0) {
        e.innerHTML = items.join(', ') + '[' + repeat.REPC + '/' + repeat.REPM + ']';
    } else {
        e.innerHTML = items.join(', ');
    }
}

function CMPutStateCounts(e, v) {
    var pattern = '<span class="target_%S" title="%S">&nbsp;%C&nbsp;</span>';

    var items = [];
    for (var i in CMTargetStates)
        if (i in v)
            items.push(pattern.replace(/%C/g, v[i]).replace(/%S/g, CMTargetStates[i]));

    e.innerHTML = items.join('+').replace(/\+(?=[^+]*$)/, '/');
}

function CMPutStatePiechart(e, v) {
    var pie = e.ownerDocument.createElementNS(e.namespaceURI, 'g');

    var startangle = 0;

    for (var i in set('k', 's', 'd', 'f', 'r', 'e', 'p', 'l', 'c', 'i')) {
        if (v === null || i in v) {
            var sector;

            if (v === null || v[i] == v.t) {
                sector = e.ownerDocument.createElementNS(e.namespaceURI, 'ellipse');

                sector.setAttribute('cx', e.cx);
                sector.setAttribute('cy', e.cy);
                // ellipse doenst fully cover some graphviz shapes (eg. invhouse), so we do rx a little bigger
                sector.setAttribute('rx', e.rx + e.rx / 4);
                sector.setAttribute('ry', e.ry);
            } else {
                sector = e.ownerDocument.createElementNS(e.namespaceURI, 'path');

                var endangle = startangle + v[i] / v.t * Math.PI * 2;

                var x1 = e.cx + e.rx * Math.sin(startangle);
                var y1 = e.cy - e.ry * Math.cos(startangle);
                var x2 = e.cx + e.rx * Math.sin(endangle);
                var y2 = e.cy - e.ry * Math.cos(endangle);

                var lf = (endangle - startangle > Math.PI) ? 1 : 0;

                sector.setAttribute('d', 'M ' + e.cx + ',' + e.cy + ' L ' + x1 + ',' + y1 + ' A ' + e.rx + ',' + e.ry + ' 0 ' + lf + ' 1 ' + x2 + ',' + y2 + ' Z');

                startangle = endangle;
            }

            sector.setAttribute('class', 'target_' + (v !== null ? CMTargetStates[i] : 'foreign'));

            pie.appendChild(sector);
        }
    }

    e.replaceChild(pie, e.firstChild);
}

function CMFormatCmnsmHint(hint) {
    var concatWithBr = function(array) {
        var result = '';
        for (var i = 0; i < array.length; i++) {
            if (array[i].length != 0) {
                result += (array[i] + ((i != (array.length - 1)) ? '<br/>' : '')); // do not add '<br/>' to last element
            }
        }
        return result;
    };

    var hintObj = eval('(' + hint + ')');

    var solverWebLink = hintObj.solverIsLocal ? '' : CM.format('<a href="{0}proxy/{1}/proxy_solver/{2}/{3}/">(link)</a>', PageEnv.UrlRoot, hintObj.worker, hintObj.target, hintObj.task);

    var stateAndSolver = CM.format('<span class="cmnsm_{0}">{1}</span>&nbsp;{2}&nbsp;{3}', hintObj.state, hintObj.state, hintObj.solver, solverWebLink);
    var message = hintObj.message;

    var resources = '';
    var first = true;
    for (k in hintObj.resources) {
        r = hintObj.resources[k];
        if (!first) {
            resources += ',&nbsp;';
        }
        resources += CM.format('{0}&nbsp;:&nbsp;<span class="sh_green">{1}</span>', k, r.value);
        if (r.name.length != 0) {
            resources += CM.format('&nbsp;<span class="sh_blue">({0})</span>', r.name);
        }
        first = false;
    }

    var priority = CM.format('priority:&nbsp;<span class="sh_green">{0}</span>', hintObj.priority);

    var resStrNumbersWhite = hintObj.resourceStr.replace(/(\d+)/g, '<span class="sh_white">$1</span>')
    var resStr = CM.format('<span class="sh_gray"><span class="sh_white">res=</span>{0}</span>', resStrNumbersWhite);

    return concatWithBr([stateAndSolver, message, resources, priority, resStr]);
}

function CMFormatTaskInfo(info) {
    this.template = this.template ||
        '<span class="sh_gray">Task <span class="sh_white">%N</span> on worker <span class="sh_white">%W</span>:</span><br/>'+
        'Last changed: <span class="sh_green">%LC</span>';

    return template
        .replace('%W', info.worker)
        .replace('%N', info.ntask)
        .replace('%LC', info.lastchanged ? new Date(info.lastchanged * 1000).toDateTimeString() : '&mdash;');
}

function CMFormatTaskInfoHint(hint) {
    return CMFormatTaskInfo(eval('(' + hint + ')'));
}

function CMFormatTargetInfo(info) {
    if (info.task) {
        return CMFormatTaskInfo(info.task);
    }
    return '';
}

function CMFormatTargetInfoHint(hint) {
    return CMFormatTargetInfo(eval('(' + hint + ')'));
}

CM = { // CM namespace; at some point I desided to make pure-blooded namespace and gradually move all functions with CM prefix/all global objects here
    forEach: function(array, f) {
        for (var i = 0; i < array.length; i++) {
            f(array[i]);
        }
    },
    findTableBody: function(tableElement) {
        var element = tableElement;
        for (element = element.firstChild; element && element.tagName !== 'TBODY'; element = element.nextSibling) {
            // intentionally left empty
        }
        return element;
    },
    collectTableRows: function(tableElement, predicate) {
        var element = CM.findTableBody(tableElement);
        if (!element) {
            return [];
        }

        var array = [];
        for (element = element.firstChild; element; element = element.nextSibling) {
            if (typeof predicate !== 'function' || predicate(element)) {
                array.push(element);
            }
        }
        return array;
    },
    format: function(string) {
        var args = arguments;
        return string.replace(/{(\d+)}/g, function(match, numberStr) {
          var number = parseInt(numberStr);
          return typeof args[number + 1] != 'undefined'
            ? args[number + 1]
            : match
          ;
        });
    },
    getQueryString: function() {
        var query = location.search.substring(1);
        var keyValues = query.split("&");
        var result = {};
        keyValues.forEach(function(item) {
            var parts = item.split("=");
            result[parts[0]] = parts[1];
        })
        return result; 
    }
};
