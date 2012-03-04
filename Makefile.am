
################ build ########################3

INCLUDES = $(WX_CXXFLAGS)

cb_plugin_lib_LTLIBRARIES = libdebugger_gdbmi.la
libdebugger_gdbmi_la_SOURCES = src/actions.cpp  src/cmd_queue.cpp  src/cmd_result_parser.cpp	\
				src/cmd_result_tokens.cpp  src/config.cpp  src/definitions.cpp	\
				src/escape.cpp  src/events.cpp  src/frame.cpp  src/gdb_executor.cpp	\
				src/helpers.cpp src/plugin.cpp  src/updated_variable.cpp
				
noinst_HEADERS = src/config.h \
							src/frame.h \
							src/helpers.h \
							src/gdb_executor.h \
							src/cmd_queue.h \
							src/updated_variable.h \
							src/cmd_result_parser.h \
							src/actions.h \
							src/definitions.h \
							src/events.h \
							src/escape.h \
							src/cmd_result_tokens.h \
							src/plugin.h

libdebugger_gdbmi_la_LDFLAGS = -avoid-version -shared -no-undefined


################ resources ########################3

cb_plugin_data_DATA = debugger_gdbmi.zip

debugger_gdbmi_RESOURCES = resource/debugger_gdbmi_menu.xrc resource/manifest.xml

EXTRA_DIST = $(debugger_gdbmi_RESOURCES)
CLEANFILES = debugger_gdbmi.zip

debugger_gdbmi.zip: $(debugger_gdbmi_RESOURCES)
	PWD=`pwd` cd $(srcdir)/resource/ && zip $(PWD)/debugger_gdbmi.zip manifest.xml *.xrc > /dev/null
