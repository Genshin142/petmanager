import mysql.connector
from datetime import datetime

def seed_data():
    try:
        # 1. 连接数据库
        conn = mysql.connector.connect(
            host="localhost",
            user="root",
            password="362345943",
            database="petstore",
            charset='utf8mb4'
        )
        cursor = conn.cursor()

        # 2. 彻底清理入库单表
        print("Refreshing product_inbound table...")
        cursor.execute("DELETE FROM product_inbound") 

        # 3. 准备格式统一的商品数据
        products = [
            ('IN20260510001', '064992201122', '渴望(Orijen)六种鱼全猫粮', '1.8kg/袋', '猫粮/主食', '广州派特贸易有限公司', 20, 285.00, '2026-01-15', 540, '管理员'),
            ('IN20260510002', '3182550702331', '皇家(Royal Canin)K36幼猫猫粮', '2kg/袋', '猫粮/主食', '皇家宠物食品(上海)有限公司', 50, 115.00, '2026-03-10', 365, '管理员'),
            ('IN20260510003', '6949313200125', '比乐(Bile)原味系列成犬粮', '10kg/袋', '狗粮/主食', '上海福贝宠物用品有限公司', 15, 185.00, '2026-02-20', 540, '管理员'),
            ('IN20260510004', '064992202112', '渴望(Acana)农场盛宴全犬犬粮', '11.4kg/袋', '狗粮/主食', '广州派特贸易有限公司', 10, 480.00, '2026-01-05', 540, '管理员'),
            ('IN20260510005', '4901133716577', '伊纳宝(CIAO)啾噜系列肉泥猫条', '56g/盒 (14g*4条)', '猫零食', '伊纳宝宠物食品(青岛)有限公司', 100, 8.50, '2026-04-01', 730, '管理员'),
            ('IN20260510006', '076344091012', '魏斯宝(Wellness)CORE无谷猫粮', '5.4kg/袋', '猫粮/主食', '北京合众众诚商贸有限公司', 12, 310.00, '2026-02-15', 540, '管理员'),
            ('IN20260510007', '6901234567890', '皇家(Royal Canin)全犬期犬粮', '10kg/袋', '狗粮/主食', '皇家宠物食品(上海)有限公司', 8, 265.00, '2026-02-01', 365, '管理员'),
            ('IN20260510008', '4901133718908', '伊纳宝(CIAO)烧鲣鱼猫零食', '15g/条', '猫零食', '伊纳宝宠物食品(青岛)有限公司', 50, 4.20, '2026-03-20', 730, '管理员'),
            ('IN20260510009', '6971161234567', '网易严选 全价无谷猫粮', '7.2kg/袋', '猫粮/主食', '网易严选有限公司', 30, 215.00, '2026-03-01', 540, '管理员'),
        ]

        # 4. 执行批量插入
        sql = """INSERT INTO product_inbound 
                 (inbound_no, barcode, product_name, spec, category, supplier, quantity, cost_price, production_date, shelf_life_days, operator_name, is_shelved, created_at)
                 VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, 0, NOW())"""
        
        cursor.executemany(sql, products)

        # 5. 提交并关闭
        conn.commit()
        print("Database specifications unified successfully!")

    except mysql.connector.Error as err:
        print(f"Error: {err}")
    finally:
        if 'conn' in locals() and conn.is_connected():
            cursor.close()
            conn.close()

if __name__ == "__main__":
    seed_data()
