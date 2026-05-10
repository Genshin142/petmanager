import mysql.connector
from mysql.connector import Error

def seed_database():
    connection = None
    try:
        config = {
            'user': 'root',
            'password': '362345943', 
            'host': '127.0.0.1',
            'database': 'petstore'
        }

        print(f"Connecting to MySQL as {config['user']}...")
        connection = mysql.connector.connect(**config)
        
        if connection.is_connected():
            cursor = connection.cursor()

            # 1. 手动检测并补全列 (通用方案)
            print("Checking pets table columns...")
            cursor.execute("DESC pets")
            existing_columns = [col[0] for col in cursor.fetchall()]

            required_columns = {
                'age_months': "INT UNSIGNED AFTER gender",
                'health_status': "VARCHAR(50) AFTER weight",
                'medical_history': "TEXT AFTER health_status",
                'vaccine_status': "TEXT AFTER medical_history",
                'dietary_habit': "TEXT AFTER vaccine_status",
                'join_time': "DATETIME DEFAULT CURRENT_TIMESTAMP AFTER current_status"
            }

            for col, definition in required_columns.items():
                if col not in existing_columns:
                    print(f"Adding missing column: {col}...")
                    cursor.execute(f"ALTER TABLE pets ADD COLUMN {col} {definition}")
            
            # 扩展 current_status 长度
            cursor.execute("ALTER TABLE pets MODIFY COLUMN current_status VARCHAR(50);")

            # 2. 检查 services 表列
            print("Checking services table columns...")
            cursor.execute("DESC services")
            service_cols = [col[0] for col in cursor.fetchall()]
            
            svc_required = {
                'commission_value': "DECIMAL(7,2) UNSIGNED DEFAULT 10.00 AFTER duration_min",
                'description': "TEXT AFTER commission_value",
                'icon_path': "VARCHAR(255) AFTER description",
                'status': "TINYINT DEFAULT 1 AFTER icon_path"
            }

            for col, definition in svc_required.items():
                if col not in service_cols:
                    print(f"Adding missing column to services: {col}...")
                    cursor.execute(f"ALTER TABLE services ADD COLUMN {col} {definition}")
            
            # 更新分类枚举值以匹配 UI (先清空以防旧数据冲突)
            print("Cleaning services for schema update...")
            cursor.execute("SET FOREIGN_KEY_CHECKS = 0;")
            cursor.execute("TRUNCATE TABLE services;")
            cursor.execute("SET FOREIGN_KEY_CHECKS = 1;")
            
            print("Updating category enum values...")
            cursor.execute("ALTER TABLE services MODIFY COLUMN category ENUM('洗护', '美容', '保健', '寄养', '接送') NOT NULL;")
            
            # 统一清理：移除不再需要的 commission_type 字段
            if 'commission_type' in service_cols:
                print("Removing old column: commission_type...")
                cursor.execute("ALTER TABLE services DROP COLUMN commission_type")

            # 3. 寄养房间数据填充 (使用已存在的 boarding_rooms)
            print("Seeding boarding_rooms...")
            cursor.execute("SET FOREIGN_KEY_CHECKS = 0;")
            cursor.execute("TRUNCATE TABLE boarding_rooms;")
            cursor.execute("SET FOREIGN_KEY_CHECKS = 1;")
            
            room_sql = "INSERT INTO boarding_rooms (room_no, room_type, status, description) VALUES (%s, %s, %s, %s)"
            rooms_data = []
            # 101-110: 标准房 (10间)
            for i in range(101, 111):
                rooms_data.append((str(i), '标准房', '空闲', '标准温馨隔间，每日定时清洁'))
            # 111-115: 豪华房 (5间)
            for i in range(111, 116):
                rooms_data.append((str(i), '豪华房', '空闲', '全透明景观套房，配备24h高清监控'))
            # 116-120: 多宠房 (5间)
            for i in range(116, 121):
                rooms_data.append((str(i), '多宠房', '空闲', '超大共享空间，适合同家庭多只宠物'))
            
            cursor.executemany(room_sql, rooms_data)

            # 4. 基础数据准备
            print("Checking product tables for img_data column...")
            for table in ['products', 'product_inbound']:
                cursor.execute(f"DESC {table}")
                cols = [c[0] for c in cursor.fetchall()]
                if 'img_data' not in cols:
                    print(f"Adding img_data to {table}...")
                    cursor.execute(f"ALTER TABLE {table} ADD COLUMN img_data LONGTEXT AFTER category")

            print("Cleaning old data...")
            cursor.execute("SET FOREIGN_KEY_CHECKS = 0;")
            cursor.execute("TRUNCATE TABLE members;")
            cursor.execute("TRUNCATE TABLE pets;")
            cursor.execute("TRUNCATE TABLE products;")
            cursor.execute("TRUNCATE TABLE product_inbound;")
            cursor.execute("TRUNCATE TABLE sys_employees;")
            cursor.execute("SET FOREIGN_KEY_CHECKS = 1;")

            import base64
            import os
            import json
            def get_b64(path):
                if not os.path.exists(path): return ""
                with open(path, "rb") as f:
                    return base64.b64encode(f.read()).decode('utf-8')

            # 准备商品图片组
            # Acana 犬粮 (正面, 侧面, 背面, 配料)
            acana_imgs = [
                get_b64(r"C:\Users\任坤\.gemini\antigravity\brain\ca88cb1d-9d84-4f5d-9ef9-08a103e6e5c6\acana_front_1778385822114.png"),
                get_b64(r"C:\Users\任坤\.gemini\antigravity\brain\ca88cb1d-9d84-4f5d-9ef9-08a103e6e5c6\acana_side_1778385836861.png"),
                get_b64(r"C:\Users\任坤\.gemini\antigravity\brain\ca88cb1d-9d84-4f5d-9ef9-08a103e6e5c6\acana_back_1778385852366.png"),
                get_b64(r"C:\Users\任坤\.gemini\antigravity\brain\ca88cb1d-9d84-4f5d-9ef9-08a103e6e5c6\acana_ingredients_1778385869295.png")
            ]
            # Zeze 猫爬架 (正面, 细节, 背面, 说明书)
            zeze_imgs = [
                get_b64(r"C:\Users\任坤\.gemini\antigravity\brain\ca88cb1d-9d84-4f5d-9ef9-08a103e6e5c6\zeze_front_1778385884934.png"),
                get_b64(r"C:\Users\任坤\.gemini\antigravity\brain\ca88cb1d-9d84-4f5d-9ef9-08a103e6e5c6\zeze_detail_1778385904361.png"),
                get_b64(r"C:\Users\任坤\.gemini\antigravity\brain\ca88cb1d-9d84-4f5d-9ef9-08a103e6e5c6\zeze_back_1778385925226.png"),
                get_b64(r"C:\Users\任坤\.gemini\antigravity\brain\ca88cb1d-9d84-4f5d-9ef9-08a103e6e5c6\zeze_manual_1778385940779.png")
            ]
            # 皇家犬粮 (之前的图作为单图展示)
            royal_imgs = [
                get_b64(r"C:\Users\任坤\.gemini\antigravity\brain\ca88cb1d-9d84-4f5d-9ef9-08a103e6e5c6\royal_canin_dog_food_1778385348572.png")
            ]

            print("Inserting members...")
            member_sql = "INSERT INTO members (name, phone, level_name, balance, points) VALUES (%s, %s, %s, %s, %s)"
            members_data = [
                ('张三', '13800138000', '普通会员', 500.00, 100),
                ('李芳', '13911112222', '金卡会员', 2500.00, 500),
                ('王强', '13733334444', '银卡会员', 120.00, 50),
                ('赵六', '13366667777', '金卡会员', 3000.00, 800)
            ]
            cursor.executemany(member_sql, members_data)

            print("Inserting pets...")
            pet_sql = "INSERT INTO pets (pet_name, species, breed, gender, member_id, current_status) VALUES (%s, %s, %s, %s, %s, %s)"
            pets_data = [
                ('布丁', '狗', '金毛犬', '公', 1, '在家'),
                ('芝麻', '猫', '英短蓝猫', '母', 2, '寄养中'),
                ('豆豆', '狗', '柴犬', '公', 2, '洗护中'),
                ('小雪', '狗', '萨摩耶', '母', 4, '在家')
            ]
            cursor.executemany(pet_sql, pets_data)

            print("Skipping initial products (waiting for shelving)...")

            print("Seeding enriched product_inbound records...")
            inbound_sql = "INSERT INTO product_inbound (inbound_no, barcode, product_name, category, img_data, supplier, quantity, cost_price, production_date, is_shelved) VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s)"
            inbound_data = [
                ('IN20240401001', '064992201234', '渴望(Acana)经典赤足犬粮 2kg', '主食', json.dumps(acana_imgs), '渴望官方代理', 15, 245.0, '2024-03-10', 0),
                ('IN20240402001', '6977778881112', 'ZEZE 仙人掌剑麻猫爬架', '用品', json.dumps(zeze_imgs), 'ZEZE工厂直供', 8, 128.0, '2024-04-01', 0),
                ('IN20240403001', '6901234567890', '皇家全犬期犬粮 10kg', '主食', json.dumps(royal_imgs), '皇家官方渠道', 10, 210.0, '2024-01-10', 1)
            ]
            cursor.executemany(inbound_sql, inbound_data)

            print("Seeding initial employees...")
            employees_sql = """
                INSERT INTO sys_employees 
                (username, password, real_name, role, department, gender, age, phone, email, id_card, base_salary, join_date, status, img_url, emergency_contact, emergency_phone, address, education) 
                VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s)
            """
            employees_data = [
                ('admin', '123456', '陈店长', '店长', '管理部', '男', 35, '13800138000', 'admin@pet.com', '110101199001011234', 8000.00, '2023-01-01', '在职', '', '陈夫人', '13800138001', '上海市浦东新区张江镇', '本科'),
                ('staff01', '123456', '小王', '收银', '服务部', '女', 24, '13911112222', 'wang@pet.com', '110101199905054321', 4500.00, '2024-03-15', '在职', '', '王父', '13911110000', '上海市徐汇区漕河泾', '大专'),
                ('staff02', '123456', '大李', '美容师', '技术部', '男', 28, '13766667777', 'li@pet.com', '110101199510105566', 5500.00, '2023-11-20', '在职', '', '李母', '13766661111', '上海市闵行区莘庄', '高中')
            ]
            cursor.executemany(employees_sql, employees_data)

            connection.commit()

            print("Seeding initial services...")
            services_sql = """
                INSERT INTO services (name, category, price, duration_min, commission_value, description, icon_path, status) 
                VALUES (%s, %s, %s, %s, %s, %s, %s, %s)
            """
            services_data = [
                # 1. 洗护类 (S001-S005)
                ('基础三项护理', '洗护', 45.00, 20, 15.00, '宠物店最核心的基础护理组合。包含：1.专业剪指甲并打磨平整防止抓伤；2.清理外耳道积垢并拔除耳毛；3.科学挤压肛门腺预防囊肿异味。使用专用止血粉及植物洗耳水。', 'wash_three.png', 1),
                ('全身深度洗护', '洗护', 128.00, 60, 40.00, '包含基础三项护理。选用美国“克里斯汀森”或“艾特美”系列高端洗发水。流程：二次深度去油清洗、精华护毛素滋养按摩、足底毛修剪、大功率吹水机彻底吹干及全身分层拉毛梳理。', 'wash_deep.png', 1),
                ('专业除臭洗护', '洗护', 168.00, 75, 50.00, '包含基础三项护理。针对体味较重的宠物，选用日本“爱丽思”生物酶除臭沐浴露，有效分解皮脂异味分子。包含深度清洗、除臭SPA泡浴及专业干毛护理，洗后清爽留香。', 'wash_odor.png', 1),
                ('眼部泪痕清理', '洗护', 35.00, 15, 10.00, '针对猫狗重度泪痕。使用澳洲“蓝血”泪痕净配合专业纳米清洁棉，温和软化并清除眼周干结垢渍，长期护理可改善眼周毛发发黄现象，预防眼部感染。', 'wash_tear.png', 1),
                ('爪部滋润护理', '洗护', 35.00, 15, 10.00, '使用德国“莎金氏”天然蜂蜡肉垫护理膏。针对脚垫粗糙、干裂进行深度涂抹与按摩吸收，在脚垫表面形成保护膜，增加弹性，预防冬季眲裂及夏日高温烫伤风险。', 'wash_paw.png', 1),
                
                # 2. 美容类 (S006-S009)
                ('全身推子造型', '美容', 188.00, 100, 70.00, '利润支撑项目。使用专业“安迪斯”静音推子。流程：全身短毛修剪（保留1-3mm安全长度），头尾部萌系圆头造型。适合夏季散热或家庭打理，包含全身深度洗护全套流程。', 'groom_clip.png', 1),
                ('全手剪精修造型', '美容', 388.00, 180, 150.00, '技艺核心项目。由首席美容师使用“银弧”专业手工直剪及牙剪进行逐层修剪。打造贵宾、比熊泰迪装等高难度网红造型，线条流畅自然。包含全身深度洗护全套流程。', 'groom_scissor.png', 1),
                ('局部卫生修剪', '美容', 50.00, 30, 20.00, '针对性修剪。包含：1.眼睛周围遮眼毛精修；2.腹部及生殖器周边卫生毛推短；3.脚蹼间杂毛清理。有效预防宠物在外出时沾染污垢，保持居家卫生。', 'groom_trim.png', 1),
                ('手工毛发开结', '美容', 80.00, 60, 40.00, '针对重度打结宠物的专项护理。使用“傲宠”专业开结喷雾配合排梳，由美容师手工逐一解开毛结，最大限度保留原有被毛，减少宠物扯痛感。按小时额外计费。', 'groom_mat.png', 1),
                
                # 3. 保健类 (S010-S013)
                ('专业洁牙服务', '保健', 58.00, 20, 25.00, '预防性护理。使用“多美洁”免刷牙清洁凝胶或专用指套牙刷，清理齿缝残留及初期牙菌斑。改善口腔异味，长期坚持可有效减缓牙石生成，保护牙龈健康。', 'health_dental.png', 1),
                ('体内驱虫护理', '保健', 60.00, 10, 20.00, '选用德国“拜耳-拜宠清”或“海乐妙”等一线品牌。针对肠道内线虫、蛔虫、钩虫及绦虫进行全面清除，是爱宠每季必备的基础健康保障。', 'health_bug_in.png', 1),
                ('体外驱虫防护', '保健', 98.00, 10, 20.00, '选用法国“福来恩”或“超可信”高效滴剂。有效杀灭并预防跳蚤、蜱虫、虱子等体外寄生虫，药效可持续30天，为爱宠建立全天候防护屏障。', 'health_bug_out.png', 1),
                ('耳道深度灌洗', '保健', 80.00, 25, 30.00, '针对耳炎或重度耳螨。使用法国“维克”耳净灌洗液，配合无菌棉签进行深层清创。有效溶解顽固耳垢，抑制细菌生长，缓解宠物频繁挠头等症状。', 'health_ear.png', 1),
                
                # 4. 寄养类 - 日托系列 (S014-S016)
                ('日托寄养 (普通房间)', '寄养', 40.00, 600, 10.00, '日间托管，标准独立隔间。包含：每日充足饮水、两次室内陪玩、基础卫生清理。环境恒温通风，适合上班族家庭。不含过夜服务。', 'boarding_day_std.png', 1),
                ('日托寄养 (豪华套房)', '寄养', 100.00, 600, 25.00, '日间托管，全透明景观套房。包含：24小时远程监控查看权限、每日新鲜果蔬加餐、资深员工一对一互动。不含过夜服务。', 'boarding_day_lux.png', 1),
                ('日托寄养 (多宠家庭房)', '寄养', 150.00, 600, 40.00, '日间托管，超大共享空间。专为同一家庭的2-3只宠物设计，减少分离焦虑，空间宽敞可放置自带窝垫。不含过夜服务。', 'boarding_day_fam.png', 1),
                
                # 4. 寄养类 - 全托系列 (S017-S019)
                ('全托寄养 (普通房间)', '寄养', 90.00, 1440, 20.00, '24小时全天候寄养，普通独立隔间。包含：每日两次户外遛狗、定时投喂、每日环境消毒、夜间值班巡查。', 'boarding_std.png', 1),
                ('全托寄养 (豪华套房)', '寄养', 260.00, 1440, 50.00, '24小时全天候寄养，五星级景观套房。包含：全天候实时视频监控、每日手工鲜食补餐、资深员工一对一户外陪玩30分钟。', 'boarding_lux.png', 1),
                ('全托寄养 (多宠家庭房)', '寄养', 380.00, 1440, 70.00, '24小时全天候寄养，超大家庭套间。允许同家庭多只宠物共同居住，空间宽广，配备多层爬架或休息垫，提供居家级舒适体验。', 'boarding_family.png', 1),
                
                # 5. 接送类 (S020-S022)
                ('单程接宠入店', '接送', 40.00, 30, 10.00, '10km范围内专车接宠。车辆配备专用航空箱及即时消毒系统，由具备宠物急救知识的司机操作，确保入店途中安全无应激。', 'pickup_in.png', 1),
                ('单程送宠回家', '接送', 40.00, 30, 10.00, '服务结束后专车送回。将宠物安全护送至家长手中，并现场交接宠物在店期间的健康及美容状态报告。', 'dropoff_home.png', 1),
                ('往返接送服务', '接送', 70.00, 60, 20.00, '一站式全流程闭环接送。包含入店接宠及回家送宠两次行程，价格比单程更优惠，适合工作繁忙无法到店的家长。', 'transport_round.png', 1)
            ]
            cursor.executemany(services_sql, services_data)

            connection.commit()
            print(">>> Success! Mock data seeded.")

    except Error as e:
        print(f"Error: {e}")
    finally:
        if connection and connection.is_connected():
            cursor.close()
            connection.close()

if __name__ == "__main__":
    seed_database()
