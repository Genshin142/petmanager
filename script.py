import re

files = [
    "e:/QT/work/PetManager/modules/membermodule.cpp",
    "e:/QT/work/PetManager/modules/productmodule.cpp",
    "e:/QT/work/PetManager/modules/servicemanagementmodule.cpp",
    "e:/QT/work/PetManager/modules/salarymodule.cpp",
    "e:/QT/work/PetManager/modules/performancemodule.cpp",
    "e:/QT/work/PetManager/modules/membermodule.h",
    "e:/QT/work/PetManager/modules/productmodule.h"
]

for f in files:
    try:
        with open(f, 'r', encoding='utf-8') as file:
            content = file.read()
    except FileNotFoundError:
        continue
        
    # Remove batch delete button
    content = re.sub(r'\s*QPushButton\s*\*\s*\w+Btn\s*=\s*new\s*QPushButton\("批量删除"\);\s*\w+Btn->setCursor.*?\s*connect\(\w+Btn,\s*&QPushButton::clicked,\s*this,\s*&\w+::\w+\);\s*', '', content, flags=re.DOTALL)
    
    # Also remove any operationLayout->addWidget(batchDeleteBtn);
    content = re.sub(r'\s*operationLayout->addWidget\(batchDeleteBtn\);\s*operationLayout->addSpacing\(\d+\);', '', content)
    
    with open(f, 'w', encoding='utf-8') as file:
        file.write(content)
print("Done")
