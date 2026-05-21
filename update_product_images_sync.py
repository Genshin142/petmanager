import mysql.connector
from mysql.connector import Error
import base64
import os
import json
import glob
from datetime import datetime, timedelta

def image_to_base64(file_path):
    if not os.path.exists(file_path):
        print(f"Warning: File not found {file_path}")
        return ""
    with open(file_path, "rb") as image_file:
        return base64.b64encode(image_file.read()).decode('utf-8')

def find_base_dir():
    candidates = [
        r"C:\Users\任坤\.gemini\antigravity\brain\df9ce58e-49d6-4062-ba25-326214d12bdd",
        r"C:\Users\任坤\.gemini\antigravity\brain\ca88cb1d-9d84-4f5d-9ef9-08a103e6e5c6",
        r"C:\Users\任坤\.gemini\antigravity\brain\006b0669-af8e-4892-9b40-51bf330e7f75"
    ]
    for c in candidates:
        if os.path.exists(c):
            if glob.glob(os.path.join(c, "*acana*")) or glob.glob(os.path.join(c, "*orijen*")):
                return c
    # Fallback recursive search
    brain_dir = r"C:\Users\任坤\.gemini\antigravity\brain"
    if os.path.exists(brain_dir):
        for root, dirs, files in os.walk(brain_dir):
            if any("acana" in f.lower() or "orijen" in f.lower() for f in files):
                return root
    return ""

