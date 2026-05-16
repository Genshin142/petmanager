QT       += core gui network widgets svg charts printsupport sql

CONFIG += c++17

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    loginwindow.cpp \
    mainwindow.cpp \
    loadingwindow.cpp \
    utils/networkmanager.cpp \
    modules/membermodule.cpp \
    modules/rolemodule.cpp \
    modules/petmodule.cpp \
    modules/fostermodule.cpp \
    modules/appointmentmodule.cpp \
    modules/checkoutmodule.cpp \
    modules/financemodule.cpp \
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
    modules/petdatamanager.cpp \
    modules/logisticsmanager.cpp \
    modules/logisticsdetaildrawer.cpp \
    modules/logisticsmodule.cpp \
    modules/productdatamanager.cpp \
    modules/inboundmodule.cpp \
    modules/inboundregistrationdialog.cpp \
    modules/selectinbounddialog.cpp \
    modules/employeedetaildrawer.cpp \
    modules/memberdetaildrawer.cpp \
    modules/appointmentitemdelegate.cpp \
    modules/appointmentdetaildrawer.cpp \
    modules/globallightbox.cpp \
    modules/staffdatamanager.cpp \
    modules/staffselectiondialog.cpp \
    modules/pricemanager.cpp \
    modules/orderdetaildrawer.cpp \
    modules/servicedatamanager.cpp \
    modules/memberdatamanager.cpp \
    modules/servicemanagementmodule.cpp \
    modules/quickorderdialog.cpp \
    modules/servicedialog.cpp \
    modules/boardingdatamanager.cpp \
    modules/rechargedialog.cpp \
    modules/salarydatamanager.cpp \
    modules/personalmodule.cpp \
    modules/passwordchangedialog.cpp \
    modules/backupmanager.cpp \
    modules/backupprogressdialog.cpp \
    modules/systemsettingsdialog.cpp \
    modules/logdatamanager.cpp \
    modules/operationlogdialog.cpp \
    modules/scheduledatamanager.cpp \
    modules/schedulemodule.cpp \
    modules/scheduletemplatedialog.cpp \
    utils/logger.cpp

HEADERS += \
    utils/logger.h \
    utils/networkmanager.h \
    loginwindow.h \
    mainwindow.h \
    loadingwindow.h \
    modules/membermodule.h \
    modules/rolemodule.h \
    modules/petmodule.h \
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
    modules/petdatamanager.h \
    modules/fosterganttmodel.h \
    modules/fosterganttdelegate.h \
    modules/logisticsmanager.h \
    modules/logisticsdetaildrawer.h \
    modules/logisticsmodule.h \
    modules/productdatamanager.h \
    modules/inboundmodule.h \
    modules/inboundregistrationdialog.h \
    modules/selectinbounddialog.h \
    modules/employeedetaildrawer.h \
    modules/memberdetaildrawer.h \
    modules/appointmentitemdelegate.h \
    modules/appointmentdetaildrawer.h \
    modules/globallightbox.h \
    modules/staffdatamanager.h \
    modules/staffselectiondialog.h \
    modules/pricemanager.h \
    modules/orderdetaildrawer.h \
    modules/servicedatamanager.h \
    modules/memberdatamanager.h \
    modules/servicemanagementmodule.h \
    modules/quickorderdialog.h \
    modules/servicedialog.h \
    modules/boardingdatamanager.h \
    modules/rechargedialog.h \
    modules/salarydatamanager.h \
    modules/personalmodule.h \
    modules/passwordchangedialog.h \
    modules/backupmanager.h \
    modules/backupprogressdialog.h \
    modules/systemsettingsdialog.h \
    modules/logdatamanager.h \
    modules/operationlogdialog.h \
    modules/scheduledatamanager.h \
    modules/schedulemodule.h \
    modules/scheduletemplatedialog.h \
    modules/financemodule.h

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

win32: LIBS += -ldbghelp
