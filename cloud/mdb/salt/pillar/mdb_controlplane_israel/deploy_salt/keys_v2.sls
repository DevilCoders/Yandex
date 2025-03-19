data:
    config:
        salt:
            master:
                private: {{ salt.lockbox.get('bcnd669s12ga8ibupdt6').private | tojson }}
                public: {{ salt.lockbox.get('bcnd669s12ga8ibupdt6').public | tojson }}
