Look inside a file with flatbuffer data and figure out how much space every field occupies

example/lib -- Library with a flatbuffer schema.
example/build -- Binary that creates some data with that schema.
example/introspect -- Binary that does size introspection.

How to try:
    * Build and run example/binary. It will create monster.fb file.
    * Bulld example/introspect and run it like
        ./example/introspect/introspect ./example/build/monster.fb
        or
        ./example/introspect/introspect ./example/build/monster.fb --verbose

