ActionMenu = {
    delay: 250,

    urlroot: '',

    init: Env.ReadOnly ? function() {} : function() {
        ActionMenu.frame = document.createElement('table');

        ActionMenu.frame.id = 'actionMenu';

        ActionMenu.frame.style.display = 'none';

        ActionMenu.frame.innerHTML =
            '<tr><td>Run:</td><td>' +
            '<a name="actionMenuItem" cmdhref="%Rcommand/run?%C">This only</a>, ' +
            '<a name="actionMenuItem" cmdhref="%Rcommand/run-path?%C">Whole path</a>, ' +
            '<a name="actionMenuItem" cmdhref="%Rcommand/run-path?all_workers&%C">Whole path on all workers</a>' +
            '</td></tr>' +
            '<tr><td>Retry run:</td><td>' +
            '<a name="actionMenuItem" cmdhref="%Rcommand/retry-run-path?%C">Whole path</a>, ' +
            '<a name="actionMenuItem" cmdhref="%Rcommand/retry-run-path?all_workers&%C">Whole path on all workers</a>' +
            '</td></tr>' +
            '<tr><td>Forced run:</td><td>' +
            '<a name="actionMenuItem" cmdhref="%Rcommand/forced-run?%C">This only</a>' +
            '</td></tr>' +
            '<tr><td>Forced ready:</td><td>' +
            '<a name="actionMenuItem" cmdhref="%Rcommand/forced-ready?%C">This only</a>' +
            '</td></tr>' +
            '<tr><td>Cancel:</td><td>' +
            '<a name="actionMenuItem" cmdhref="%Rcommand/cancel?%C">This only</a>, ' +
            '<a name="actionMenuItem" cmdhref="%Rcommand/cancel-path?%C">Whole path</a>, ' +
            '<a name="actionMenuItem" cmdhref="%Rcommand/cancel-following?%C">With followers</a>, ' +
            '<a name="actionMenuItem" cmdhref="%Rcommand/cancel-path?all_workers&%C">Whole path on all workers</a>, ' +
            '<a name="actionMenuItem" cmdhref="%Rcommand/cancel-following?all_workers&%C">With followers on all workers</a>' +
            '</td></tr>' +
            '<tr><td>Invalidate:</td><td>' +
            '<a name="actionMenuItem" cmdhref="%Rcommand/invalidate?%C">This only</a>, ' +
            '<a name="actionMenuItem" cmdhref="%Rcommand/invalidate-path?%C" onclick="return confirm(\'Invalidate whole path\');">Whole path</a>, ' +
            '<a name="actionMenuItem" cmdhref="%Rcommand/invalidate-following?%C" onclick="return confirm(\'Invalidate with followers\');">With followers</a>, ' +
            '<a name="actionMenuItem" cmdhref="%Rcommand/invalidate-path?all_workers&%C" onclick="return confirm(\'Invalidate whole path\');">Whole path on all workers</a>, ' +
            '<a name="actionMenuItem" cmdhref="%Rcommand/invalidate-following?all_workers&%C" onclick="return confirm(\'Invalidate with followers\');">With followers on all workers</a>' +
            '</td></tr>' +
            '<tr><td>Reset:</td><td>' +
            '<a name="actionMenuItem" cmdhref="%Rcommand/reset?%C">This only</a>, ' +
            '<a name="actionMenuItem" cmdhref="%Rcommand/reset-path?%C" onclick="return confirm(\'Reset whole path\');">Whole path</a>, ' +
            '<a name="actionMenuItem" cmdhref="%Rcommand/reset-following?%C" onclick="return confirm(\'Reset with followers\');">With followers</a>, ' +
            '<a name="actionMenuItem" cmdhref="%Rcommand/reset-path?all_workers&%C" onclick="return confirm(\'Reset whole path\');">Whole path on all workers</a>, ' +
            '<a name="actionMenuItem" cmdhref="%Rcommand/reset-following?all_workers&%C" onclick="return confirm(\'Reset with followers\');">With followers on all workers</a>' +
            '</td></tr>' +
            '<tr><td>Other:</td><td>' +
            '<a name="actionMenuItem" cmdhref="%Rcommand/mark-success?%C">Mark as successful</a>, ' +
            '<a name="actionMenuItem" cmdhref="%Rcommand/reset-stat?%C">Reset resource stat</a>, ' +
            '<a name="actionMenuItem" cmdhref="%Rgraph?%C&walk=down&depth=-1">Show with children</a>, ' +
            '<a name="actionMenuItem" cmdhref="%Rgraph?%C&walk=up&depth=-1">Show with parents</a>' +
            '</td></tr>' +
            '<tr><td><b>Navigate:</b></td><td><a name="actionMenuItem" cmdhref="%Rtarget/%T">to this</a></td></tr>' +
            '<tr><td>Description:</td><td><p id="actionMenuItemDescr"></p>' +
            '</td></tr>';

        ActionMenu.frame.onmouseout = function() {
            ActionMenu.hide();
        }

        ActionMenu.frame.onmouseover = function() {
            ActionMenu.hold();
        }

        document.body.appendChild(ActionMenu.frame);

        ActionMenu.items = document.getElementsByName('actionMenuItem');
    },

    hold: Env.ReadOnly ? function() {} : function() {
        if ('timer' in ActionMenu) {
            clearTimeout(ActionMenu.timer);
            delete ActionMenu.timer;
        }
    },

    popup: Env.ReadOnly ? function() {} : function(cmdparams) {
        target = /^(.*)target=([^&]+)(.*)$/.exec(cmdparams)[2];
        ActionMenu.hold();

        ActionMenu.timer = setTimeout(function() {
            for (var i = 0; i < ActionMenu.items.length; ++i)
                ActionMenu.items[i].href = ActionMenu.items[i].getAttribute('cmdhref').replace('%R', ActionMenu.urlroot).replace('%C', cmdparams).replace('%T', target);

            if (typeof TargetDescriptions !== "undefined")
                document.getElementById("actionMenuItemDescr").innerHTML = TargetDescriptions[cmdparams.replace(/^(.*)target=((\w|-|_)+)(.*)$/, "$2")];

            ActionMenu.frame.style.display = 'block';

            if (Mouse.x + ActionMenu.frame.offsetWidth < window.pageXOffset + window.innerWidth)
                ActionMenu.frame.style.left = (Mouse.x + 1) + 'px';
            else
                ActionMenu.frame.style.left = (Mouse.x - ActionMenu.frame.offsetWidth - 1) + 'px';

            if (Mouse.y + ActionMenu.frame.offsetHeight < window.pageYOffset + window.innerHeight)
                ActionMenu.frame.style.top = (Mouse.y + 1) + 'px';
            else
                ActionMenu.frame.style.top = (Mouse.y - ActionMenu.frame.offsetHeight - 1) + 'px';
        }, ActionMenu.delay);
    },

    hide: Env.ReadOnly ? function() {} : function() {
        ActionMenu.hold();

        ActionMenu.timer = setTimeout(function() {
            ActionMenu.frame.style.display = 'none';
        }, ActionMenu.delay);
    },

    setUrlRoot: Env.ReadOnly ? function() {} : function(urlroot) {
        ActionMenu.urlroot = urlroot.toString();
    },

    assignControls: Env.ReadOnly ? function() {} : function(controls) {
        for (var i = 0; i < controls.length; ++i) {
            controls[i].onmouseover = function() {
                ActionMenu.popup(this.cmdparams || this.getAttribute('cmdparams'));
            }
            controls[i].onmouseout = function() {
                ActionMenu.hide();
            }
        }
    },

    controlAttributes: Env.ReadOnly ? function() { return ''; } : function(cmdparams) {
        return 'onmouseover="ActionMenu.popup(\'' + cmdparams + '\')" onmouseout="ActionMenu.hide()"';
    }
};

window.addEventListener('load', ActionMenu.init, false);
