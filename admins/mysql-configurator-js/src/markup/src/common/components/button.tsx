interface Props {
    text: string;
    primary?: boolean;
    onClick: () => void;
    disabled?: boolean;
}
export class Button extends React.Component<Props, {}> {
    static displayName = "Button";

    onClick(evt: any) {
        evt.preventDefault();
        if (!this.props.disabled) {
            this.props.onClick();
        }
    }

    render(): JSX.Element {
        let cls = "";
        if (this.props.primary) cls = "primary";
        if (this.props.disabled) cls = "disables";
        return <button className={cls} onClick={this.onClick.bind(this)}>{ this.props.text }</button>;
    }
}
