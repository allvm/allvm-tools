
default: A B

libA.so:
libB.so:
%.so: %.c
	$(CC) $^ -o $@ -shared -fPIC -Wl,-soname,$@

A:
B:
%: main%.c lib%.so
	$(CC) $^ -o $@ -L. -l$* -Wl,-rpath,$(DESTDIR)/lib

.PHONY: default clean

clean:
	-rm -f A B libA.so libB.so

install: default
	install -Dm755 A B -t $(DESTDIR)/bin/
	install -Dm755 lib{A,B}.so -t $(DESTDIR)/lib
