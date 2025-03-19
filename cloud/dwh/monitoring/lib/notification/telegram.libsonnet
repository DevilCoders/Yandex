{
    description: error 'Check must have description field',

    template_name: error 'Template name should be set',
    template_kwargs: {
        status: [
            'WARN',
            'CRIT',
        ],
        login: [
            'datalake-and-analytics',
        ],
        method: [
            'telegram',
        ],
    },
}
