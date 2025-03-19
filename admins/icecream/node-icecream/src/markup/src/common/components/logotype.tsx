interface Props {
    text: string;
    onClick: () => void;
}

export class Logotype extends React.Component<Props, {}> {
    render() {
        return <div className="logotype" onClick={ this.props.onClick }>
            <img src="/logotype.svg"/> { this.props.text }
        </div>;
    }
}
