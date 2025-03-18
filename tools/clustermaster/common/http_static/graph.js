window.addEventListener('load', initGraphPage, false);

function initGraphPage() {
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

    var graphDocument = MasterElement.getSVGDocument();

    if (graphDocument) {

        var graphRect = graphDocument.documentElement.getAttribute('viewBox').split(' ');
        MasterElement.style.width = graphRect[2] + 'pt';
        MasterElement.style.height = graphRect[3] + 'pt';

        var background = graphDocument.getElementById('graph-background-node');

        if (background) {
            background.onmousedown = function(event) {
                if (event.button == 0) {
                    graphDocument.documentElement.onmousemove = function(e) {
                        document.body.scrollLeft += event.pageX - e.pageX;
                        document.body.scrollTop += event.pageY - e.pageY;
                    }
                    graphDocument.documentElement.onmouseup = function() {
                        graphDocument.documentElement.onmousemove = null;
                        graphDocument.documentElement.onmouseup = null;
                    }
                }
            }

            background.ondragstart = function() {
                return false;
            }

            background.onselectstart = function() {
                return false;
            }
        }

        var query = CM.getQueryString();
        var target = query["target"];
        if (target) {
            var graphNode = jQuery(graphDocument).find('.graph');
            var graph = new Graph(graphNode);
            var depth = query['depth'] && query['depth'] > -1 ? query['depth'] : Infinity;
            if (Graph.WalkDirections.indexOf(query['walk']) > -1)
                graph.filterBy(target.split(","), query['walk'], depth);
            else
                alert('To filter graph you must specify query parameter "walk=up" or "walk=down"');
        }

        for (var tag in set('a', 'g')) {
            for (var elements = graphDocument.getElementsByTagName(tag), i = 0; i < elements.length; ++i) {
                if (elements[i].getAttribute('class') !== 'node')
                    continue;

                (function(e, target) {
                    if (tag === 'a') {
                        e.setAttributeNS('http://www.w3.org/1999/xlink', 'xlink:href', urlroot + 'target/' + target + (worker ? '/' + worker : ''));
                        e.cmdparams = 'target=' + target + (worker ? '&worker=' + worker : '');

                        ActionMenu.assignControls([e]);
                    } else if (tag === 'g') {
                        e.cx = parseFloat(e.firstChild.getAttribute('cx'));
                        e.cy = parseFloat(e.firstChild.getAttribute('cy'));
                        e.rx = parseFloat(e.firstChild.getAttribute('rx'));
                        e.ry = parseFloat(e.firstChild.getAttribute('ry'));

                        Updater.register(target, function(v) {
                            CMPutStatePiechart(e, v);
                        });
                    }
                })(elements[i], elements[i].parentNode.getAttribute('id'));
            }
        }

        Mouse.assignDocument(graphDocument, MasterElement);

        ActionMenu.setUrlRoot(urlroot);
    }

    Updater.serv(stateurl);
}

function Graph(graphNode) {
    this.graphNode = graphNode;
    this.loadVertices(graphNode);
    this.loadEdges(graphNode);
}

Graph.WalkDirections = ['up', 'down']
Graph.MapToTraverseFunction = { up: "keepParents", down: "keepChildren" }

Graph.prototype.loadVertices = function(graphNode) {
    var result = graphNode
        .children()
        .filter(function() { return jQuery(this).attr('isNode'); })
        .map(function() { return { outgoing: [], incoming: [], node: this }; });
    this.vertices = {};
    for (var i = 0; i < result.length; i++)
        this.vertices[result[i].node.id] = result[i];
};

Graph.prototype.loadEdges = function(graphNode) {
    var self = this;
    var edges = graphNode
        .contents()
        .filter(function() { return this.nodeType == 8; }) //comments
        .filter(function() { return this.nodeValue.indexOf("&#45;&gt;") > -1; })
        .map(function() { return self.createEdge(this); });
    for (var i = 0; i < edges.length; i++) {
        this.vertices[edges[i].tail].outgoing.push(edges[i]);
        this.vertices[edges[i].head].incoming.push(edges[i]);
    }
};

Graph.prototype.filterBy = function(targets, walkDirection, depth) {
    var traverseFunction = Graph.MapToTraverseFunction[walkDirection];
    for (i = 0; i < targets.length; ++i) {
        this[traverseFunction](targets[i], depth);
    }
    this.purge();
    this.alignTopLeft();
}

