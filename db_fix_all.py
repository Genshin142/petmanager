import mysql.connector
from mysql.connector import Error

def fix_database():
    connection = None
    try:
        config = {
            'user': 'root',
            'password': '362345943', 
            'host': '127.0.0.1',
            'database': 'petstore'
        }

        connection = mysql.connector.connect(**config)
        if connection.is_connected():
            cursor = connection.cursor()

            # --- 核心修复：Products 表 ---
            print("Optimizing products table...")
            cursor.execute("DESC products")
            prod_cols = [col[0] for col in cursor.fetchall()]

            # 1. 处理 stock -> stock_current 迁移
            if 'stock' in prod_cols and 'stock_current' not in prod_cols:
                print("  + Renaming 'stock' to 'stock_current'...")
                cursor.execute("ALTER TABLE products CHANGE COLUMN stock stock_current INT DEFAULT 0")
            
            # 2. 补全其他缺失字段
            prod_fixes = {
                'brand': "VARCHAR(50) DEFAULT '' AFTER name",
                'origin': "VARCHAR(50) DEFAULT '' AFTER brand",
                'spec': "VARCHAR(50) DEFAULT '' AFTER category",
                'unit': "VARCHAR(20) DEFAULT '件' AFTER spec",
                'sale_price': "DECIMAL(10,2) DEFAULT 0.00",
                'cost_price': "DECIMAL(10,2) DEFAULT 0.00 AFTER sale_price",
                'stock_current': "INT DEFAULT 0 AFTER sale_price",
                'stock_min': "INT DEFAULT 5 AFTER stock_current",
                'production_date': "VARCHAR(20) DEFAULT ''",
                'shelf_life_days': "INT DEFAULT 365",
                'supplier': "VARCHAR(100) DEFAULT ''",
                'supplier_phone': "VARCHAR(20) DEFAULT ''",
                'description': "TEXT",
                'ingredients': "TEXT",
                'storage_req': "VARCHAR(255) DEFAULT ''",
                'tags': "VARCHAR(255) DEFAULT ''",
                'is_active': "TINYINT(1) DEFAULT 1",
                'is_deleted': "TINYINT(1) DEFAULT 0"
            }
            
            for col, df in prod_fixes.items():
                # 重新获取当前列名，防止改名后状态不一致
                cursor.execute("DESC products")
                current_cols = [c[0] for c in cursor.fetchall()]
                if col not in current_cols:
                    print(f"  + Adding column: {col}...")
                    cursor.execute(f"ALTER TABLE products ADD COLUMN {col} {df}")

            # --- 核心修复：product_inbound 表 ---
            print("Checking product_inbound table...")
            cursor.execute("""
                CREATE TABLE IF NOT EXISTS product_inbound (
                    id INT AUTO_INCREMENT PRIMARY KEY,
                    inbound_no VARCHAR(50) NOT NULL,
                    barcode VARCHAR(50) NOT NULL,
                    product_name VARCHAR(100) NOT NULL,
                    spec VARCHAR(50),
                    category VARCHAR(50),
                    supplier VARCHAR(100),
                    quantity INT DEFAULT 0,
                    cost_price DECIMAL(10,2) DEFAULT 0.00,
                    production_date DATE,
                    shelf_life_days INT DEFAULT 365,
                    supplier_phone VARCHAR(20),
                    operator_name VARCHAR(50),
                    is_shelved TINYINT(1) DEFAULT 0,
                    created_at DATETIME DEFAULT CURRENT_TIMESTAMP
                )
            """)
            
            # 补全可能缺失的字段 (ALTER TABLE)
            cursor.execute("DESC product_inbound")
            inbound_cols = [c[0] for c in cursor.fetchall()]
            inbound_fixes = {
                'supplier_phone': "VARCHAR(20) AFTER shelf_life_days",
                'shelf_life_days': "INT DEFAULT 365 AFTER production_date",
                'operator_name': "VARCHAR(50) AFTER supplier_phone"
            }
            for col, df in inbound_fixes.items():
                if col not in inbound_cols:
                    print(f"  + Adding missing column to inbound: {col}...")
                    cursor.execute(f"ALTER TABLE product_inbound ADD COLUMN {col} {df}")

            # --- 2. 检查排班表 (sys_schedules) ---
            cursor.execute("SHOW INDEX FROM sys_schedules WHERE Key_name = 'idx_emp_date'")
            if not cursor.fetchone():
                cursor.execute("ALTER TABLE sys_schedules ADD UNIQUE KEY idx_emp_date (emp_id, work_date)")
                print(" [OK] sys_schedules: 添加复合唯一索引 idx_emp_date")

            # --- 核心修复：Members 表 ---
            print("Optimizing members table...")
            cursor.execute("DESC members")
            mem_cols = [c[0] for c in cursor.fetchall()]
            mem_fixes = {
                "gender": "ENUM('男', '女', '未知') DEFAULT '未知' AFTER name",
                "birthday": "DATE AFTER gender",
                "consume_amt": "DECIMAL(12,2) DEFAULT 0.00 AFTER balance"
            }
            for col, df in mem_fixes.items():
                if col not in mem_cols:
                    print(f"  + Adding column: {col}...")
                    cursor.execute(f"ALTER TABLE members ADD COLUMN {col} {df}")

            connection.commit()
            print("\n>>> Success! Database schema is now perfect.")

    except Error as e:
        print(f"\n[ERROR] {e}")
    finally:
        if connection and connection.is_connected():
            cursor.close()
            connection.close()

if __name__ == "__main__":
    fix_database()
