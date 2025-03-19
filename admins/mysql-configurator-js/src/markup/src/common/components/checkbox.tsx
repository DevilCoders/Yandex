interface Props {
    value: boolean;
    onChange: (value: boolean) => void;
}
export class CheckBox extends React.Component<Props, {}> {
    static displayName = "CheckBox";

    onClick() {
        this.props.onChange(!this.props.value);
    }
    render(): JSX.Element {
        let cls = "checkbox";
        if (this.props.value) cls += " checked";

        return <div className={cls} onClick={this.onClick.bind(this)}>
            <div/>
        </div>;
    }
}
