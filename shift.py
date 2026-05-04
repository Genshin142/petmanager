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
    
    # 1. Header remove "选择"
    content = content.replace('"选择", ', '')
    
    # 2. Decrement setColumnCount
    def dec_col_count(m):
        return f"{m.group(1)}{int(m.group(2)) - 1}{m.group(3)}"
    content = re.sub(r'(\w+Table->setColumnCount\()(\d+)(\))', dec_col_count, content)
    
    # 3. Remove column 0 width/resize and shift others
    def shift_col_setup(m):
        col = int(m.group(2))
        if col == 0:
            return ""
        return f"{m.group(1)}{col - 1}{m.group(3)}"
    
    content = re.sub(r'(\w+Table->setColumnWidth\()(\d+)(,\s*\d+\);)', shift_col_setup, content)
    content = re.sub(r'(\w+Table->horizontalHeader\(\)->setSectionResizeMode\()(\d+)(,\s*QHeaderView::\w+\);)', shift_col_setup, content)
    
    # 4. Hide columns shift
    content = re.sub(r'(\w+Table->setColumnHidden\()(\d+)(,\s*true\);)', shift_col_setup, content)
    
    # 5. Shift items
    def shift_item(m):
        col = int(m.group(2))
        if col == 0:
            # If setting column 0, we can't just delete the line easily here unless we match the whole line.
            # But normally we don't setItem on 0 if it's the checkbox widget. Wait, cellWidget(r, 0)
            pass
        return f"{m.group(1)}{col - 1}{m.group(3)}"
        
    content = re.sub(r'(\w+Table->(?:item|setItem|cellWidget|setCellWidget)\s*\([^,]+,\s*)(\d+)([\),])', shift_item, content)
    
    # 6. Remove checkbox generation block
    chk_pattern = r'\s*// 复选框.*?\w+Table->setCellWidget\([^,]+,\s*0,\s*chkWidget\);'
    content = re.sub(chk_pattern, '', content, flags=re.DOTALL)
    # Alternatively, if there's no comment
    chk_pattern2 = r'\s*QWidget \*chkWidget = new QWidget\(\);.*?chkLayout->addWidget\(chkBox, 0, Qt::AlignCenter\);\s*\w+Table->setCellWidget\([^,]+,\s*0,\s*chkWidget\);'
    content = re.sub(chk_pattern2, '', content, flags=re.DOTALL)

    with open(f, 'w', encoding='utf-8') as file:
        file.write(content)
print("Shift completed")
