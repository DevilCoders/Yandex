# Примечания

Примечания бывают разные.

{% note %}

Самые простые.

{% endnote %}

И посложнее.

{% note warning %}

Примечаниями часто злоупотребляют.

Не стоит этого делать.

- Причина 1.

- Причина 2.

{% endnote %}


Однако, всё равно пусть работают.

{% note "JFYI" %}
Примечание может иметь заголовок.
{% endnote %}

Или же

{% note tip "" %}
не иметь его вовсе.
{% endnote %}

{% note tip %}

Вот, например, таблица.

First Header | Second Header
------------ | -------------
Content from cell 1 | Content from cell 2
Content in the first column | Content in the second column
 
{% endnote %}

Или так.

- Раз.

  {% note warning %}
  
  Будьте осторожны.
  
  {% note warning %}
  
  Это плохая идея.
  
  {% endnote %}
    
  {% endnote %}
  
- В список

  - Можно вложить список
  
    {% note tip %}
    
    А в него - примечание.
    
    - А в него - ещё список.
   
      - Раз.
      
      - Два.
    
    {% endnote %}
  
Внутри blockquote примечания формально тоже могут быть

> Только
>
> {% note alert %}  
>
> Зачем?
>
> {% endnote %}
>
> Причём, blockquote тоже бывают вложенными:
> >
> > {% note alert %}
> >
> > Это уже *вообще* **перебор**.
> > 
> > {% endnote %}