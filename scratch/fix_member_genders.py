import mysql.connector

def fix_members_complete():
    try:
        db = mysql.connector.connect(
            host='127.0.0.1',
            user='root',
            password='362345943',
            database='petstore',
            charset='utf8mb4'
        )
        cursor = db.cursor()
        
        # 补全性别和生日
        updates = [
            ('男', '1990-05-15', '张三'),
            ('女', '1995-12-20', '李芳'),
            ('男', '1988-03-10', '王强'),
            ('男', '1992-08-30', '赵六')
        ]
        
        for gender, bday, name in updates:
            print(f"Updating {name}: Gender={gender}, Birthday={bday}")
            cursor.execute('UPDATE members SET gender = %s, birthday = %s WHERE name = %s', (gender, bday, name))
        
        db.commit()
        print("Successfully updated member data.")
        
    except Exception as e:
        print(f"Error: {e}")
    finally:
        if 'db' in locals() and db.is_connected():
            db.close()

if __name__ == "__main__":
    fix_members_complete()
