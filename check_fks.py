import mysql.connector

def check_fks():
    try:
        db = mysql.connector.connect(
            host="localhost",
            user="root",
            password="362345943",
            database="petstore"
        )
        cursor = db.cursor()
        
        query = """
        SELECT COLUMN_NAME, REFERENCED_TABLE_NAME, REFERENCED_COLUMN_NAME 
        FROM INFORMATION_SCHEMA.KEY_COLUMN_USAGE 
        WHERE TABLE_NAME = 'orders' AND REFERENCED_TABLE_NAME IS NOT NULL
        """
        cursor.execute(query)
        fks = cursor.fetchall()
        print("Foreign Keys for 'orders':")
        for fk in fks:
            print(fk)
            
        db.close()
    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    check_fks()
