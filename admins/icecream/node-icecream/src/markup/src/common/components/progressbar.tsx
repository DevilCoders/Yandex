interface Props {
    values: number[];
    colors?: string[];
    animated?: boolean;
    big?: boolean;
}
export class ProgressBar extends React.Component<Props, {}> {
    static displayName = "ProgressBar";

    render(): JSX.Element {
        // empty progressbar
        if (this.props.values.length === 0) return <div/>;

        // classname
        let cls = "progress";
        if (this.props.animated) cls += " animated";
        if (this.props.big) cls += " big";

        // generate items
        let sum = 0;
        let items: JSX.Element[] = [];
        for (let idx in this.props.values) {
            // skip empties
            if (!this.props.values[idx]) continue;

            let style: React.CSSProperties = { width: `${this.props.values[idx]}%` };
            if (this.props.colors) style.backgroundColor = this.props.colors[idx];
            items.push(<div key={idx} style={style}/>);

            sum += this.props.values[idx];
        }

        if (sum < 100) {
            items.push(<div key="final" style={{width:`${100 - sum}%`, backgroundColor:"#eee"}}/>);
        }


        return (
            <div className={cls}>
                { items }
            </div>
        );
    }
}
