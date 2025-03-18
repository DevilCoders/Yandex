Mouse = {
    x: 0, y: 0,

    assignDocument: function(document, node) {
        var left = 0, top = 0;

        (function(node) {
            while (node != null) {
                left += node.offsetLeft;
                top += node.offsetTop;
                node = node.offsetParent;
            }
        })(node);

        (function(left, top) {
            document.onmousemove = function(event) {
                Mouse.x = left + event.pageX;
                Mouse.y = top + event.pageY;
            }
        })(left, top);
    }
};
