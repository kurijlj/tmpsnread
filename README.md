tmpsnread
=========
Terminal application that displays current temperature sensors readings in csv
format. Idea behind this, was to have tool that prints temperature sensors data
in format that can be easily fetched to other programs for displaying or
processing (e.g. conky).


Requirements
============
lm_sensors >= 3.3.2


Installation
============
Compile with:
```bash
$ RELEASE=true make tmpsnread
```

install with (as root):
```bash
$ strip --strip-unneeded ./tmpsnread
$ install -m 755 ./tmpsnread /usr/bin/
```
