MIN_SIZE = 20;
LETTER_RE = new RegExp("[0-9a-z\\u00C0-\\u1FFF\\u2C00-\\uD7FF]", "ig");

function Hl(elements, options) {
    this.options = options || {};
    this.view = this.options.view || {};
    this.id = Math.random().toString().slice(2);

    this.$elements = $(elements);
    this.$el = this._render();
    this.$wrapper = this._wrapper();
    this.fit();
    this.bindAll();

    this.$el.appendTo(this.$wrapper);
    this.$wrapper.appendTo($("#html_page").contents().find("html")[0]);
}

Hl.prototype = {
    destroy: function () {
        this.$wrapper.remove();
        this.unbindAll();
    },
    bindAll: function () {
        var fit = $.proxy(this.fit, this);
        $(window).on('resize.' + this.id, fit);
    },
    unbindAll: function () {
        $(window).off('.' + this.id);
    },
    _wrapper: function () {
        var $el = $(document.createElement('y-element-wrapper'));

        return $el.css({
            display: 'block',
            position: 'absolute',
            height: '100%',
            width: '100%',
            left: 0,
            top: 0,
            pointerEvents: 'none'
        });
    },
    _render: function () {
        var $el = $(document.createElement('y-element-overlay'))

        $.each(this.view, function (key, value) {
            if (typeof $el[key] === 'function') {
                $el[key](value);
            }
        });

        return $el.css({
            display: 'block',
            position: 'absolute',
            pointerEvents: 'none'
        });
    },
    bounds: function () {
        var totalW = $("#html_page").contents().width();
        var totalH = $("#html_page").contents().height();
        var x1 = Infinity;
        var y1 = Infinity;
        var x2 = 0;
        var y2 = 0;
        this.$elements.each(function () {
            var $el = $(this);
            var pos = $el.offset();
            x1 = Math.min(x1, pos.left);
            y1 = Math.min(y1, pos.top);
            x2 = Math.max(x2, pos.left + $el.outerWidth());
            y2 = Math.max(y2, pos.top + $el.outerHeight());
            $el.find("*").each(function () {
                var $desc = $(this);
                var off = $desc.offset();
                var elWidth = $desc.width();
                var elHeight = $desc.height();
                var tagName = $desc.prop("tagName").toLowerCase();
                var elIsVisible = $desc.is(":visible") &&
                                  $desc.css("visibility") != "hidden";
                var elIsHiddenOption = ["optgroup", "option"].indexOf(tagName) >= 0 &&
                                       $desc.parents("select").length;
                var elIsBig = (elWidth > MIN_SIZE || elHeight > MIN_SIZE || $desc.text().match(LETTER_RE));
                var elIsOnPage = off.left + elWidth >= 0 &&
                                 off.left <= totalW &&
                                 off.top + elHeight >= 0 &&
                                 off.top <= totalH;
                var elHasContent = jQuery.trim($desc.text()) != "" ||
                                   ["input", "img"].indexOf(tagName) >= 0;
                if (elIsVisible && elIsBig && elIsOnPage && elHasContent && !elIsHiddenOption) {
                    x1 = Math.min(x1, off.left);
                    y1 = Math.min(y1, off.top);
                    x2 = Math.max(x2, off.left + elWidth);
                    y2 = Math.max(y2, off.top + elHeight);
                }
            });
        });
        x2 = Math.min(x2, totalW);
        y2 = Math.min(y2, totalH);
        return {
            x: Math.max(x1, 0),
            y: Math.max(y1, 0),
            w: x2 - x1,
            h: y2 - y1
        };
    },
    fit: function () {
        var bounds = this.bounds();
        this.$el.css({
            width: (bounds.w - 6) + 'px',
            height: (bounds.h - 6) + 'px',
            left: (bounds.x + 3) + 'px',
            top: (bounds.y + 3) + 'px'
        });
    },
    attachInstance: function (property) {
        this.$wrapper.data(property, this);
    }
};

Highlight = {
    addMark: function (options) {
        options = options || {};
        if (!options.el) {
            throw new Error('options.selector is required');
        }
        var $el = options.el;
        var id = options.id;
        var html = options.html || '<div></div>';
        var events = options.events || {};
        var hl = new Hl($el, {
            view: {
                html: $(html).attr('data-mark-id', id).on(events),
            }
        });
        hl.attachInstance('hl');
    },
    removeMark: function (id) {
        $("#html_page").contents().find("[data-mark-id=\"" + id + "\"]").parent().parent().each(function () {
            var hl = $(this).data('hl');

            if (hl) {
                hl.destroy();
            }
        });
    },
};
