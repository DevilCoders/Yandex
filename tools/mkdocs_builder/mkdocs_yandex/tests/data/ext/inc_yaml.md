
## Expected: line 1

{% code "include.yml" lang="yaml" lines="1" %}

## Expected: lines 1 and 5

{% code "include.yml" lang="yaml" lines="1,5" %}

## Expected: lines 1-3

{% code "include.yml" lang="yaml" lines="1-3" %}

## Expected: lines 6-8

{% code "include.yml" lang="yaml" lines="6-" %}

## Expected: lines 1, 3-4, 7-8

{% code "include.yml" lang="yaml" lines="1,3-4,7-8" %}

## Expected: error
{% code "include.yml" lang="yaml" lines="line 2" %}

## Expected: lines 7, 8

{% code "include.yml" lang="yaml" lines="line 6-" %}

## Expected: line 3

{% code "include.yml" lines="line 2-line 4" %}
