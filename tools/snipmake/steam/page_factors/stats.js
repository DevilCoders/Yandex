Stats = {
    average: function (arr) {
        return Stats.sum(arr) / arr.length;
    },
    median: function (arr) {
        if (!arr.length) {
            return NaN;
        }
        if (arr.length & 1) {
            return arr[Math.floor(arr.length / 2)];
        }
        return (arr[arr.length / 2 - 1] + arr[arr.length / 2]) / 2;
    },
    variance: function (arr, average) {
        if (!arr.length) {
            return NaN;
        }
        var res = 0;
        for (var i = 0; i < arr.length; ++i) {
            res += (arr[i] - average) * (arr[i] - average);
        }
        return res / (arr.length - 1 || 1);
    },
    sum: function (arr) {
        var res = 0;
        for (var i = 0; i < arr.length; ++i) {
            res += arr[i];
        }
        return res;
    },
    min: function (arr) {
        return (arr.length? arr[0]: NaN);
    },
    max: function (arr) {
        return (arr.length? arr[arr.length - 1]: NaN);
    },
    freq: function (hash) {
        var res;
        for (var key in hash) {
            if (typeof(res) == "undefined") {
                res = key;
            } else {
                if (hash[key] > hash[res]) {
                    res = key;
                }
            }
        }
        return (typeof(res) == "undefined"? NaN: res);
    },
    safeFloat: function(fl) {
        if (isNaN(fl)) {
            return 0;
        }
        if (!isFinite(fl)) {
            return Number.MAX_VALUE;
        }
        return fl;
    },
};
