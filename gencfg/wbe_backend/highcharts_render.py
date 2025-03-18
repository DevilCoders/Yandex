import time
import random
import copy

MAX_POINTS = 17000  # need so set some constant to this value (which meand maximim number of points per serie)


class ChartType(object):
    def __init__(self, tp, zoomable=False):
        assert (tp in ['line', 'area', 'column'])

        self.tp = tp
        self.zoomable = zoomable

    def render(self):
        if self.zoomable:
            result = """
                chart: {
                    type: '%s',
                    zoomType: 'x',
                },""" % self.tp
        else:
            result = """
                chart: {
                    type: '%s',
                },""" % self.tp

        return result


class Title(object):
    def __init__(self, title, subtitle=None):
        self.title = title
        self.subtitle = subtitle

    def render(self):
        result = """
            title: {
                text: '%s',
                x: -20, // center
            },""" % self.title

        if self.subtitle is not None:
            result += """
                subtitle: {
                    text: '%s',
                    x: -10 // center
                },""" % self.subtitle

        return result


class Axes(object):
    def __init__(self, tp, xaxistitle, yaxistitle, minvalue=None):
        assert (tp in ['linear', 'datetime'])

        self.tp = tp
        self.xaxistitle = xaxistitle
        self.yaxistitle = yaxistitle
        if minvalue is None:
            self.minvalue = 'null'
        else:
            self.minvalue = str(minvalue)

    def render(self):
        result = """
            yAxis: {
                type: 'linear',
                title: {
                    text: '%s',
                },
                min: %s,
            },""" % (self.yaxistitle, self.minvalue)

        if self.tp == 'linear':
            result += """
                xAxis: {
                    type: 'linear',
                    title: {
                        text: '%s'
                    },
                },""" % self.xaxistitle
        elif self.tp == 'datetime':
            result += """
                xAxis: {
                    type: 'datetime',
                    title: {
                        text: '%s'
                    },
                    dateTimeLabelFormats: {
                        millisecond: '%%H:%%M<br/>%%Y-%%m-%%d',
                        second: '%%H:%%M<br/>%%Y-%%m-%%d',
                        minute: '%%H:%%M<br/>%%Y-%%m-%%d',
                        hour: '%%H:%%M<br/>%%Y-%%m-%%d',
                        day: '%%H:%%M<br/>%%Y-%%m-%%d',
                        week: '%%H:%%M<br/>%%Y-%%m-%%d',
                        month: '%%H:%%M<br/>%%Y-%%m-%%d',
                        year: '%%H:%%M<br/>%%Y-%%m-%%d'
                    },
                },""" % self.xaxistitle

        return result


class Tooltip(object):
    def __init__(self, shared=False,
                 headerFormat='<span style="font-size: 10px">{point.key}</span><br/>',
                 pointFormat='<span style="color:{series.color}">T</span> {series.name}: <b>{point.y}</b><br/>'):
        self.shared = 'true' if shared else 'false'
        self.headerFormat = headerFormat
        self.pointFormat = pointFormat

    def render(self):
        result = """
            tooltip: {
                shared: %s,
                headerFormat: '%s',
                pointFormat: '%s',
            },""" % (self.shared, self.headerFormat, self.pointFormat)

        return result


class HistdbTooltip(Tooltip):
    def __init__(self):
        super(HistdbTooltip, self).__init__(
            headerFormat='<b>{series.name}</b><br>',
            pointFormat='{point.y: .f} {point.suffix}<br>{point.tag}<br>{point.dt}',
        )


class MongodbCurrentTooltip(Tooltip):
    def __init__(self):
        super(MongodbCurrentTooltip, self).__init__(
            headerFormat='<b>{series.name}</b><br>',
            pointFormat='<b>{point.y: .f} {point.suffix}</b><br>{point.extra}',
        )


class MongodbHistTooltip(Tooltip):
    def __init__(self):
        super(MongodbHistTooltip, self).__init__(
            shared=True,
            pointFormat='<span style="color:{series.color}; font-weight:bold;"> {series.name}</span>: <b>{point.y: .f}</b> {point.suffix}<br/>',
        )


