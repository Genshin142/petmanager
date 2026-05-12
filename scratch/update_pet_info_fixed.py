import mysql.connector

def update_pet_info_real():
    try:
        conn = mysql.connector.connect(
            host="127.0.0.1",
            user="root",
            password="362345943",
            database="petstore"
        )
        cursor = conn.cursor()
        
        # 定义真实感数据 (使用正确列名: age_months, vaccine_status)
        # ID 对应数据库中的 pet_id (1-10)
        updates = [
            (1, 36, "已接种(狂犬/五联)"),
            (2, 24, "已接种(猫三联)"),
            (3, 18, "已接种(狂犬/五联)"),
            (4, 48, "已接种(猫三联)"),
            (5, 24, "已接种(狂犬/五联)"),
            (6, 12, "已接种(猫三联)"),
            (7, 60, "已接种(狂犬/五联)"),
            (8, 24, "已接种(狂犬/五联)"),
            (9, 14, "已接种(猫三联)"),
            (10, 8, "未接种")
        ]
        
        for pet_id, age_months, vaccine_status in updates:
            # 更新宠物基本表
            query = "UPDATE pets SET age_months = %s, vaccine_status = %s WHERE pet_id = %s"
            cursor.execute(query, (age_months, vaccine_status, pet_id))
            
            # 如果已接种，添加一条活动日志
            if "已接种" in vaccine_status:
                log_query = """
                    INSERT INTO pet_activity_logs (pet_id, log_time, log_type, remark)
                    VALUES (%s, NOW(), '检查', %s)
                """
                cursor.execute(log_query, (pet_id, f"完成年度疫苗接种: {vaccine_status}"))
                
            print(f"Updated pet {pet_id}: AgeMonths={age_months}, VaccineStatus={vaccine_status}")
            
        conn.commit()
        print("Pet info and logs updated successfully.")
        
    except Exception as e:
        print(f"Error: {e}")
    finally:
        if 'conn' in locals() and conn.is_connected():
            conn.close()

if __name__ == "__main__":
    update_pet_info_real()
