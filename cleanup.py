import re

files = [
    "e:/QT/work/PetManager/modules/membermodule.cpp",
    "e:/QT/work/PetManager/modules/productmodule.cpp",
    "e:/QT/work/PetManager/modules/servicemanagementmodule.cpp",
    "e:/QT/work/PetManager/modules/salarymodule.cpp",
    "e:/QT/work/PetManager/modules/performancemodule.cpp"
]

for f in files:
    with open(f, 'r', encoding='utf-8') as file:
        content = file.read()
    
    # Remove any line that has (r, -1, chkWidget) or (i, -1) or similar broken widgets
    content = re.sub(r'.*?\w+Table->(?:setCellWidget|cellWidget)\([^,]+,\s*-1.*?\n', '', content)
    content = re.sub(r'\s*QWidget \*chkWidget = new QWidget\(\);\s*QHBoxLayout \*chkLayout = new QHBoxLayout\(chkWidget\);\s*chkLayout->setContentsMargins\(0, 0, 0, 0\);\s*QCheckBox \*chkBox = new QCheckBox\(\);\s*chkLayout->addWidget\(chkBox, 0, Qt::AlignCenter\);', '', content)
    
    with open(f, 'w', encoding='utf-8') as file:
        file.write(content)
print("Cleanup completed")
