from cloud.marketplace.common.yc_marketplace_common.client import MarketplacePrivateClient
from cloud.marketplace.common.yc_marketplace_common.models.form import FormCreateRequest
from cloud.marketplace.common.yc_marketplace_common.utils.ids import generate_id


def test_form_crud(marketplace_private_client: MarketplacePrivateClient):
    example = {
        "title": "Форма для Убунту по умолчанию",
        "billingAccountId": generate_id(),
        "metadata_template": "#cloud-config\ndatasource:\n  Ec2:\n    strict_id: false\nssh_pwauth: no\nusers:"
                             "\n- name: {{ login }}\n  sudo: ALL=(ALL) NOPASSWD:ALL\n  shell: /bin/bash\n  "
                             "ssh-authorized-keys:\n  - {{ key }}\n",
        "fields": [
            {
                "field": "login",
                "control": "input",
                "label": "Логин",
                "placeholder": "",
                "hint": "",
                "default": "ubuntu",
            },
            {
                "field": "key",
                "control": "textarea",
                "label": "SSH-ключ",
                "placeholder": "Открытый ключ. Должен начинаться с 'ssh-rsa', 'ssh-ed25519', 'ecdsa-sha2-nistp256', "
                               "'ecdsa-sha2-nistp384' или 'ecdsa-sha2-nistp521'.",
                "hint": "Скопируйте в поле содержимое файла с открытым ключом, "
                        "который был создан при генерации ключей для доступа по SSH.",
            },
        ],
        "schema": {
            "definitions": {},
            "$schema": "http://json-schema.org/draft-07/schema#",
            "$id": "http://example.com/root.json",
            "type": "object",
            "required": [
                "login",
                "key",
            ],
            "properties": {
                "login": {
                    "$id": "#/properties/login",
                    "allOf": [
                        {
                            "type": "string",
                            "pattern": "^(.*)$",
                        },
                        {
                            "type": "string",
                            "pattern": "^(a?)$",
                        },
                    ],
                },
                "key": {
                    "$id": "#/properties/key",
                    "type": "string",
                    "pattern": "^(.+)$",
                },
            },
        },
    }

    op = marketplace_private_client.create_form(FormCreateRequest(example))
    f_id = op.metadata["formId"]
    form = marketplace_private_client.get_form(f_id)

    assert form.title == example["title"]

    # f_l = marketplace_private_client.list_form()
    # assert len(f_l.forms) == 1
