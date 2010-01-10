EXTRA_DIST = sydbox.1.txt sydbox.1.xml sydbox.1 \
	     logo.svg \
	     paludis-ipv6.conf paludis-noipv6.conf \
	     sydbox.conf

BUILT_SOURCES = sydbox.1

MANPAGE_XSL := /etc/asciidoc/docbook-xsl/manpage.xsl

all: paludis.conf sydbox.1

sydbox.1.xml: sydbox.1.txt
	$(AM_V_GEN)
	$(AM_V_at)asciidoc -d manpage -b docbook $<

sydbox.1: sydbox.1.xml
	$(AM_V_GEN)
	$(AM_V_at)xsltproc --nonet "$(MANPAGE_XSL)" $<

dist_man_MANS= sydbox.1
sysconf_DATA = sydbox.conf

if WANT_IPV6
paludis.conf: paludis-ipv6.conf
	$(AM_V_GEN)
	$(AM_V_at)cat $< > $@
else
paludis.conf: paludis-noipv6.conf
	$(AM_V_GEN)
	$(AM_V_at)cat $< > $@
endif

sydsharedir= $(datadir)/sydbox
sydshare_DATA= paludis.conf
