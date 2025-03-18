// usage
// $('.uploader').uploader();
(function () {
    function Uploader(options) {
        options = options || {};

        this.$el = $(options.el);
        this.$file = this.$el.find('input[type="file"]');
        this.$progress = this.$el.find('.progress').hide();
        this.$progressBar = this.$progress.find('.bar');

        this.fileId = null;
        this.fileName = null;
        this.$csrfToken = this.$el.closest('form').find('input[name="csrfmiddlewaretoken"]');

        this._bindEvents();
        this._resetFile();
    }

    Uploader.prototype = {
        _bindEvents: function () {
            var self = this;
            this.$file.on('change', function (e) {
                self._onFileSelect(e);
            });

            this.$el.on('click', '.js-remove', function (e) {
                self._deleteFile();
            });

            this.$el.on('reset', function () {
                self._resetFile();
            });

            this.$el.on('delete', function () {
                self._deleteFile();
            });
        },

        _toggleProgress: function (progress) {
            if (progress === false) {
                this.$progress.hide();
                return;
            }
            this.$progress.show();
            this.$progressBar.css({
                width: progress + '%'
            });
        },

        _onFileSelect: function (e) {
            var self = this;
            if (!$(this.$file)[0].files.length ) {
                return alert(this.data('message-no-file'));
            }

            function end(fileId) {
                self.fileId = fileId;
                self.fileName = self.$file.val();
                self._toggleDeletable(true);
            }

            function error() {
                alert('Error upload');
            }

            function progress(progress) {
                self._toggleProgress(progress);
            }

            this._upload(this.$file, this.$csrfToken)
                .then(end, error, progress)
                .always(function () {
                    self._toggleProgress(false);
                });
        },

        _deleteFile: function () {
            if (!this.fileId) {
                return;
            }

            var self = this;

            function end() {
                self._resetFile();
            }

            function error(e) {
                alert(e && e.responseText || 'Delete error');
            }

            this._delete(this.fileId, this.$csrfToken)
                .then(end, error);
        },

        _toggleDeletable: function (state) {
            if (state) {
                var $deleteFileButton = $('<a class="btn js-remove"><i class="icon-remove-sign"></i> ' + this.fileName + '</a>');
                this.$file.hide().after($deleteFileButton);
            } else {
                this.$file.show();
                this.$el.find('.js-remove').remove();
            }
        },

        /**
         * Resets file value
         * @private
         */
        _resetFile: function () {
            this._toggleDeletable(false);

            var $clone = this.$file.clone(true);
            this.$file.replaceWith($clone);
            this.$file = $clone;

            this.fileId = null;
            this.fileName = null;
        },

        /**
         * @param {jQuery} [$file]
         * @param {jQuery} [$inputs]
         * @returns {*}
         * @private
         */
        _upload: function ($file, $inputs) {
            var data = new FormData();
            data.append($file[0].name, $file[0].files[0]);
            $($inputs).each(function () {
                data.append(this.name, $(this).val());
            });

            return this._ajax(this.data('url-fileupload'), data);
        },

        /**
         * @param {String} [fileId]
         * @param {jQuery} [$inputs]
         * @returns {*}
         * @private
         */
        _delete: function (fileId, $inputs) {
            var data = new FormData();
            data.append('file', fileId);
            $($inputs).each(function () {
                data.append(this.name, $(this).val());
            });

            return this._ajax(this.data('url-filedelete'), data);
        },

        _ajax: function (url, data) {
            var dfd = $.Deferred();

            $.ajax({
                url: url,
                type: 'POST',
                xhr: function () {
                    var customXHR = $.ajaxSettings.xhr();
                    if (customXHR.upload) {
                        customXHR.upload.addEventListener('progress', function (e) {
                            dfd.notify(e.loaded / e.total);
                        }, false);
                    }
                    return customXHR;
                },
                data: data,
                cache: false,
                contentType: false,
                processData: false
            })
            .then(dfd.resolve, dfd.reject);

            return dfd.promise();
        },

        data: function (what) {
            return this.$el.data(what);
        }
    };

    $.fn.uploader = function () {
        $(this).each(function () {
            new Uploader({
                el: this
            });
        });
    };
})();
