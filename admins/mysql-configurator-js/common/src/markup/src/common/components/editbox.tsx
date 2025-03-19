interface Props {
    value: string;
    placeholder?: string;
    onChange: (value: string) => void;
    onEnter?: () => void;
}
export class EditBox extends React.Component<Props, {}> {
    static displayName = "EditBox";

    onChange(evt: any) {
        this.props.onChange(evt.target.value);
    }

    onKeyPress(evt: any) {
        if (this.props.onEnter && evt.nativeEvent.charCode === 13) {
            evt.preventDefault();
            evt.stopPropagation();
            this.props.onEnter();
        }
    }

    render(): JSX.Element {
        return <input
                    type="text"
                    onChange={this.onChange.bind(this)}
                    onKeyPress={this.onKeyPress.bind(this)}
                    placeholder={this.props.placeholder}
                    value={this.props.value}/>;
    }
}