def update_and_shelve_products():
    base_dir = find_base_dir()
    if not base_dir:
        print("Error: Could not locate product images directory in brain folders.")
        return

    print(f"Resolved product images base directory: {base_dir}")

    db = mysql.connector.connect(
        host="localhost",
        user="root",
        password="362345943",
        database="petstore",
        charset='utf8mb4'
    )
    cursor = db.cursor(dictionary=True)

    mapping = {
        "064992201122": ["*orijen_cat_food_front_*", "*orijen_cat_food_back_side_*", "*orijen_cat_front_*", "*orijen_cat_back_*", "*orijen_cat_side_*", "*orijen_cat_ingredients_*"],
        "3182550702331": ["*rc_k36_views_*", "*royal_canin_front_*", "*royal_canin_back_*", "*royal_canin_side_*", "*royal_canin_ingredients_*"],
        "6949313200125": ["*bile_dog_food_views_*"],
        "064992202112": ["*acana_dog_food_views_*", "*acana_front_*", "*acana_side_*", "*acana_back_*", "*acana_ingredients_*"],
        "4901133716577": ["*ciao_churu_views_*"],
        "076344091012": ["*wellness_core_views_*"],
        "6901234567890": ["*rc_adult_dog_views_*", "*royal_canin_dog_food_*"],
        "4901133718908": ["*ciao_churu_views_*"],
        "6971161234567": ["*orijen_cat_food_front_*"]
    }

    # Helper to resolve actual files using globs
    def resolve_files(barcode):
        resolved = []
        seen = set()
        for pat in mapping.get(barcode, []):
            matches = glob.glob(os.path.join(base_dir, pat))
            for m in matches:
                if m not in seen:
                    resolved.append(m)
                    seen.add(m)
        return resolved

    print("\n--- Phase 1: Resolving and updating product images ---")
    cached_images = {}
    for barcode in mapping.keys():
        files = resolve_files(barcode)
        base64_list = []
        for f in files:
            b64 = image_to_base64(f)
            if b64:
                base64_list.append(b64)
        if base64_list:
            img_data_val = json.dumps(base64_list)
            cached_images[barcode] = img_data_val
            print(f"Loaded {len(base64_list)} images for product barcode {barcode}")
        else:
            cached_images[barcode] = ""

    product_details = {
        "064992201122": {"brand": "渴望 (Orijen)", "origin": "加拿大", "shelf_life_days": 540},
        "3182550702331": {"brand": "皇家 (Royal Canin)", "origin": "法国/国产", "shelf_life_days": 365},
        "6949313200125": {"brand": "比乐 (Bile)", "origin": "国产", "shelf_life_days": 365},
        "064992202112": {"brand": "爱肯拿 (Acana)", "origin": "加拿大", "shelf_life_days": 540},
        "4901133716577": {"brand": "CIAO (伊纳宝)", "origin": "日本", "shelf_life_days": 730},
        "076344091012": {"brand": "Wellness Core", "origin": "美国", "shelf_life_days": 540},
        "6901234567890": {"brand": "皇家 (Royal Canin)", "origin": "法国/国产", "shelf_life_days": 365},
        "4901133718908": {"brand": "CIAO (伊纳宝)", "origin": "日本", "shelf_life_days": 730},
        "6971161234567": {"brand": "渴望 (Orijen)", "origin": "加拿大", "shelf_life_days": 540}
    }

    # Fetch only unshelved inbound records
    cursor.execute("SELECT * FROM product_inbound WHERE is_shelved = 0")
    inbound_records = cursor.fetchall()
    
    print("\n--- Phase 2: Automatically shelving inbound records to store inventory ---")
    for rec in inbound_records:
        barcode = rec["barcode"]
        inbound_id = rec["id"]
        inbound_no = rec["inbound_no"]
        qty = rec["quantity"]
        cost = rec["cost_price"]
        sale = rec["sale_price"]
        
        # Calculate sale price if missing/zero
        if not sale or sale == 0:
            sale = cost * 1.5
            
        img_data = cached_images.get(barcode, rec["img_data"] or "")
        
        # Get brand, origin, shelf_life_days details
        details = product_details.get(barcode, {"brand": "", "origin": "", "shelf_life_days": rec["shelf_life_days"] or 365})
        brand_val = details["brand"]
        origin_val = details["origin"]
        shelf_life = details["shelf_life_days"]

        # 1. Update product_inbound record
        cursor.execute("""
            UPDATE product_inbound 
            SET sale_price = %s, img_data = %s, is_shelved = 1 
            WHERE id = %s
        """, (sale, img_data, inbound_id))

        # 2. Check if product already exists in products table
        cursor.execute("SELECT product_id FROM products WHERE barcode = %s", (barcode,))
        prod = cursor.fetchone()
        
        if prod:
            product_id = prod["product_id"]
            # Update existing product stock and prices
            cursor.execute("""
                UPDATE products 
                SET stock_current = stock_current + %s, stock_curr = stock_curr + %s, 
                    cost_price = %s, sale_price = %s, img_data = %s, supplier = %s,
                    brand = %s, origin = %s, shelf_life_days = %s
                WHERE product_id = %s
            """, (qty, qty, cost, sale, img_data, rec["supplier"], brand_val, origin_val, shelf_life, product_id))
            print(f"Updated existing store product: {rec['product_name']} (Barcode: {barcode}), Added stock: {qty}")
        else:
            # Insert new product master record
            cursor.execute("""
                INSERT INTO products 
                (barcode, name, spec, category, img_data, cost_price, sale_price, stock_current, stock_curr, supplier, is_active, unit, is_deleted, brand, origin, shelf_life_days)
                VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, 1, '件', 0, %s, %s, %s)
            """, (barcode, rec["product_name"], rec["spec"] or "件", rec["category"], img_data, cost, sale, qty, qty, rec["supplier"], brand_val, origin_val, shelf_life))
            product_id = cursor.lastrowid
            print(f"Created new store product: {rec['product_name']} (Barcode: {barcode}) with stock: {qty}")

        # 3. Handle product_batches
        prod_date = rec["production_date"]
        shelf_life = rec["shelf_life_days"] or 365
        
        if isinstance(prod_date, str):
            prod_date_obj = datetime.strptime(prod_date, "%Y-%m-%d").date()
        elif isinstance(prod_date, datetime):
            prod_date_obj = prod_date.date()
        else:
            prod_date_obj = prod_date  # already a datetime.date object
            
        expiry_date = prod_date_obj + timedelta(days=shelf_life)
        
        cursor.execute("""
            INSERT INTO product_batches (batch_id, product_id, expiry_date, initial_qty, current_qty)
            VALUES (%s, %s, %s, %s, %s)
            ON DUPLICATE KEY UPDATE current_qty = VALUES(current_qty)
        """, (inbound_no, product_id, expiry_date.strftime("%Y-%m-%d"), qty, qty))

    db.commit()
    cursor.close()
    db.close()
    print("\n>>> SUCCESS! All products in the inbound records have been fully shelved, updated with images/prices, and synced to the store stock!")

if __name__ == "__main__":
    update_and_shelve_products()
