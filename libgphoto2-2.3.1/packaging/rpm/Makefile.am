# $Id: Makefile.am 9218 2006-09-28 02:29:16Z hun $
#
# create an RPM package out of an automake based source distribution
# � 2001 Hans Ulrich Niedermann <gp@n-dimensional.de>
#

EXTRA_DIST = package.spec.in @PACKAGE@.spec

DISTCLEANFILES = @PACKAGE@.spec

clean-local:
	rm -rf BUILD RPMS SPECS SOURCES SRPMS

@PACKAGE@.spec: package.spec
	cp $< $@

rpm: Makefile package.spec
if RPM_LIBUSB_DEVEL
	@echo "Building libgphoto2 RPM with USB support and dependency on the libusb RPM package."
else
	@echo "#############################################################################"
	@echo "# Building libgphoto2 RPM packages ***WITHOUT*** USB support.               #"
	@echo "# Install both the libusb and the libusb-devel RPM package for USB support. #"
	@echo "#############################################################################"
	@sleep 3
endif
	$(MAKE) clean
	mkdir -p BUILD SPECS SOURCES SRPMS
	for arch in i386 i486 i586 i686 alpha noarch; do \
		mkdir -p "RPMS/$${arch}"; \
	done
	$(MAKE) SOURCES/@PACKAGE@-@VERSION@.tar.gz SPECS/@PACKAGE@-@VERSION@.spec
	@RPMBUILD@ --define "_topdir $$(pwd)" -ba SPECS/@PACKAGE@-@VERSION@.spec

SOURCES/@PACKAGE@-@VERSION@.tar.gz: $(top_srcdir)/@PACKAGE@-@VERSION@.tar.gz
	ln -f $< $@

SPECS/@PACKAGE@-@VERSION@.spec: package.spec
	ln -f $< $@
