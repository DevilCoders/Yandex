interface Props {
    color: string;
}

export class ColorShower extends React.Component<Props, {}> {
    static displayName = "ColorShower";
    render(): JSX.Element {
        return <div className="colorshower" style={{backgroundColor: this.props.color}}/>;
    }
}
