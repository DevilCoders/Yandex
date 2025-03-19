const moment = require('vendor/moment/v2.21');
const params = ChartEditor.getParams();


module.exports = {
    xAxis: {
        type: 'datetime'
    },
    plotOptions: {
        line:{
            marker:{
                    enabled:true
                }
            }
    }
};