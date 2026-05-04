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
    
    # Remove any line that contains item(..., -1) or similar broken index access
    content = re.sub(r'\s*\(\w+Table->item\([^,]+,\s*-1\)->text\(\)\.contains\([^)]+\)\s*\|\|', '', content)
    
    with open(f, 'w', encoding='utf-8') as file:
        file.write(content)
print("Cleanup 2 completed")
