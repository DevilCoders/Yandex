import tarfile
from pathlib import Path
from typing import Optional

from .validation import validate_template, validate_variable


def check_templates(templates_path: str, variables_path: Optional[str]):
    templates_path = Path(templates_path)
    if not templates_path.exists():
        raise ValueError(f'\nФайл с шаблонами "{templates_path}" не найден')
    if not templates_path.name.endswith('.tsv'):
        raise ValueError(f'\nОписание шаблонов "{templates_path}" имеет расширение, отличное от TSV')

    if variables_path:
        variables_path = Path(variables_path)
        if not variables_path.exists() or not variables_path.is_dir():
            raise ValueError(f'\nДиректория "{variables_path}" не найдена')
        variable_sources = [f.name for f in variables_path.iterdir()]
    else:
        variable_sources = []

    for source in variable_sources:
        if not source.endswith('.tsv'):
            continue
        if not Path(variables_path, source).is_file():
            raise ValueError(f'\nОписание переменной "{source}" не найдено')
        for variable in Path(variables_path, source).read_text().splitlines():
            valid, msg = validate_variable(variable)
            if not valid:
                raise ValueError(f'\nНекорректная переменная "{variable}"\n{msg}')

    for template in templates_path.read_text().splitlines():
        valid, msg = validate_template(template, variable_sources)
        if not valid:
            raise ValueError(f'\nНекорректный шаблон "{template}"\n{msg}')


def prepare_templates(templates_path: str, variables_path: Optional[str], output_path: str = 'templates.tar.gz'):
    check_templates(templates_path, variables_path)

    with tarfile.open(output_path, mode='w:gz') as tar:
        tar.add(templates_path, arcname='templates.tsv')
        if variables_path is not None:
            for source_path in Path(variables_path).iterdir():
                if not source_path.name.endswith('.tsv'):
                    continue
                tar.add(str(source_path), arcname=f'variables/{source_path.name}')

    print(f'Шаблоны успешно проверены и помещены в архив {output_path}.\n'
          'Теперь вы можете загрузить этот архив в датасферу, воспользовавшись соответствующим magic\'ом')
