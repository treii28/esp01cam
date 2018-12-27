# esp01cam
an attempt to get a serial webcamera working on an esp01 with asyncwebserver

Set INITSPIFFS to 1 in platformio.ini to initialize the spiffs filesystem.

still getting memory errors with OTA and MDNS not included and debugging set to a bare minimum

stack trace:
```
0x4021ab4c: tcp_input at core/tcp_in.c line 435
0x4021f224: mem_malloc at core/mem.c line 136
0x4021e9ad: ip4_input at core/ipv4/ip4.c line 685
0x402180b8: ethernet_input_LWIP2 at netif/ethernet.c line 182
0x40217ef3: esp2glue_ethernet_input at glue-lwip/lwip-git.c line 439
0x4023ad02: ethernet_input at glue-esp/lwip-esp.c line 363
0x4023ad13: ethernet_input at glue-esp/lwip-esp.c line 371
```
