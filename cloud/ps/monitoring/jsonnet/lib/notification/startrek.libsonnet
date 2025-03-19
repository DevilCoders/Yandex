{
    description: error 'Check must have description field',

    template_name: 'startrek',
    template_kwargs: {
        status: [
            'WARN',
            'CRIT',
        ],
        queue: 'CLOUDPS',
        components: [
            'juggler',
        ],
    },
}

