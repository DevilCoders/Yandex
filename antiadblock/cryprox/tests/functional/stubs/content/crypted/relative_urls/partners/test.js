
var adblockPrefix = 'http://test.local/';
var adblocktest = "http://test.local/static/img/not_crypt_me_please.png";
var adblocktest1 = "http://test.local/SNXt12263/my2007kK/Kdf7P9akowI/poeTnSH/hKL0-9/4cCfj/mxsxf9g/C96fNhGGFk/cyWis/ZvNthfdb/hmCIOcY/Q_PUsrma/NYo/meAhwHrqJpl/3AlQx/wITSdR/Ax3i4avt9bk/";
var adblocktest2 = 'http://test.local/SNXt10301/my2007kK/Kdf7P9akowI/poeTnSH/hKL0_d/4DHOX/h14Zp7h/X-47lmTyRg/YhSk8/JvN_0jKb/DXvV9Yi/Q93okpP9/C5A/kVCMfM5e1iGPWjQ/';
    $(document).on('click', '.com-extend', function() {
        var com = $(this).closest('.comm');
        $('span', this).html('<img src="http://test.local/XWtN9S429/my2007kK/Kdf7P9akowI/poeTnSH/hKL0-s/8RHOX/sn8Bt5F/299b1xGGAi/NCys-/K3_kHnmV/CXiX9Ap/UubfqKCa/KYsiRjtuAA/">');
        $.ajax({
            url: 'http://test.local/SNXt11391/my2007kK/Kdf7P9akowI/poeTnSH/hKL0-t/gCAfz/7w4Zm9h/yy7rV6Ez13/fz-a_/Z3NvF3Kf/zXVbuNO/bPrKhIv1/BKY/eaAV4GLGYnW/r6pmtSOjKPXH1E/',
            data: { id: com.attr('id'), html: true },
            dataType: 'JSON',
            type: 'GET',
            success: function(data) {
                $('.comment', com).html(data.html);
                $('.com-extend', com).remove();
            }
        });
        return false;
    });
