import { withStyles } from '@material-ui/core/styles';
import Slider from '@material-ui/core/Slider';

const YandexSlider = withStyles({
    root: {
      color: '#555',
      height: 3,
      padding: '13px 0',
    },
      active: {},
      valueLabel: {
        left: 'calc(-50% + 12px)',
        top: -22,
        '& *': {
          background: 'transparent',
          color: '#000',
        },
      },
      track: {
        height: 2,
      },
      rail: {
        height: 2,
        opacity: 0.5,
        backgroundColor: '#bfbfbf',
      },
      mark: {
        backgroundColor: '#bfbfbf',
        height: 8,
        width: 1,
        marginTop: -3,
      },
      markActive: {
        opacity: 1,
        backgroundColor: 'currentColor',
      }


}
    
    )(Slider);
  
  export default YandexSlider;