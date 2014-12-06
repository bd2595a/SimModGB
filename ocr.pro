HEADERS += ocr.h
HEADERS += neural.h
SOURCES += ocr.cpp
SOURCES += neural.cpp

target.path = ocr
sources.files= $$SOURCES $$HEADERS ocr.pro
sources.path = .
INSTALLS += target sources
