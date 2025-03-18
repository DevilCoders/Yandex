const easeOutQuadSimple = t => {
    return t * (2 - t);
};

const easeOutQuad = (currentTime, fromValue, toValue, fullTime) => {
    const simpleTime = currentTime / fullTime;
    const coeff = easeOutQuadSimple(simpleTime);

    return fromValue + ((toValue - fromValue) * coeff);
};

const SCROLL_OFFSET = 30;
const SCROLL_PERIOD = 10;

let scrollFunction,
    animationMap = new Map();

scrollFunction = (element, offset, delay, cb) => {
    const fromValue = element.scrollTop;
    const toValue = offset - SCROLL_OFFSET;

    let currentTime = 0,
        interval = animationMap.get(element),
        stopAnimation = () => {
            clearInterval(interval);
            animationMap.delete(element);
            element.removeEventListener('wheel', stopAnimation);
            if (typeof cb === 'function') {
                cb();
            }
        };

    if (interval) {
        stopAnimation();
    }

    interval = setInterval(() => {
        currentTime += SCROLL_PERIOD;
        element.scrollTop = easeOutQuad(currentTime, fromValue, toValue, delay);
        if (currentTime === delay) {
            stopAnimation();
        }
    }, SCROLL_PERIOD);
    animationMap.set(element, interval);

    element.addEventListener('wheel', stopAnimation);
};

export const scroll = scrollFunction;
