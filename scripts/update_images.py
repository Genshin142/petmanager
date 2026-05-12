import mysql.connector
import json
import base64
import os

def image_to_base64(path):
    if not os.path.exists(path):
        return ""
    with open(path, "rb") as image_file:
        return base64.b64encode(image_file.read()).decode('utf-8')

def update_product_images():
    try:
        conn = mysql.connector.connect(
            host="localhost",
            user="root",
            password="362345943",
            database="petstore",
            charset='utf8mb4'
        )
        cursor = conn.cursor()

        base_path = r"C:\Users\任坤\.gemini\antigravity\brain\006b0669-af8e-4892-9b40-51bf330e7f75"
        
        # 1. 渴望 (Orijen) 图片组
        orijen_imgs = [
            os.path.join(base_path, "orijen_cat_front_1778407858057.png"),
            os.path.join(base_path, "orijen_cat_back_1778407873244.png"),
            os.path.join(base_path, "orijen_cat_side_1778407887552.png"),
            os.path.join(base_path, "orijen_cat_ingredients_1778407907355.png")
        ]
        orijen_json = json.dumps([image_to_base64(p) for p in orijen_imgs])
        cursor.execute("UPDATE product_inbound SET img_data = %s WHERE barcode = %s", (orijen_json, "064992201122"))

        # 2. 皇家 (Royal Canin) 图片组
        royal_imgs = [
            os.path.join(base_path, "royal_canin_front_1778407921833.png"),
            os.path.join(base_path, "royal_canin_back_1778407941661.png"),
            os.path.join(base_path, "royal_canin_side_1778407976748.png"),
            os.path.join(base_path, "royal_canin_ingredients_1778407957594.png")
        ]
        royal_json = json.dumps([image_to_base64(p) for p in royal_imgs])
        cursor.execute("UPDATE product_inbound SET img_data = %s WHERE barcode = %s", (royal_json, "3182550702331"))

        conn.commit()
        print("Images updated successfully for key products!")

    except mysql.connector.Error as err:
        print(f"Error: {err}")
    finally:
        if 'conn' in locals() and conn.is_connected():
            cursor.close()
            conn.close()

if __name__ == "__main__":
    update_product_images()