class PlotOptions(object):
    def __init__(self, raw):
        self.raw = raw

    def render(self):
        return self.raw


class HistdbPlotOptions(PlotOptions):
    def __init__(self):
        super(HistdbPlotOptions, self).__init__("")


class MongodbCurrentPlotOptions(PlotOptions):
    def __init__(self):
        super(MongodbCurrentPlotOptions, self).__init__("""
            plotOptions: {
                column: {
                    stacking: 'normal',
                    groupPadding: 0,
                    pointPadding: 0,
                    borderWidth: 1,
                    turboThreshold : %s,
                }
            },
            legend: {
                layout: 'vertical',
                align: 'right',
                verticalAlign: 'middle',
                borderWidth: 0
            },""" % MAX_POINTS)


class MongodbHistPlotOptions(PlotOptions):
    def __init__(self, stacking=True):
        if stacking:
            self.stacking = "'normal'"
        else:
            self.stacking = "null"

        super(MongodbHistPlotOptions, self).__init__("""
            plotOptions: {
                area: {
                    stacking: %(stacking)s,
                    lineColor: '#666666',
                    lineWidth: 1,
                    marker: {
                        enabled: false,
                    },
                    turboThreshold : %(threshold)s,
                },
                spline : {
                    turboThreshold : %(threshold)s,
                    marker: {
                        enabled: false,
                    },
                },
                line : {
                    turboThreshold : %(threshold)s,
                    marker: {
                        enabled: false,
                    },
                },
                series : {
                    animation : false,
                },
            },""" % {'stacking': self.stacking, 'threshold': MAX_POINTS})


class Series(object):
    def __init__(self, name, data, xisdate=False, params=None):
        if params is None:
            params = {}
        assert (len(data) <= MAX_POINTS)
        for elem in data:
            if 'x' not in elem or 'y' not in elem:
                raise Exception("Invalid series elem %s" % elem)

        self.name = name
        self.data = copy.copy(data)
        self.xisdate = xisdate
        self.params = copy.copy(params)

        # convert values
        for elem in self.data:
            if xisdate:
                localt = time.localtime(elem['x'])
                elem['x'] = "Date.UTC(%s, %s, %s, %s)" % (
                localt.tm_year, localt.tm_mon - 1, localt.tm_mday, localt.tm_hour)
            elif not isinstance(elem['x'], (int, long, float)):
                elem['x'] = "'%s'" % elem['x']

            for k in elem.keys():
                if k == 'x':
                    continue
                if not isinstance(elem[k], (int, long, float)):
                    elem[k] = "'%s'" % elem[k]
                elif isinstance(elem[k], float):
                    elem[k] = round(elem[k], 2)

            elem['origy'] = elem['y']

    def render(self):
        elemsdata = []
        for elem in self.data:
            elemsdata.append('{' + ''.join(map(lambda x: '%s: %s, ' % (x, elem[x]), elem)) + '}, ')

        result = ""
        for k in self.params:
            result += """
                %s: '%s',""" % (k, self.params[k])

        result += """
            name: '%s',
            cropThreshold : %s,
            data: [ %s ],""" % (self.name, MAX_POINTS, ''.join(elemsdata))

        return result


class IModifySeries(object):
    def __init__(self):
        pass

    def render_button(self):
        raise Exception("Not implemented")

    def render_jscode(self):
        raise Exception("Not implemented")


class TModifySeriesOnCheck(object):
    def __init__(self, descr, jscode):
        self.descr = descr
        self.jscode = jscode
        self.myid = str(random.randint(0, 1 << 64))

    # html code to show input
    def render_button(self, chartid):
        result = "<input type='checkbox' id='%(myid)s' onchange='modifySeries%(chartid)s();'> %(descr)s" % {
            'myid': self.myid, 'chartid': chartid, 'descr': self.descr
        }

        return result

    # function that modifies data
    def render_jscode(self):
        result = """
function modifyElem%(myid)s(serie) {
    var x = document.getElementById("%(myid)s").checked;
    if (x) {
        %(jscode)s;
    }
}""" % {'myid': self.myid, 'jscode': self.jscode}

        return result


