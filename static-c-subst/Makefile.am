noinst_LIBRARIES = libstatic-c-subst.a

libstatic_c_subst_a_SOURCES = getaddrinfo.c getpwuid.c getgrnam.c dlopen.c

check_PROGRAMS = gai_tests
gai_tests_SOURCES = tests.c

.PHONY: tests
tests: gai_tests
	./gai_tests
