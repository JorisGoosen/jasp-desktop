# This is part of https://github.com/jasp-stats/INTERNAL-jasp/issues/996 and works, but requires me to install V8 because of stupid dependency resolution based on CRAN
# So ive turned it off for now, but if you'd like to use it you can!
R_MODULES_INSTALL_DEPENDENCIES = false

isEmpty(MODULE_NAME) {
    message(You must specify MODULE_NAME to use InstallModule.pri!)
} else {
    isEmpty(MODULE_DIR) MODULE_DIR=$$PWD

    JASP_LIBRARY_DIR = $${JASP_BUILDROOT_DIR}/Modules/$${MODULE_NAME}
	include(../R_INSTALL_CMDS.pri)
	#First we remove the installed module to make sure it gets properly update. We leave the library dir to avoid having to install the dependencies all the time.
	#This will just have to get cleaned up by "clean"
	Install$${MODULE_NAME}.commands        =  rm -rf $$JASP_LIBRARY_DIR/$${MODULE_NAME} && mkdir $$JASP_LIBRARY_DIR;

    #Then, if we so desire, we install all dependencies (that are missing anyhow)
	$$R_MODULES_INSTALL_DEPENDENCIES: Install$${MODULE_NAME}.commands       +=  $${INSTALL_R_PKG_DEPS_CMD_PREFIX}$${MODULE_DIR}/$${MODULE_NAME}$${INSTALL_R_PKG_DEPS_CMD_POSTFIX};

    #Install the actual module package
	Install$${MODULE_NAME}.commands       +=  $${INSTALL_R_PKG_CMD_PREFIX}$${MODULE_DIR}/$${MODULE_NAME}$${INSTALL_R_PKG_CMD_POSTFIX};

    #And lastly we do some postprocessing (on mac this includes fixing any and all necessary paths in dylib's and so's)
	win32: Install$${MODULE_NAME}.commands       +=  $${JASP_BUILDROOT_DIR}/JASPEngine.exe $$JASP_LIBRARY_DIR
	unix:  Install$${MODULE_NAME}.commands       +=  $${JASP_BUILDROOT_DIR}/JASPEngine $$JASP_LIBRARY_DIR

    QMAKE_EXTRA_TARGETS += Install$${MODULE_NAME}
	POST_TARGETDEPS     += Install$${MODULE_NAME}

    #See this: https://www.qtcentre.org/threads/9287-How-do-I-get-QMAKE_CLEAN-to-delete-a-directory
	#The windows one still ought to be tested though
	unix:  QMAKE_DEL_FILE = rm -rf
	win32: QMAKE_DEL_FILE = rmdir

    QMAKE_CLEAN			+= $$JASP_LIBRARY_DIR/$${MODULE_NAME}/* $$JASP_LIBRARY_DIR/*
	#QMAKE_DISTCLEAN	+= $$JASP_LIBRARY_DIR/*/*/* $$JASP_LIBRARY_DIR/*/* $$JASP_LIBRARY_DIR/* $$JASP_LIBRARY_DIR
}
