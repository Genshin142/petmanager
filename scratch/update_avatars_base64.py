import mysql.connector
import base64
import os

db_config = {
    "host": "localhost",
    "user": "root",
    "password": "362345943",
    "database": "petstore"
}

avatar_dir = r"E:\QT\work\PetManager\assets\avatars"

def image_to_base64(file_path):
    try:
        with open(file_path, "rb") as image_file:
            ext = os.path.splitext(file_path)[1].lower().replace('.', '')
            if ext == 'jpg': ext = 'jpeg'
            encoded_string = base64.b64encode(image_file.read()).decode('utf-8')
            return f"data:image/{ext};base64,{encoded_string}"
    except Exception as e:
        print(f"Error encoding image {file_path}: {e}")
        return None

def update_database():
    try:
        conn = mysql.connector.connect(**db_config)
        cursor = conn.cursor()

        cursor.execute("SELECT real_name FROM sys_employees")
        employees = cursor.fetchall()

        for (name,) in employees:
            found_file = None
            for filename in os.listdir(avatar_dir):
                basename = os.path.splitext(filename)[0]
                if basename == name:
                    found_file = os.path.join(avatar_dir, filename)
                    break
            
            if found_file:
                print(f"Processing {name} -> {found_file}")
                base64_str = image_to_base64(found_file)
                if base64_str:
                    # 直接写回 img_url
                    sql = "UPDATE sys_employees SET img_url = %s WHERE real_name = %s"
                    cursor.execute(sql, (base64_str, name))
                    print(f"Successfully updated Base64 in img_url for {name}")
            else:
                print(f"No image file found for {name}")

        conn.commit()
        print("Update complete.")

    except mysql.connector.Error as err:
        print(f"Database error: {err}")
    finally:
        if 'conn' in locals() and conn.is_connected():
            cursor.close()
            conn.close()

if __name__ == "__main__":
    update_database()
