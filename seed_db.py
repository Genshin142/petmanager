import mysql.connector
from mysql.connector import Error

def seed_database():
    connection = None
    try:
        config = {
            'user': 'root',
            'password': '362345943', 
            'host': '127.0.0.1',
            'database': 'petstore'
        }

        print(f"Connecting to MySQL as {config['user']}...")
        connection = mysql.connector.connect(**config)
        
        if connection.is_connected():
            cursor = connection.cursor()

            # 1. 手动检测并补全列 (通用方案)
            print("Checking pets table columns...")
            cursor.execute("DESC pets")
            existing_columns = [col[0] for col in cursor.fetchall()]

            required_columns = {
                'age_months': "INT UNSIGNED AFTER gender",
                'health_status': "VARCHAR(50) AFTER weight",
                'medical_history': "TEXT AFTER health_status",
                'vaccine_status': "TEXT AFTER medical_history",
                'dietary_habit': "TEXT AFTER vaccine_status",
                'join_time': "DATETIME DEFAULT CURRENT_TIMESTAMP AFTER current_status"
            }

            for col, definition in required_columns.items():
                if col not in existing_columns:
                    print(f"Adding missing column: {col}...")
                    cursor.execute(f"ALTER TABLE pets ADD COLUMN {col} {definition}")
            
            # 解决 ENUM 限制问题：将 current_status 改为 VARCHAR
            print("Expanding 'current_status' column flexibility...")
            cursor.execute("ALTER TABLE pets MODIFY COLUMN current_status VARCHAR(50);")
            
            connection.commit()

            print("Cleaning old data...")
            cursor.execute("SET FOREIGN_KEY_CHECKS = 0;")
            cursor.execute("TRUNCATE TABLE pets;")
            cursor.execute("TRUNCATE TABLE members;")
            cursor.execute("TRUNCATE TABLE products;")
            cursor.execute("TRUNCATE TABLE product_inbound;")
            cursor.execute("SET FOREIGN_KEY_CHECKS = 1;")

            print("Inserting members...")
            members_sql = "INSERT INTO members (name, gender, birthday, phone, balance, level_name) VALUES (%s, %s, %s, %s, %s, %s)"
            members_data = [
                ('张三', '男', '1990-01-01', '13800138001', 500.00, '金卡会员'),
                ('李芳', '女', '1995-05-12', '13911112222', 150.00, '普通会员'),
                ('赵六', '男', '1988-11-20', '13366667777', 1200.00, '铂金会员')
            ]
            cursor.executemany(members_sql, members_data)
            connection.commit()

            cursor.execute("SELECT member_id, name FROM members")
            member_map = {name: mid for (mid, name) in cursor.fetchall()}

            print("Inserting pets...")
            pets_sql = """
                INSERT INTO pets (member_id, pet_name, species, breed, gender, age_months, weight, health_status, avatar_path, current_status) 
                VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s)
            """
            pets_data = [
                (member_map['张三'], '布丁', '狗', '金毛犬', '公', 24, 25.5, '健康', 'foster_outdoor_new.png', '待接走'),
                (member_map['李芳'], '芝麻', '猫', '英短蓝猫', '母', 12, 4.2, '良好', 'foster_sleep_new.png', '寄养中'),
                (member_map['李芳'], '豆豆', '狗', '柴犬', '公', 36, 11.0, '健康', 'foster_bath_new.png', '在家'),
                (member_map['赵六'], '小雪', '狗', '萨摩耶', '母', 18, 18.5, '健康', 'load_img.jpg', '洗护中')
            ]
            cursor.executemany(pets_sql, pets_data)
            connection.commit()

            print("Inserting products...")
            products_sql = """
                INSERT INTO products (barcode, name, brand, origin, category, spec, unit, sale_price, cost_price, stock_current, stock_min, production_date, shelf_life_days, supplier, tags, is_active) 
                VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s)
            """
            products_data = [
                ('6901234567890', '皇家全价猫粮 2kg', '皇家', '法国', '宠物主食', '2kg/袋', '袋', 168.00, 110.00, 50, 10, '2026-01-01', 365, '皇家宠物食品', '高肉含量,美毛', 1),
                ('6901234567891', '比瑞吉幼犬粮 1.5kg', '比瑞吉', '国产', '宠物主食', '1.5kg/袋', '袋', 125.00, 85.00, 5, 10, '2026-02-15', 365, '中宠实业', '天然无谷', 1),
                ('6901234567892', '麦富迪鸡肉干 100g', '麦富迪', '国产', '宠物零食', '100g/包', '包', 25.00, 12.00, 100, 20, '2026-03-20', 540, '麦富迪贸易', '磨牙补钙', 1),
                ('6901234567893', '小佩智能饮水机', '小佩', '国产', '宠物用品', '智能款', '台', 199.00, 140.00, 3, 5, '2026-01-10', 1095, '小佩科技', '智能互动', 1),
                ('6901234567894', '伊丽莎白圈 L码', '博乐', '国产', '医疗护理', 'L码', '个', 35.00, 15.00, 15, 5, '2025-12-01', 1825, '博乐医疗', '术后护理', 1)
            ]
            cursor.executemany(products_sql, products_data)

            # 2. 插入模拟入库记录
            print("Seeding product_inbound records...")
            cursor.execute("TRUNCATE TABLE product_inbound")
            
            inbound_data = [
                ('IN20260509001', '690987654321', '小鲜肉混合猫砂 6L', '6L/袋', '洗护', '中宠贸易', 100, 15.50, '2026-04-01', 730, '010-66668888', '仓库管理员', 1),
                ('IN20260509002', '999888777666', '渴望六种鱼猫粮 1.8kg', '1.8kg/袋', '主食', '渴望中国代理', 30, 280.00, '2026-05-15', 540, '021-12345678', '张工', 0),
                ('IN20260509003', '690123456789', '皇家全价猫粮 2kg', '2kg/袋', '主食', '皇家宠物食品', 25, 110.00, '2026-03-20', 365, '400-888-1234', '小李', 1)
            ]

            insert_query = """
                INSERT INTO product_inbound 
                (inbound_no, barcode, product_name, spec, category, supplier, quantity, cost_price, production_date, shelf_life_days, supplier_phone, operator_name, is_shelved) 
                VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s)
            """
            cursor.executemany(insert_query, inbound_data)

            connection.commit()
            print(">>> Success! Mock data seeded.")

    except Error as e:
        print(f"Error: {e}")
    finally:
        if connection and connection.is_connected():
            cursor.close()
            connection.close()

if __name__ == "__main__":
    seed_database()
