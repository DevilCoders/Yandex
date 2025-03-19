const moment = require('vendor/moment/v2.21');
const params = ChartEditor.getParams();
//tets

module.exports = {
   

    yAxis: [
        { // Primary yAxis
           
            title: {
                text: 'Total Subaccounts Counsumption ',
            },
            opposite: true
        }, 
        { // Secondary yAxis
            title: {
                text: 'Total Subaccounts Count',
            },

    
        }],

        plotOptions: {
        line:{
            marker:{
                    enabled:true
                }
            }
    }
    
};