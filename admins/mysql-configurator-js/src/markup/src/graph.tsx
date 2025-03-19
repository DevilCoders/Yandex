interface GraphProps {
    name: string;
    timestamps: number[];
    values: number[];
    scale: number;
}

interface Item {
    x: number;
    y: number;
}

function suffixFormatter(val: number, axis: any) {
    if (val >= 1000000)
        return (val / 1000000).toFixed(axis.tickDecimals) + " M";
    else if (val >= 1000)
        return (val / 1000).toFixed(axis.tickDecimals) + " k";
    else
        return val.toFixed(axis.tickDecimals);
}

export class Graph extends React.Component<GraphProps, {}> {
    static displayName = "Graph";
    plot: any;

    graphRef(r: HTMLElement) {
        // combine items
        let items = this.props.timestamps.map((val, idx) => {
            return [
                val * 1000 + 10800000,
                this.props.values[idx],
            ];
        });

        // filter items
        if (this.props.scale === 1 ||
            this.props.scale === 5 ||
            this.props.scale === 10) {
            let d = new Date();
            let t = d.getTime() - this.props.scale * 60 * 1000 + 10800000;
            items = items.filter(function(x){
                return x[0] >= t;
            });
        }

        if (this.plot) {
            this.plot.setData([items]);
            this.plot.setupGrid();
            this.plot.draw();
        } else {
            this.plot = $.plot($(r), [items], {
                xaxis: {
                    mode: "time",
                },
                yaxis: {
                    min: 0,
                    tickFormatter: suffixFormatter,
                },
                series: {
                    shadowSize: 0,
                },
                colors: ["#9d5656"],
                grid: {
                    borderWidth: 1,
                },
            });
        }
    }

    render(): JSX.Element {
        return <div className="graph">
            <h4>{ this.props.name }</h4>
            <div ref={this.graphRef.bind(this)}/>
        </div>;
    }
}
