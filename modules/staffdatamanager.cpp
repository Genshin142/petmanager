#include "staffdatamanager.h"
#include <QDateTime>

StaffDataManager* StaffDataManager::m_instance = nullptr;

StaffDataManager* StaffDataManager::instance()
{
    if (!m_instance) {
        m_instance = new StaffDataManager();
    }
    return m_instance;
}

StaffDataManager::StaffDataManager(QObject *parent) : QObject(parent)
{
    initMockData();
}

void StaffDataManager::initMockData()
{
    auto addDemo = [&](const QString &id, const QString &name, const QString &role, const QString &status, 
                       const QString &gender, int age, const QString &phone, const QString &email, const QString &idCard,
                       int baseSalary) {
        EmployeeInfo info;
        info.id = id; info.name = name; info.role = role; info.status = status;
        info.gender = gender; info.age = age; info.phone = phone; info.email = email;
        info.idCard = idCard; info.baseSalary = baseSalary;
        info.joinDate = "2023-01-01";
        info.department = (role == "店长") ? "总店管理层" : "一线服务部";
        m_staff[id] = info;
    };

    addDemo("E001", "李四", "高级美容师", "在岗", "男", 28, "13800138000", "lisi@pet.com", "440106199601011234", 3500);
    addDemo("E002", "王五", "店员", "请假", "女", 24, "13911223344", "wangwu@pet.com", "440106200005204321", 3000);
    addDemo("E003", "张三", "实习生", "在岗", "男", 21, "13755667788", "zhangsan@pet.com", "440106200310105566", 1200);
    addDemo("E004", "赵六", "宠物医生", "在岗", "男", 35, "15088996677", "zhaoliu@pet.com", "440106198912128899", 6500);
    addDemo("E005", "孙梅", "店长", "在岗", "女", 32, "13612345678", "sunmei@pet.com", "440106199201011122", 8000);
    addDemo("E006", "周莉", "高级美容师", "请假", "女", 27, "13698765432", "zhouli@pet.com", "440106199505053344", 3500);
    addDemo("E007", "吴刚", "店员", "离岗", "男", 22, "13587654321", "wugang@pet.com", "440106200202025566", 2800);
    addDemo("E008", "陈七", "店员", "离职", "女", 26, "13511223344", "chenqi@pet.com", "440106199808081122", 3000);
}

QList<EmployeeInfo> StaffDataManager::allStaff() const
{
    return m_staff.values();
}

EmployeeInfo StaffDataManager::getStaff(const QString &id) const
{
    return m_staff.value(id);
}

void StaffDataManager::addStaff(const EmployeeInfo &info)
{
    m_staff[info.id] = info;
    emit staffDataChanged();
}

void StaffDataManager::updateStaff(const EmployeeInfo &info)
{
    m_staff[info.id] = info;
    emit staffDataChanged();
}

void StaffDataManager::removeStaff(const QString &id)
{
    m_staff.remove(id);
    emit staffDataChanged();
}

QStringList StaffDataManager::activeStaffNames() const
{
    QStringList names;
    for (const auto &info : m_staff.values()) {
        if (info.status != "离职") {
            names << info.name;
        }
    }
    return names;
}
