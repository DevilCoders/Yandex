interface Props {
    values: string[];
    selected: number;
    vertical?: boolean;
    onChange: (selected: number) => void;
}
export class Selector extends React.Component<Props, {}> {
    static displayName = "Selector";

    onClick(index: number) {
        this.props.onChange(Number(index));
    }

    render(): JSX.Element {
        let items: JSX.Element[] = [];
        for (let i in this.props.values) {
            let cls = "";
            if (Number(i) === this.props.selected) cls = "selected";
            items.push(<div key={i} className={cls} onClick={this.onClick.bind(this,i)}>
                { this.props.values[i] }
            </div>);
        }

        let cls = "selector";
        if (this.props.vertical) cls += " vertical";

        return <div className={cls}>
            { items }
        </div>;
    }
}
