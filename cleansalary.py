import re

files = [
    "e:/QT/work/PetManager/modules/salarymodule.cpp"
]

for f in files:
    with open(f, 'r', encoding='utf-8') as file:
        content = file.read()
    
    # Remove batch pay button
    content = re.sub(r'\s*QPushButton\s*\*\s*batchPayBtn.*?connect\(batchPayBtn.*?\);\s*', '', content, flags=re.DOTALL)
    content = re.sub(r'\s*toolLayout->addWidget\(batchPayBtn\);\s*', '', content)
    
    with open(f, 'w', encoding='utf-8') as file:
        file.write(content)
print("Salary cleaned")
