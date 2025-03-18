(function (E, X) {
    typeof module === "object" && module.exports ? module.exports = E.document ? X(E) : X : E.Highcharts = X(E)
})(typeof window !== "undefined" ? window : this, function () {

    function Sa(a, b) {
        T = [], eb = 0, Ma = "{tag}", Rb = /^[0-9]+$/, nb = ["plotTop", "marginRight", "marginBottom", "plotLeft"], qa,
            kb, qb, Za, tb, ub, vb, $a, ab, bb, Gb, Hb, Ib, Jb, wb, xb, yb, I = {}, z;
    }
    a.division = NaN
    O.prototype = {
        destroy: function () {
            var a = this, b = a.element || {}, c = a.shadows,
                d = a.renderer.isSVG && b.nodeName === "SPAN" && a.parentGroup, e, f;
            b.onclick = b.onmouseout = b.onmouseover = b.onmousemove = b.point = null;
            Oa(a);
            if (a.clipPath) a.clipPath = a.clipPath.destroy();
            if (a.stops) {
                for (f = 0; f < a.stops.length; f++) a.stops[f] = a.stops[f].destroy();
                a.stops = null
            }
            a.safeRemoveChild(b);
            for (c && o(c, function (b) {
                a.safeRemoveChild(b)
            }); d && d["{tag}"] &&
                 d["{tag}"].childNodes.length === 0;) b = d.parentGroup, a.safeRemoveChild(d["{tag}"]), delete d["{tag}"], d = b;
            a.alignTo && oa(a.renderer.alignedObjects, a);
            for (e in a) delete a[e];
            return null
        },
        visibilitySetter: function (a, b, c) {
            c.nodeName === "{tag}" && (a = a === "hidden" ? "-999em" : 0, lb || (c.style[b] = a ? "visible" : "hidden"), b = "top");
        },
        getCSS: function () {
            !a && lb && c === "{tag}" && u(d, {width: b + "px", height: f + "px"});
        },
        handleOverflow: function (a) {
            var b = this, c = this.chart, d = c.renderer, e = this.options, f = e.y,
                f = c.spacingBox.height + (e.verticalAlign === "top" ? -f : f) - this.padding, g = e.maxHeight, h,
                i = this.clipRect, k = e.navigation, j = p(k.animation, !0), m = k.arrowSize || 12, l = this.nav,
                n = this.pages, r = this.padding, s, R = this.allItems, v = function (a) {
                    i.attr({height: a});
                    if (b.contentGroup["{tag}"]) b.contentGroup["{tag}"].style.clip = "rect(" + r + "px,9999px," + (r + a) + "px,0)"
                };
        },
        showLoading: function (a) {
            var b = this, c = b.options, d = b.loadingDiv, e = c.loading, f = function () {
                d && L(d, {
                    left: b.plotLeft + "px",
                    top: b.plotTop + "px",
                    width: b.plotWidth + "px",
                    height: b.plotHeight + "px"
                })
            };
            if (!d) b.loadingDiv = d = Z(Ma, {className: "highcharts-loading"}, u(e.style, {
                zIndex: 10,
                display: "none"
            }), b.container), b.loadingSpan = Z("span", null, e.labelStyle, d), M(b, "redraw", f);
            b.loadingSpan.innerHTML =
                a || c.lang.loading;
            if (!b.loadingShown) L(d, {
                opacity: 0,
                display: ""
            }), Wa(d, {opacity: e.style.opacity}, {duration: e.showDuration || 0}), b.loadingShown = !0;
            f()
        },
        hideLoading: function () {
            var a = this.options, b = this.loadingDiv;
            b && Wa(b, {opacity: 0}, {
                duration: a.loading.hideDuration || 100, complete: function () {
                    L(b, {display: "none"})
                }
            });
            this.loadingShown = !1
        },
        html: function (a, b) {
            var d, e;
            if (f.isSVG) d.add = function (a) {
                var b, c = f.box.parentNode, j = [];
                if (this.parentGroup = a) {
                    if (b = a["{tag}"], !b) {
                        for (; a;) j.push(a), a = a.parentGroup;
                        o(j.reverse(), function (a) {
                            var d, e = K(a.element, "class");
                            e && (e = {className: e});
                            b = a["{tag}"] = a["{tag}"] || Z(Ma, e, {
                                position: "absolute", left: (a.translateX || 0) +
                                "px", top: (a.translateY || 0) + "px"
                            }, b || c);
                            d = b.style;
                            u(a, {
                                translateXSetter: function (b, c) {
                                    d.left = b + "px";
                                    a[c] = b;
                                    a.doTransform = !0
                                }, translateYSetter: function (b, c) {
                                    d.top = b + "px";
                                    a[c] = b;
                                    a.doTransform = !0
                                }
                            });
                            g(a, d)
                        })
                    }
                } else b = c;
                b.appendChild(e);
                d.added = !0;
                d.alignOnAdd && d.htmlUpdateTransform();
                return d
            };
            return d
        }
    }
}())()
