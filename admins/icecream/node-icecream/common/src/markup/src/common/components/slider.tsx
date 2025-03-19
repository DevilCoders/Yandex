interface Props {
    min?: number;
    max?: number;
    points?: number[];
    step?: number;
    value: number;
    showInput?: boolean;
    onChange: (value: number) => void;
}

export class Slider extends React.Component<Props, {}> {
    bar: HTMLElement;
    pts: number[];

    constructor() {
        super();
        this.handleInputChange = this.handleInputChange.bind(this);
        this.handleOnBlur = this.handleOnBlur.bind(this);
    }

    public static defaultProps: Partial<Props> = {
        step: 1,
    };

    onMouseMove(e: React.MouseEvent<HTMLElement>) {
        // calculate the position
        let frac = (e.clientX
                    - this.bar.offsetLeft
                    + this.bar.offsetParent.scrollLeft)
                        / this.bar.offsetWidth;
        if (frac < 0) frac = 0;
        if (frac > 1) frac = 1;

        let idx = Math.round((this.pts.length - 1) * frac);
        this.props.onChange(this.pts[idx]);
    }

    onMouseDown(e: React.MouseEvent<HTMLElement>) {
        e.preventDefault();
        document.onmousemove = this.onMouseMove.bind(this);
        document.onmouseup = this.onMouseUp.bind(this);
        this.onMouseMove(e);
    }

    onMouseUp(e: React.MouseEvent<HTMLElement>) {
        e.preventDefault();
        document.onmousemove = null;
        document.onmouseup = null;
    }

    onDragStart() {
        return false;
    }

    setBar(bar: HTMLElement) {
        this.bar = bar;
    }

    genPoints() {
        // Generates equally spaced points
        let pts: number[] = [];
        for (let i = this.props.min;
                i <= this.props.max;
                i += this.props.step) {
            pts.push(i);
        }
        return pts;
    }

    handleInputChange(event: React.ChangeEvent<HTMLInputElement>) {
        let value: number = Number(event.target.value);
        this.props.onChange(value);
    }

    handleOnBlur(e: React.FocusEvent<HTMLInputElement>) {
        let value: number;
        if (this.props.value > this.props.max ) {
            value = this.props.max;
        } else if (this.props.value < this.props.min) {
            value = this.props.min;
        } else {
            value = this.props.value;
        }
        this.props.onChange(value);
    }

    render(): JSX.Element {
        if (!this.pts) {
            this.pts = this.props.points || this.genPoints();
        }

        let pos: number = 0;
        for (let i = 0; i < this.pts.length; i++) {
            if (this.pts[i] >= this.props.value) {
                pos = i;
                break;
            }
        }
        let output: JSX.Element;
        if (this.props.showInput === true) {
            let max: number;
            if (this.props.max !== undefined) {
                max = this.props.max;
            } else {
                max = Infinity;
            }
            output = <input type="number"
                        className="simpleInput"
                        min={0}
                        max={ max }
                        step={this.props.step}
                        value={ this.props.value }
                        onBlur={ this.handleOnBlur }
                        onChange={ this.handleInputChange }
                    />;
        } else {
              output = <div className="simpleOutput" >
                            { this.props.value }
                        </div>;
        }
        let style = {left: `${pos / (this.pts.length - 1) * 100}%`};

        return <div className="slider">
            <div ref={this.setBar.bind(this)}
                onMouseDown={this.onMouseDown.bind(this)}
                onDragStart={this.onDragStart} >
             <div style={style} /></div>
            {output}
        </div>;
    }
}
