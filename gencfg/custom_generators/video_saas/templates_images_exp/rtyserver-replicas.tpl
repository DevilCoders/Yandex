{%- for geo, replicas in data %}
                <Replica>
                    Servers: {{ replicas|join(' ') }}
                    Id: {{ loop.index }}
                    Priority: ${LOCATION and tostring((LOCATION=="{{ geo|upper }}") and 1 or 0) or 0}
                </Replica>
{%- endfor %}