class TModifyToPercentCpuUsage(TModifySeriesOnCheck):
    def __init__(self, descr):
        jscode = """
            for (j = 0; j < serie.data.length; j++) {
                if (serie.data[j].host_power == 0) {
                    serie.data[j].y = 0;
                } else if (serie.data[j].host_power < serie.data[j].y) {
                    serie.data[j].y = 100.;
                } else {
                    serie.data[j].y = serie.data[j].y / serie.data[j].host_power * 100;
                }
            }
"""
        super(TModifyToPercentCpuUsage, self).__init__(descr, jscode)


class TModifyToInstanceMemoryUsage(TModifySeriesOnCheck):
    def __init__(self, descr):
        jscode = """
            for (j = 0; j < serie.data.length; j++) {
                if (serie.data[j].instance_count <= 0) {
                    serie.data[j].y = 0.0;
                } else {
                    serie.data[j].y /= serie.data[j].instance_count;
                }
            }
"""
        super(TModifyToInstanceMemoryUsage, self).__init__(descr, jscode)


class TModifySeriesOnSelect(object):
    def __init__(self, descr, data, jscode):
        self.descr = descr
        self.data = data
        self.jscode = jscode
        self.myid = str(random.randint(0, 1 << 64))

    def render_button(self, chartid):
        result = "<select id='%(myid)s' onchange='modifySeries%(chartid)s();'>\n" % {'myid': self.myid,
                                                                                     'chartid': chartid}

        for option_descr, option_value, option_selected in self.data:
            result += "<option %(selected)s value = %(value)s>%(descr)s</option>\n" % {
                'selected': 'selected' if option_selected else '',
                'value': option_value,
                'descr': option_descr,
            }

        result += "</select> %(descr)s" % {'descr': self.descr}

        return result

    def render_jscode(self):
        result = """
function modifyElem%(myid)s(serie) {
    var selector = document.getElementById("%(myid)s");
    var selectorValue = selector.options[selector.selectedIndex].value;
    %(jscode)s;
}""" % {'myid': self.myid, 'jscode': self.jscode}

        return result


class TSmoothOnSelect(TModifySeriesOnSelect):
    def __init__(self, descr, data):
        jscode = """
            smoothWidth = selectorValue;

            // create starting window
            var window = [];
            var windowIndex = 0;
            var windowSum = 0.0;
            for (var i = 0; i < smoothWidth; i++) {
                var index = i - smoothWidth / 2;
                if (index < 0) {
                    index = 0;
                }

                window.push(serie.data[index].y);
                windowSum += serie.data[index].y;
            }

            // go through serie and modify data
            // every new data point become average of [-smoothWidth/2 + curindex; smoothWidth/2 + curindex]
            for (var i = 0; i < serie.data.length; i++) {
                serie.data[i].y = windowSum / smoothWidth;

                var newindex = i + Math.floor(smoothWidth / 2) + smoothWidth % 2;
                if (newindex >= serie.data.length) {
                    newValue = windowSum / smoothWidth;
                } else {
                    newValue = serie.data[newindex].y;
                }

                windowSum -= window[windowIndex];
                window[windowIndex] = newValue;
                windowSum += window[windowIndex];

                windowIndex = (windowIndex + 1) % smoothWidth;
            }
"""
        super(TSmoothOnSelect, self).__init__(descr, data, jscode)


