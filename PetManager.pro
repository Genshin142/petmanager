QT       += core gui network widgets svg

CONFIG += c++17

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    loginwindow.cpp \
    mainwindow.cpp \
    modules/membermodule.cpp \
    modules/rolemodule.cpp \
    modules/petmodule.cpp \
    modules/productmodule.cpp \
    modules/fostermodule.cpp \
    modules/ordermodule.cpp \
    modules/statsmodule.cpp \
    modules/addmemberdialog.cpp \
    modules/custommessagedialog.cpp \
    modules/selectpetdialog.cpp \
    modules/addpetdialog.cpp

HEADERS += \
    loginwindow.h \
    mainwindow.h \
    modules/membermodule.h \
    modules/rolemodule.h \
    modules/petmodule.h \
    modules/productmodule.h \
    modules/fostermodule.h \
    modules/ordermodule.h \
    modules/statsmodule.h \
    modules/addmemberdialog.h \
    modules/custommessagedialog.h \
    modules/selectpetdialog.h \
    modules/addpetdialog.h

FORMS += \
    loginwindow.ui \
    mainwindow.ui \
    modules/addmemberdialog.ui \
    modules/addpetdialog.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    src.qrc
