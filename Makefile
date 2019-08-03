CONTIKI_PROJECT = smart-thermostat
all: $(CONTIKI_PROJECT)

WITH_UIP6=1
UIP_CONF_IPV6=1


APPS = smart-thermostat
APPS += er-coap-13
APPS += erbium

CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"

CFLAGS += -DUIP_CONF_IPV6_RPL=1
CFLAGS += -DWITH_COAP=13
CFLAGS += -DREST=coap_rest_implementation
CFLAGS += -DUIP_CONF_TCP=0

# linker optimizations
SMALL=1

# APPS += rest-coap
# CFLAGS += -DWITH_COAP

CONTIKI = ../..


include $(CONTIKI)/Makefile.include

# border router rules
$(CONTIKI)/tools/tunslip6:	$(CONTIKI)/tools/tunslip6.c
	(cd $(CONTIKI)/tools && $(MAKE) tunslip6)

connect-router:	$(CONTIKI)/tools/tunslip6
	sudo $(CONTIKI)/tools/tunslip6 aaaa::1/64

connect-router-cooja:	$(CONTIKI)/tools/tunslip6
	sudo $(CONTIKI)/tools/tunslip6 -a 127.0.0.1 aaaa::1/64

connect-minimal:
	sudo ip address add fdfd::1/64 dev tap0