
_R_HOME = $$(R_HOME)

linux {
	exists(/app/lib/*) {
		contains(QMAKE_HOST.arch, x86_64):{
      		_R_HOME = /app/lib64/R
		} else {
			_R_HOME = /app/lib/R
		}
  } else {
    exists(/usr/lib64/R) {
      isEmpty(_R_HOME): _R_HOME = /usr/lib64/R
		} else {
      isEmpty(_R_HOME): _R_HOME = /usr/lib/R
		}
	}

  #QMAKE_CXXFLAGS += -D\'R_HOME=\"$$_R_HOME\"\'
  INCLUDEPATH += $$_R_HOME/library/include  \
      /usr/include/R/                       \
      /usr/share/R/include                  \
      $$_R_HOME/site-library/Rcpp/include

  R_EXE  = $$_R_HOME/bin/R

  DEFINES += 'R_HOME=\\\"$$_R_HOME\\\"'
}

include(R_INSTALL_CMDS.pri)

INCLUDEPATH += \
    $$_R_HOME/library/Rcpp/include \
    $$_R_HOME/include


message(using R_HOME of $$_R_HOME)
