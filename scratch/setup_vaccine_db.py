import mysql.connector
from datetime import datetime, timedelta

def setup_vaccine_database():
    try:
        conn = mysql.connector.connect(
            host="127.0.0.1",
            user="root",
            password="362345943",
            database="petstore"
        )
        cursor = conn.cursor()
        
        # 1. 创建疫苗明细记录表
        cursor.execute("""
            CREATE TABLE IF NOT EXISTS pet_vaccine_records (
                record_id INT AUTO_INCREMENT PRIMARY KEY,
                pet_id INT NOT NULL,
                vaccine_type VARCHAR(50) NOT NULL,
                vaccine_date DATE NOT NULL,
                expiry_date DATE NOT NULL,
                remarks TEXT,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                FOREIGN KEY (pet_id) REFERENCES pets(pet_id)
            ) COMMENT='宠物疫苗接种明细表'
        """)
        
        # 2. 清理旧数据（如果有）
        cursor.execute("DELETE FROM pet_vaccine_records")
        
        # 3. 为 1-9 号宠物录入详细档案（10号糯米未接种）
        today = datetime.now()
        vaccine_data = [
            # (pet_id, type, days_ago)
            (1, "狂犬疫苗", 180), (1, "犬五联", 190),
            (2, "猫三联(第一针)", 300), (2, "猫三联(第二针)", 270),
            (3, "狂犬疫苗", 100), (3, "犬五联", 110),
            (4, "猫三联", 400), (4, "狂犬疫苗", 410),
            (5, "犬五联", 60),
            (6, "猫三联", 30),
            (7, "狂犬疫苗", 200), (7, "犬五联", 210),
            (8, "狂犬疫苗", 150), (8, "犬五联", 160),
            (9, "猫三联", 45)
        ]
        
        for pet_id, v_type, days_ago in vaccine_data:
            v_date = (today - timedelta(days=days_ago)).strftime('%Y-%m-%d')
            e_date = (today - timedelta(days=days_ago) + timedelta(days=365)).strftime('%Y-%m-%d')
            
            query = """
                INSERT INTO pet_vaccine_records (pet_id, vaccine_type, vaccine_date, expiry_date)
                VALUES (%s, %s, %s, %s)
            """
            cursor.execute(query, (pet_id, v_type, v_date, e_date))
            
        conn.commit()
        print("Vaccine records table created and populated successfully.")
        
    except Exception as e:
        print(f"Error: {e}")
    finally:
        if 'conn' in locals() and conn.is_connected():
            conn.close()

if __name__ == "__main__":
    setup_vaccine_database()
