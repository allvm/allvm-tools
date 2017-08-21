
default: create_allexes

libA.so:
libB.so:
%.so: %.c
	cc $^ -o $@ -shared -fPIC -Wl,-soname,$@

A:
B:
%: main%.c lib%.so
	cc $^ -o $@ -L. -l$* -Wl,-rpath,.

.PHONY: default create_allexes clean

create_allexes: A B



clean:
	-rm -f A B libA.so libB.so