class TMedianOnSelect(TModifySeriesOnSelect):
    def __init__(self, descr, data, median_width):
        jscode = """
            var medianWidth = %(median_width)s;
            var medianValue = selectorValue;

            if (medianValue == -1) { // -1 means disabled
                return;
            }

            var newdata = new Array(serie.data.length);
            for (var i = 0; i < serie.data.length; i++) {
                newdata[i] = serie.data[i].y;
            }

            if (serie.data.length < medianWidth) {
                newdata.sort(function(a,b){return a-b;});
                medianIndex = Math.floor(medianValue * newdata.length);
                for (var i = 0; i < newdata.length; i++) {
                    newdata[i] = newdata[medianIndex];
                }
            } else {
                start = Math.floor(medianWidth / 2);
                end = start + medianWidth %% 2;
                for (var index = start; index < serie.data.length - end; index++) {
                    var wn = new Array(end + start);
                    for (var i = 0; i < wn.length; i++) {
                        wn[i] = serie.data[index - start + i].y;
                    }

                    wn.sort(function(a,b){return a-b;});

                    newdata[index] = wn[Math.floor(medianValue * wn.length)];
                }

                for (var index = 0; index < start; index++) {
                    newdata[index] = newdata[start];
                }
                for (var index = serie.data.length - end; index < serie.data.length; index++) {
                    newdata[index] = newdata[serie.data.length - end];
                }
            }

            for (var index = 0; index < serie.data.length; index++) {
                serie.data[index].y = newdata[index];
            }
""" % {'median_width': median_width}
        super(TMedianOnSelect, self).__init__(descr, data, jscode)


def rendermodifiers(modifiers, chartid):
    buttons_result = '\n'.join(map(lambda x: x.render_button(chartid), modifiers))

    modify_calls = '\n'.join(map(lambda x: "modifyElem%s(serie);" % x.myid, modifiers))

    mainfunc_result = """
function modifySeries%(chartid)s() {
    var chart = $('#%(chartid)s').highcharts();

    for (i = 0; i < chart.series.length; i++) {
        serie = chart.series[i];
        newdata = serie.data;

        for (j = 0; j < serie.data.length; j++) {
            newdata[j].y = newdata[j].origy;
        }

        %(modify_calls)s;

        for (j = 0; j < serie.data.length; j++) {
            newdata[j].y = Math.round(serie.data[j].y * 100) / 100;
        }

        serie.setData(newdata, redraw = false);
    }

    chart.redraw();
}
""" % {'chartid': chartid, 'modify_calls': modify_calls}

    subfuncs_result = '\n\n'.join(map(lambda x: x.render_jscode(), modifiers))

    return buttons_result, mainfunc_result, subfuncs_result


def rendergraph(charttype, title, axes, tooltip, plotoptions, series, modifiers):
    chartid = str(random.randint(0, 1 << 64))

    modifiers_buttons, modifiers_mainfunc, modifiers_subfuncs = rendermodifiers(modifiers, chartid)

    result = ""
    result += charttype.render()
    result += title.render()
    result += axes.render()
    result += tooltip.render()
    result += plotoptions.render()
    result += """
        series: [
            %s
        ],""" % (', '.join(map(lambda x: '{ %s }' % x.render(), series)))

    result = """
        <div id="%(chartid)s" style="width:100%%; height:400px;"></div>
        <script>
            $(function () {
                $('#%(chartid)s').highcharts({
                    %(result)s
                });
            });
            %(modifiers_mainfunc)s
            %(modifiers_subfuncs)s
        </script>
        %(modifiers_buttons)s""" % {'chartid': chartid, 'result': result, 'modifiers_buttons': modifiers_buttons,
                                    'modifiers_mainfunc': modifiers_mainfunc, 'modifiers_subfuncs': modifiers_subfuncs}

    return result


def renderall(graphs):
    result = """
<html>
    <head>
        <script src="https://ajax.googleapis.com/ajax/libs/jquery/1.8.2/jquery.min.js"></script>
        <script src="https://code.highcharts.com/highcharts.js"></script>
        <script src="https://code.highcharts.com/adapters/standalone-framework.js"></script>
        <script src="https://code.highcharts.com/highcharts.js"></script>
    </head>

    <body>
        %s
    </body>
</html>""" % ('\n'.join(graphs))

    return result
