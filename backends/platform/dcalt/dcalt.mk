SCRAMBLE = $(KOS_BASE)/utils/scramble/scramble
CDRECORD = cdrecord
GENISOIMAGE = genisoimage
MAKEIP = makeip

all: $(EXECUTABLE)

dcalt-dist: all
	$(MKDIR) dcalt-dist
	$(STRIP) $(EXECUTABLE) -o dcalt-dist/scummvm.elf
	$(OBJCOPY) -S -R .stack -O binary $(EXECUTABLE) dcalt-dist/scummvm.bin
	$(MKDIR) dcalt-dist/cd
	$(SCRAMBLE) dcalt-dist/scummvm.bin dcalt-dist/cd/scummvm.bin
	$(MKDIR) dcalt-dist/cd/docs
	$(CP) $(KOS_BASE)/AUTHORS dcalt-dist/cd/docs/KOSAUTHORS
	if [ x"$(VER_EXTRA)" = xgit ]; then \
          ver="GIT"; \
        else ver="V$(VERSION)"; fi; \
        if expr "$$ver" : V...... >/dev/null; then \
          ver="V$(VER_MAJOR).$(VER_MINOR).$(VER_PATCH)"; fi; \
        sed -e 's/[@]VERSION[@]/'"$$ver"/ -e 's/[@]DATE[@]/$(shell date '+%Y%m%d')/' < $(srcdir)/backends/platform/dcalt/ip.txt.in > dcalt-dist/ip.txt
	IP_TEMPLATE_FILE=$(srcdir)/backends/platform/dcalt/IP.TMPL $(MAKEIP) dcalt-dist/ip.txt dcalt-dist/IP.BIN
	$(CP) $(srcdir)/backends/platform/dcalt/burn.sh dcalt-dist
ifdef DIST_FILES_THEMES
	$(MKDIR) dcalt-dist/cd/themes
	$(CP) $(DIST_FILES_THEMES) dcalt-dist/cd/themes
endif
ifdef DIST_FILES_VKEYBD
	$(MKDIR) dcalt-dist/cd/vkeybd
	$(CP) $(DIST_FILES_VKEYBD) dcalt-dist/cd/vkeybd
endif
ifdef DIST_FILES_ENGINEDATA
	$(MKDIR) dcalt-dist/cd/extra
	$(CP) $(DIST_FILES_ENGINEDATA) dcalt-dist/cd/extra
endif
ifdef DIST_FILES_DOCS
	$(MKDIR) dcalt-dist/cd/docs
	$(CP) $(DIST_FILES_DOCS) dcalt-dist/cd/docs
endif
ifeq ($(DYNAMIC_MODULES),1)
	$(MKDIR) dcalt-dist/cd/plugins
	$(MKDIR) dcalt-dist/cd/plugins
	for i in $(PLUGINS); do $(STRIP) --strip-debug $$i -o dcalt-dist/cd/plugins/`basename $$i`; done
endif
