import mysql.connector
from mysql.connector import Error

def smart_unify():
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

            print("Starting smart category unification (Species/Category)...")
            
            # 1. 处理 product_inbound 表
            print("  + Processing product_inbound...")
            cursor.execute("""
                UPDATE product_inbound 
                SET category = CONCAT(LEFT(category, 1), '/', SUBSTRING(category, 2))
                WHERE (category LIKE '猫%' OR category LIKE '狗%') 
                AND category NOT LIKE '%/%' AND LENGTH(category) > 1
            """)
            cursor.execute("""
                UPDATE product_inbound 
                SET category = CONCAT('猫/', category)
                WHERE category NOT LIKE '%/%' AND (product_name LIKE '%猫%' OR product_name LIKE '%Cat%')
                AND category IN ('主食', '零食', '玩具', '洗护', '日用', '医疗')
            """)
            cursor.execute("""
                UPDATE product_inbound 
                SET category = CONCAT('狗/', category)
                WHERE category NOT LIKE '%/%' AND (product_name LIKE '%狗%' OR product_name LIKE '%犬%' OR product_name LIKE '%Dog%')
                AND category IN ('主食', '零食', '玩具', '洗护', '日用', '医疗')
            """)

            # 2. 处理 products 表
            print("  + Processing products...")
            cursor.execute("""
                UPDATE products 
                SET category = CONCAT(LEFT(category, 1), '/', SUBSTRING(category, 2))
                WHERE (category LIKE '猫%' OR category LIKE '狗%') 
                AND category NOT LIKE '%/%' AND LENGTH(category) > 1
            """)
            cursor.execute("""
                UPDATE products 
                SET category = CONCAT('猫/', category)
                WHERE category NOT LIKE '%/%' AND (name LIKE '%猫%' OR name LIKE '%Cat%')
                AND category IN ('主食', '零食', '玩具', '洗护', '日用', '医疗')
            """)
            cursor.execute("""
                UPDATE products 
                SET category = CONCAT('狗/', category)
                WHERE category NOT LIKE '%/%' AND (name LIKE '%狗%' OR name LIKE '%犬%' OR name LIKE '%Dog%')
                AND category IN ('主食', '零食', '玩具', '洗护', '日用', '医疗')
            """)

            connection.commit()
            print(">>> Success! Categories unified to 'Species/Category' format.")

    except Error as e:
        print(f"\n[ERROR] {e}")
    finally:
        if connection and connection.is_connected():
            cursor.close()
            connection.close()

if __name__ == "__main__":
    smart_unify()
