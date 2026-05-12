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

def update_product_images():
    db = mysql.connector.connect(
        host="localhost",
        user="root",
        password="362345943",
        database="petstore"
    )
    cursor = db.cursor()

    # Mapping of barcodes to image file paths
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

    for barcode, files in mapping.items():
        base64_list = []
        for f in files:
            b64 = image_to_base64(f)
            if b64:
                # Add data URI prefix if needed, but the client code just splits or parses
                # Based on previous logs, the client expects raw base64 or a JSON array of raw base64.
                # However, the client's OrderDetailDrawer uses QPixmap::loadFromData which works with raw bytes.
                # So we store raw base64.
                base64_list.append(b64)
        
        if base64_list:
            # Store as JSON array string as the client code at line 116 checks for '['
            img_data_val = json.dumps(base64_list)
            sql = "UPDATE products SET img_data = %s WHERE barcode = %s"
            cursor.execute(sql, (img_data_val, barcode))
            print(f"Updated img_data for product {barcode} with {len(base64_list)} images (array format)")

    db.commit()
    db.close()
    print("All products updated successfully.")

if __name__ == "__main__":
    update_product_images()
