##  Враппер для cli terraform

python-terraform - модуль python, предоставляющий оболочку cli `terraform`. Код основан на https://github.com/beelit94/python-terraform by beelit94@gmail.com.
Добавлена поддержка terraform 1.0, исправлены ошибки:
- https://github.com/beelit94/python-terraform/issues/81

## Использование
#### Вызов произвольной команды terraform

    from python_terraform import *
    t = Terraform()
    return_code, stdout, stderr = t.<cmd_name>(*arguments, **options)

**Note**: чтобы вызвать методы, имена которых совпадают с зарезервированными словами python, например `import`,
нужно использовать имя команды с суффиксом `_cmd`, например,
вызвать `import` можно так:

    from python_terraform import *
    t = Terraform()
    return_code, stdout, stderr = t.import_cmd(*arguments, **options)

или можно вызвать `cmd`, указав 1-м параметром имя команды:

    from python_terraform import *
    t = Terraform()
    return_code, stdout, stderr = t.cmd(<cmd_name>, *arguments, **options)

#### Передача аргументов:

    terraform apply plan.tfplan
        --> <instance>.apply('plan.tfplan')
    terraform import aws_instance.foo i-abcd1234
        --> <instance>.import('aws_instance.foo', 'i-abcd1234')

#### Примеры:
* для передачи флагов используется ```IsFlagged/None``` в качестве значения,

        terraform taint -allow-missing
           --> <instance>.taint(allow＿missing=IsFlagged)
        terraform taint
           --> <instance>.taint(allow＿missing=None) or <instance>.taint()
        terraform apply -no-color
           --> <instance>.apply(no_color=IsFlagged)

* параметры с булевым значениям принимают True or False,

        terraform apply -refresh=true --> <instance>.apply(refresh=True)

* если флаг используется несколько раз, передается список его значений,

        terraform apply -target=aws_instance.foo[1] -target=aws_instance.foo[2]
        --->
        <instance>.apply(target=['aws_instance.foo[1]', 'aws_instance.foo[2]'])

* параметр "var" принимает словарь,

        terraform apply -var='a=b' -var='c=d'
        --> tf.apply(var={'a':'b', 'c':'d'})

* параметры со значением None не используются.

#### Terraform Output

    from python_terraform import Terraform
    t = Terraform()
    return_code, stdout, stderr = t.<cmd_name>(capture_output=False)

## Примеры
#### 1. apply with variables a=b, c=d, refresh=false, no color in the output
In shell:

    cd /home/test
    terraform apply -var='a=b' -var='c=d' -refresh=false -no-color

In python-terraform:

    from python_terraform import *
    tf = Terraform(working_dir='/home/test')
    tf.apply(no_color=IsFlagged, refresh=False, var={'a':'b', 'c':'d'})

или

    from python_terraform import *
    tf = Terraform()
    tf.apply(chdir='/home/test', no_color=IsFlagged, refresh=False, var={'a':'b', 'c':'d'})

или

    from python_terraform import *
    tf = Terraform(working_dir='/home/test', variables={'a':'b', 'c':'d'})
    tf.apply(no_color=IsFlagged, refresh=False)

#### 2. fmt command, diff=true
In shell:

    cd /home/test
    terraform fmt -diff=true

In python-terraform:

    from python_terraform import *
    tf = Terraform(working_dir='/home/test')
    tf.fmt(diff=True)

