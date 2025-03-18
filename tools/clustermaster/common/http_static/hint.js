SmartHint = {
    delay: 250,

    init: function() {
        SmartHint.frame = document.createElement('div');
        SmartHint.frame.id = 'smartHint';
        SmartHint.frame.style.display = 'none';
        SmartHint.frame.onmouseout = function() {
            SmartHint.hide();
        };
        SmartHint.frame.onmouseover = function() {
            SmartHint.hold();
        };
        document.body.appendChild(SmartHint.frame);
    },

    hold: function() {
        if ('timer' in SmartHint) {
            clearTimeout(SmartHint.timer);
            delete SmartHint.timer;
        }
        if ('request' in SmartHint) {
            SmartHint.request.abort();
            delete SmartHint.request;
        }
    },

    popup: function(url, formater) {
        SmartHint.hold();

        SmartHint.timer = setTimeout(function() {
            SmartHint.request = new XMLHttpRequest();
            SmartHint.request.open('GET', url, true);

            SmartHint.request.onreadystatechange = function() {
                if (this.readyState == 4) {
                    if (this.status == 200) {
                        SmartHint.frame.innerHTML = formater.call(formater, this.responseText);
                        SmartHint.frame.style.display = 'block';

                        if (Mouse.x + SmartHint.frame.offsetWidth < window.pageXOffset + window.innerWidth)
                            SmartHint.frame.style.left = (Mouse.x + 1) + 'px';
                        else
                            SmartHint.frame.style.left = (Mouse.x - SmartHint.frame.offsetWidth - 1) + 'px';

                        if (Mouse.y - SmartHint.frame.offsetHeight > window.pageYOffset)
                            SmartHint.frame.style.top = (Mouse.y - SmartHint.frame.offsetHeight - 1) + 'px';
                        else
                            SmartHint.frame.style.top = (Mouse.y + 1) + 'px';
                    } else {
                        SmartHint.hide();
                    }

                    delete SmartHint.request;
                }
            }

            SmartHint.request.send(null);
        }, SmartHint.delay);
    },

    hide: function() {
        SmartHint.hold();

        SmartHint.timer = setTimeout(function() {
            SmartHint.frame.style.display = 'none';
        }, SmartHint.delay);
    },

    assignControls: function(controls, formater) {
        for (var i = 0; i < controls.length; ++i) {
            controls[i].onmouseover = function() {
                SmartHint.popup(this.hinturl || this.getAttribute('hinturl'), formater);
            }
            controls[i].onmouseout = function() {
                SmartHint.hide();
            }
        }
    },
};

window.addEventListener('load', SmartHint.init, false);
