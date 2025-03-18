const TIME_LIMIT = 20;

export function timeIsOver(lastUpdateTime, limit = TIME_LIMIT) {
    return (new Date() - lastUpdateTime) / 1000 > limit;
}
