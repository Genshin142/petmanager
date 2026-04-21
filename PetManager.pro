QT       += core gui network widgets svg charts

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
    modules/appointmentmodule.cpp \
    modules/checkoutmodule.cpp \
    modules/statsmodule.cpp \
    modules/addmemberdialog.cpp \
    modules/custommessagedialog.cpp \
    modules/selectpetdialog.cpp \
    modules/addpetdialog.cpp \
    modules/addappointmentdialog.cpp \
    modules/historyappointmentdialog.cpp \
    modules/performancemodule.cpp \
    modules/addemployeedialog.cpp \
    modules/salarymodule.cpp \
    modules/petrecorddrawer.cpp \
    modules/custom_calendar_edit.cpp \
    modules/petdatamanager.cpp

HEADERS += \
    loginwindow.h \
    mainwindow.h \
    modules/membermodule.h \
    modules/rolemodule.h \
    modules/petmodule.h \
    modules/productmodule.h \
    modules/fostermodule.h \
    modules/appointmentmodule.h \
    modules/historyappointmentdialog.h \
    modules/checkoutmodule.h \
    modules/addappointmentdialog.h \
    modules/statsmodule.h \
    modules/addmemberdialog.h \
    modules/custommessagedialog.h \
    modules/selectpetdialog.h \
    modules/addpetdialog.h \
    modules/performancemodule.h \
    modules/addemployeedialog.h \
    modules/salarymodule.h \
    modules/petrecorddrawer.h \
    modules/pettimelinemodel.h \
    modules/timelinedelegate.h \
    modules/compactcalendar.h \
    modules/pettimelinewidget.h \
    modules/custom_calendar_edit.h \
    modules/petdatamanager.h

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
