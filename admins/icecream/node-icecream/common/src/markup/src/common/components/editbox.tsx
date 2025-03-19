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


interface SimpleProps {
    initialValue?: string;
    placeholder?: string;
    clearOnEnter?: boolean;
    onEnter: (value: string) => void;
}
interface SimpleState {
    value: string;
}

// A simple edit box that does not expose it's state, just informs when data is updated
export class SimpleEditBox extends React.Component<SimpleProps, SimpleState> {
    static displayName = "SimpleEditBox";

    constructor() {
        super();
        this.state = {
            value: "",
        };
    }

    componentDidMount() {
        if (this.props.initialValue)
            this.setState({ value: this.props.initialValue });
    }

    onChange(value: string) {
        this.setState({ value: value });
    }

    onEnter() {
        this.props.onEnter(this.state.value);
        if (this.props.clearOnEnter)
            this.setState({ value: "" });
    }

    render() {
        return <EditBox value={this.state.value}
                        onChange={this.onChange.bind(this)}
                        onEnter={this.onEnter.bind(this)}
                        placeholder={this.props.placeholder}/>;
    }
}
