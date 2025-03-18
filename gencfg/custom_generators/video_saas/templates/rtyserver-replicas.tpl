{%- for geo, replicas, replica_id in data %}
                <Replica>
                    Servers: {{ replicas|join(' ') }}
                    Id: {{ replica_id }}
                    Priority: ${LOCATION and tostring((LOCATION=="{{ geo|upper }}") and 1 or 0) or 0}
                </Replica>
{%- endfor %}