Graph.prototype.keepChildren = function(id, depth) {
    if (!this.vertices[id].keep) {
        this.vertices[id].keep = true;
        if (depth > 0)
            this.vertices[id].outgoing.forEach(function(x) { this.keepChildren(x.head, depth - 1); }.bind(this));
    }
}

Graph.prototype.keepParents = function(id, depth) {
    if (!this.vertices[id].keep) {
        this.vertices[id].keep = true;
        if (depth > 0)
            this.vertices[id].incoming.forEach(function(x) { this.keepParents(x.tail, depth - 1); }.bind(this));
    }
}

Graph.prototype.purge = function() {
    for (id in this.vertices) {
        if (!this.vertices[id].keep) {
            this.vertices[id].node.remove();
            this.vertices[id].outgoing
                .concat(this.vertices[id].incoming)
                .forEach(this.removeEdge.bind(this));
            delete this.vertices[id];
        }
    }
}

Graph.prototype.removeEdge = function(edge) {
    this.removeItem(this.vertices[edge.tail].outgoing, edge);
    this.removeItem(this.vertices[edge.head].incoming, edge);
    edge.node.remove();
}

Graph.prototype.removeItem = function(array, item) {
    array.splice(array.indexOf(item), 1);
}

Graph.prototype.createEdge = function(comment) {
    var arr = comment.nodeValue.trim().split("&#45;&gt;");
    var div = jQuery("<div/>");
    return {
        tail: div.html(arr[0]).text(),
        head: div.html(arr[1]).text(),
        node: jQuery(comment).next()[0]
    };
}

Graph.prototype.leftPadding = 5;
Graph.prototype.topPadding = 5;

Graph.prototype.alignTopLeft = function() {
    var coords = this.getLeftTopOfGraph();
    var svgTop = this.getSvgTop();
    var offset = {
        x: coords.x - this.leftPadding,
        y: coords.y - svgTop - this.topPadding
    };
    this.shift(offset);
}

Graph.prototype.getLeftTopOfGraph = function() {
    var left = Infinity;
    var top = Infinity;
    for (id in this.vertices) {
        var coords = this.getLeftTopOfNode(this.vertices[id].node);
        left = Math.min(left, coords.x);
        top = Math.min(top, coords.y);
    }
    return { x: left, y: top };
}

Graph.prototype.getLeftTopOfNode = function(node) {
    var children = jQuery(node).children();
    for (var i = 0; i < children.length; i++) {
        switch (children[i].nodeName) {
            case "polygon":
                var x = Infinity;
                var y = Infinity;
                var points = children[i].points;
                for (var j = 0; j < points.numberOfItems; j++) {
                    x = Math.min(x, points.getItem(j).x);
                    y = Math.min(y, points.getItem(j).y);
                }
                return { x: x, y: y };
            case "ellipse":
                return {
                    x: children[i].getAttribute("cx") - children[i].getAttribute("rx"),
                    y: children[i].getAttribute("cy") - children[i].getAttribute("ry")
                }
        }
    }
    throw "No meaningful node in " + node.innerHTML;
}

Graph.prototype.shift = function(offset) {
    for (id in this.vertices) {
        this.shiftNode(this.vertices[id].node, offset);
        this.vertices[id].outgoing.forEach(function(e) {
            this.shiftNode(e.node, offset);
        }.bind(this));
    }
}

Graph.prototype.shiftNode = function(node, offset) {
    switch (node.nodeName){
        case "polygon":
            for (var i = 0; i < node.points.numberOfItems; i++) {
                node.points.getItem(i).x -= offset.x;
                node.points.getItem(i).y -= offset.y;
            }
            break;
        case "clipPath":
        case "g":
        case "a":
            for (var i = 0; i < node.children.length; i++)
                this.shiftNode(node.children[i], offset);
            break;
        case "ellipse":
            node.setAttribute("cx", node.getAttribute("cx") - offset.x);
            node.setAttribute("cy", node.getAttribute("cy") - offset.y);
            break;
        case "text":
            node.setAttribute("x", node.getAttribute("x") - offset.x);
            node.setAttribute("y", node.getAttribute("y") - offset.y);
            break;
        case "path":
            $(node).attr('transform', 'translate(' + (-offset.x) + ', ' + (-offset.y) + ')');
            break;
        default:
            throw "Unknown node: " + node.nodeName;
    }
}

Graph.prototype.getSvgTop = function() {
    var background = $('#graph-background-node', this.graphNode)[0];
    var y = Infinity;
    for (var i = 0; i < background.points.numberOfItems; i++)
        y = Math.min(y, background.points.getItem(i).y);
    return y;
}
