import mysql.connector

def restore_table():
    try:
        conn = mysql.connector.connect(
            host='localhost',
            user='root',
            password='362345943',
            database='petstore'
        )
        cursor = conn.cursor()
        
        sql = """
        CREATE TABLE IF NOT EXISTS boarding_records (
            record_id INT AUTO_INCREMENT PRIMARY KEY,
            room_no VARCHAR(20),
            pet_id INT,
            member_id INT,
            check_in_time DATETIME,
            expected_check_out_time DATETIME,
            actual_check_out_time DATETIME,
            current_weight DECIMAL(5,2),
            check_in_photo VARCHAR(255),
            notes TEXT,
            status ENUM('预约中', '入住中', '已离店', '已取消') DEFAULT '预约中',
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
        """
        
        cursor.execute(sql)
        conn.commit()
        print("Successfully restored boarding_records table")
        
        cursor.close()
        conn.close()
    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    restore_table()
