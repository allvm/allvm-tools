
all: A B

libA.so:
libB.so:
%.so: %.c
	cc $^ -o $@ -shared -fPIC -Wl,-soname,$@

A:
B:
%: main%.c lib%.so
	cc $^ -o $@ -L. -l$* -Wl,-rpath,.
