
var adblockPrefix = 'http://test.local/';
var adblocktest = "http://test.local/static/img/not_crypt_me_please.png";
var adblocktest1 = "./testing/img/yes26px_anim.png";
var adblocktest2 = '../testing/img/yes26px_anim.png';
    $(document).on('click', '.com-extend', function() {
        var com = $(this).closest('.comm');
        $('span', this).html('<img src="/static/img/loader2.gif">');
        $.ajax({
            url: '/scripts/function/get_comment_ext.php',
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
