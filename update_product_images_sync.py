import mysql.connector
import base64
import os
import json

def image_to_base64(file_path):
    if not os.path.exists(file_path):
        print(f"Warning: File not found {file_path}")
        return ""
    with open(file_path, "rb") as image_file:
        return base64.b64encode(image_file.read()).decode('utf-8')

def update_all_product_tables():
    db = mysql.connector.connect(
        host="localhost",
        user="root",
        password="362345943",
        database="petstore"
    )
    cursor = db.cursor()

    base_dir = r"C:\Users\任坤\.gemini\antigravity\brain\df9ce58e-49d6-4062-ba25-326214d12bdd"
    
    mapping = {
        "064992201122": [
            os.path.join(base_dir, "orijen_cat_food_front_1778424708468.png"),
            os.path.join(base_dir, "orijen_cat_food_back_side_1778424728317.png")
        ],
        "3182550702331": [os.path.join(base_dir, "rc_k36_views_1778424749029.png")],
        "6949313200125": [os.path.join(base_dir, "bile_dog_food_views_1778424775446.png")],
        "064992202112": [os.path.join(base_dir, "acana_dog_food_views_1778424795588.png")],
        "4901133716577": [os.path.join(base_dir, "ciao_churu_views_1778424816891.png")],
        "076344091012": [os.path.join(base_dir, "wellness_core_views_1778424835095.png")],
        "6901234567890": [os.path.join(base_dir, "rc_adult_dog_views_1778424857133.png")]
    }

    tables = ["products", "product_inbound"]

    for table in tables:
        print(f"Updating table: {table}...")
        for barcode, files in mapping.items():
            base64_list = []
            for f in files:
                b64 = image_to_base64(f)
                if b64:
                    base64_list.append(b64)
            
            if base64_list:
                img_data_val = json.dumps(base64_list)
                # Use barcode as the key for both tables
                sql = f"UPDATE {table} SET img_data = %s WHERE barcode = %s"
                cursor.execute(sql, (img_data_val, barcode))
                print(f"  [ {table} ] Updated {barcode}")

    db.commit()
    db.close()
    print("Database sync complete.")

if __name__ == "__main__":
    update_all_product_tables()
