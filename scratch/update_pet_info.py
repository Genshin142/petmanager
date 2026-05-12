import mysql.connector

def update_pet_data():
    try:
        conn = mysql.connector.connect(
            host="127.0.0.1",
            user="root",
            password="362345943",
            database="petstore"
        )
        cursor = conn.cursor()
        
        # 定义真实感数据
        updates = [
            ("P00001", "3岁", "已接种(狂犬/五联)"),
            ("P00002", "2岁", "已接种(猫三联)"),
            ("P00003", "1岁6个月", "已接种(狂犬/五联)"),
            ("P00004", "4岁", "已接种(猫三联)"),
            ("P00005", "2岁", "已接种(狂犬/五联)"),
            ("P00006", "1岁", "已接种(猫三联)"),
            ("P00007", "5岁", "已接种(狂犬/五联)"),
            ("P00008", "2岁", "已接种(狂犬/五联)"),
            ("P00009", "1岁2个月", "已接种(猫三联)"),
            ("P00010", "8个月", "未接种")
        ]
        
        for pet_id, age, vaccine in updates:
            query = "UPDATE pets SET age = %s, vaccine = %s WHERE pet_id = %s"
            cursor.execute(query, (age, vaccine, pet_id))
            print(f"Updated pet {pet_id}: Age={age}, Vaccine={vaccine}")
            
        conn.commit()
        print("All pet data updated successfully.")
        
    except Exception as e:
        print(f"Error: {e}")
    finally:
        if 'conn' in locals() and conn.is_connected():
            conn.close()

if __name__ == "__main__":
    update_pet_data()
