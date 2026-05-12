import mysql.connector
from mysql.connector import Error

def unify_categories():
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

            print("Unifying categories in database (only keeping part after '/')...")
            
            # 定义更新逻辑：如果包含 '/'，则截取最后一部分；否则保持原样（去除空格）
            # 注意：MySQL 的 SUBSTRING_INDEX(str, '/', -1) 可以方便地获取最后一部分
            
            # 1. 更新 product_inbound 表
            print("  + Processing product_inbound...")
            cursor.execute("""
                UPDATE product_inbound 
                SET category = TRIM(SUBSTRING_INDEX(category, '/', -1)) 
                WHERE category LIKE '%/%'
            """)
            rows_inbound = cursor.rowcount

            # 2. 更新 products 表
            print("  + Processing products...")
            cursor.execute("""
                UPDATE products 
                SET category = TRIM(SUBSTRING_INDEX(category, '/', -1)) 
                WHERE category LIKE '%/%'
            """)
            rows_products = cursor.rowcount

            connection.commit()
            print(f">>> Success! {rows_inbound} inbound records and {rows_products} product records unified.")

    except Error as e:
        print(f"\n[ERROR] {e}")
    finally:
        if connection and connection.is_connected():
            cursor.close()
            connection.close()

if __name__ == "__main__":
    unify_categories()
