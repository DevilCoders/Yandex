const statuses = ['ok', 'inactive'];

const N = 100;

export const SERVICES = {
    items: (new Array(N)).fill(0).map((val, index) => ({
        id: index + 1,
        name: `${'Test'.repeat((index % 10) + 1)}${index + 1}`,
        status: statuses[Math.min(index, statuses.length - 1)]
    })),
    total: N
};
