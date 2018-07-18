# SPDX-License-Identifier: LGPL-2.1-or-later
# Copyright © 2007-2018 ANSSI. All Rights Reserved.
if HTML_DOC
htmldocdir = $(datadir)/clip-lib/html
htmldoc_DATA = html/index.html
html/index.html : doxyfile_html
	doxygen doxyfile_html
HTMLINST=cp -dpR html/* $(DESTDIR)$(htmldocdir)/
else
HTMLINST=
endif !HTML_DOC
install-data-hook:
	$(HTMLINST)

clean-local:
	rm -fr html
