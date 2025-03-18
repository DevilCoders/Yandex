# proto-file: ci/tasklet/registry/demov2/woodflow/furniture_factory/proto/furniture_factory.proto
# proto-message: Output

furnitures {
    type: "Шкаф"
    description: "Шкаф из 3 досок, полученных из материала 'бревно из дерева Берёза речная', произведенного Братья - Лесопилы и Лесопилка - Можайское шоссе"
}

furnitures {
    type: "Шкаф"
    description: "Шкаф из 3 досок, полученных из материала 'бревно из дерева Ель европейская', произведенного Братья - Лесопилы и Лесопилка - Можайское шоссе"
}

remain {
    seq: 2
    source {
        name: "бревно из дерева Ель европейская"
    }
    producer: "Братья - Лесопилы"
}

sandbox_resources {
    id: 1
    type: "BUILD_LOGS"
    attributes {
        key: "ttl"
        value: "1"
    }
}
