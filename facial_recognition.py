# -*- coding: gbk -*-

import cv2
import numpy as np
import os
import serial
import keyboard 

face_cascade = cv2.CascadeClassifier('./data/haarcascade_frontalface_default.xml')

KNOWN_DIR = './data/known'
UNKNOWN_LABEL = 'Unknown'
RECOG_FACE_SIZE = (100, 100)
RECOG_THRESHOLD = 65.0 

#==============================================================================
#   加载已知人脸数据函数
#       功能：从指定文件夹读取所有人脸照片，提取人脸特征用于训练
#       输入：known_dir - 已知人脸文件夹路径（每个子文件夹代表一个人）
#       输出：images - 人脸图像列表，labels - 对应的标签ID，label_names - ID到姓名的映射
#==============================================================================
def load_known_faces(known_dir):
    label_names = {}  # 字典：{label_id: 人名}，例如 {0: "张三", 1: "李四"}
    images = []       # 存储所有人脸图像（100x100灰度图）
    labels = []       # 存储对应的标签ID，与images一一对应
    label_id = 0      # 当前分配的标签ID，每个人一个唯一ID

    # 检查文件夹是否存在
    if not os.path.isdir(known_dir):
        print(f"Known faces folder not found: {known_dir}")
        return images, labels, label_names

    # 遍历known文件夹下的每个子文件夹（每个子文件夹代表一个人）
    for person_name in sorted(os.listdir(known_dir)):
        person_dir = os.path.join(known_dir, person_name)  # 拼接完整路径
        if not os.path.isdir(person_dir):  # 跳过非文件夹（如普通文件）
            continue

        label_names[label_id] = person_name  # 记录这个ID对应的人名
        
        # 遍历该人的所有照片
        for fname in os.listdir(person_dir):
            # 只处理图片文件
            if not fname.lower().endswith(('.jpg', '.jpeg', '.png', '.bmp')):
                continue
            
            img_path = os.path.join(person_dir, fname)  # 完整图片路径
            img = cv2.imread(img_path)  # 读取图片
            if img is None:  # 读取失败则跳过
                continue
            
            gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)  # 转换为灰度图
            faces = face_cascade.detectMultiScale(gray, 1.3, 5)  # 检测人脸
            if len(faces) == 0:  # 如果没检测到人脸则跳过这张照片
                continue
            
            # 如果检测到多张人脸，选择面积最大的那个
            x, y, w, h = max(faces, key=lambda f: f[2] * f[3])
            face_roi = gray[y:y + h, x:x + w]  # 裁剪出人脸区域
            face_roi = cv2.resize(face_roi, RECOG_FACE_SIZE)  # 统一缩放到100x100
            images.append(face_roi)  # 添加到训练集
            labels.append(label_id)  # 标记为当前人的ID

        label_id += 1  # 处理下一个人时ID加1

    return images, labels, label_names

#==============================================================================
#   初始化人脸识别器函数
#       功能：创建LBPH识别器并用已知人脸数据进行训练
#       输入：known_dir - 已知人脸文件夹路径
#       输出：recognizer - 训练好的识别器，label_names - ID到姓名的映射
#==============================================================================
def init_recognizer(known_dir):
    # 检查是否安装了opencv-contrib-python（包含face模块）
    if not hasattr(cv2, 'face'):
        raise RuntimeError('OpenCV contrib not installed. Install opencv-contrib-python.')

    # 加载所有已知人脸数据
    images, labels, label_names = load_known_faces(known_dir)
    if len(images) == 0:  # 如果没有训练数据，则禁用识别功能
        print('No known face images found. Recognition will be disabled.')
        return None, {}

    # 创建LBPH（局部二值模式直方图）人脸识别器
    recognizer = cv2.face.LBPHFaceRecognizer_create()
    # 使用加载的图像和标签训练识别器
    recognizer.train(images, np.array(labels))
    return recognizer, label_names

#==============================================================================
#   1.多人脸形心检测函数 
#       输入：视频帧
#       输出：各人脸总形心坐标
#==============================================================================
def Detection(frame, recognizer, label_names):
    gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)  #转换为灰度图
    
    faces = face_cascade.detectMultiScale(gray, 1.3, 5) #人脸检测
    
    recog_name = None
    has_face = False
    if len(faces)>0: 
        has_face = True
        main_face = max(faces, key=lambda f: f[2] * f[3])
        for (x,y,w,h) in faces:
            cv2.rectangle(frame, (x,y), (x+w,y+h), (0,255,0), 2)
            X = x+w//2
            Y = y+h//2
            center_pt=(X,Y)   #各人脸中点坐标
            cv2.circle(frame, center_pt, 8, (0,0,255), -1)   #绘制各人脸中点
        if recognizer is not None:
            x, y, w, h = main_face
            face_roi = gray[y:y + h, x:x + w]
            face_roi = cv2.resize(face_roi, RECOG_FACE_SIZE)
            label_id, confidence = recognizer.predict(face_roi)
            if confidence <= RECOG_THRESHOLD and label_id in label_names:
                recog_name = label_names[label_id]
            else:
                recog_name = UNKNOWN_LABEL
        centroid_X = int(np.mean(faces, axis=0)[0] + np.mean(faces, axis=0)[2]//2) # 各人脸形心横坐标
        centroid_Y = int(np.mean(faces, axis=0)[1] + np.mean(faces, axis=0)[3]//2) # 各人脸形心纵坐标
        centroid_pt=(centroid_X,centroid_Y)   #各人脸形心坐标
        cv2.circle(frame, centroid_pt, 8, (0,0,255), -1)   #绘制各人脸形心
    else:
        frame_h, frame_w = frame.shape[:2]
        centroid_X = frame_w // 2
        centroid_Y = frame_h // 2
    #==========================================================================
    #     绘制参考线
    #==========================================================================
    frame_h, frame_w = frame.shape[:2]
    x = 0;
    y = 0;
    w = frame_w // 2;
    h = frame_h // 2;
    
    rectangle_pts = np.array([[x,y],[x+w,y],[x+w,y+h],[x,y+h]], np.int32) #最小包围矩形各顶点
    cv2.polylines(frame, [rectangle_pts], True, (0,255,0), 2) #绘制最小包围矩形
    
    x2 = frame_w // 2;
    y2 = frame_h // 2;
    rectangle_pts2 = np.array([[x2,y2],[x2+w,y2],[x2+w,y2+h],[x2,y2+h]], np.int32) #最小包围矩形各顶点
    cv2.polylines(frame, [rectangle_pts2], True, (0,255,0), 2) #绘制最小包围矩形

    #==========================================================================
    #     显示
    #==========================================================================
    if recog_name:
        cv2.putText(frame, recog_name, (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 1.0, (0, 255, 0), 2)
    cv2.imshow('frame',frame)
    
    return centroid_X,centroid_Y,recog_name,has_face
                
#==============================================================================
#   ****************************主函数入口***********************************
#==============================================================================

# 初始化识别器
recognizer, label_names = init_recognizer(KNOWN_DIR)

# 设置串口参数
ser = serial.Serial()
ser.baudrate = 115200    # 设置比特率为115200bps
ser.port = 'COM5'      # 单片机接在哪个串口，就写哪个串口。这里默认接在"COM5"端口
ser.open()             # 打开串口

# 定义USMART命令发送函数
def send_usmart_cmd(cmd):
    """发送USMART命令"""
    ser.write((cmd + '\r\n').encode())
    print(f"Sent USMART: {cmd}")

# 注册快捷键 - 按数字键发送PID调试命令
keyboard.add_hotkey('1', lambda: send_usmart_cmd('pid_set_x(50,0,300)'))
keyboard.add_hotkey('2', lambda: send_usmart_cmd('pid_set_x(30,0,200)'))
keyboard.add_hotkey('3', lambda: send_usmart_cmd('pid_set_y(50,0,300)'))
keyboard.add_hotkey('4', lambda: send_usmart_cmd('pid_set_y(30,0,200)'))

cap = cv2.VideoCapture(1) #打开摄像头
cap.set(cv2.CAP_PROP_FRAME_WIDTH, 1280)  # 请求1280分辨率
cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 720)   # 请求720分辨率
if not cap.isOpened():
    print("Camera open failed")
    ser.close()
    raise SystemExit(1)
print("Camera opened")

# 读一帧确认实际分辨率
ret, frame = cap.read()
if not ret:
    print("Camera read failed")
    ser.close()
    raise SystemExit(1)
frame_h, frame_w = frame.shape[:2]
print(f"Actual resolution: {frame_w}x{frame_h}")

#先发送一个中心坐标使初始化时云台保持水平
data = '#'+str(frame_w // 2)+'$'+str(frame_h // 2)+'\r\n'
ser.write(data.encode())

last_name = None

while(cap.isOpened()):
    _, frame = cap.read()
    
    X, Y, recog_name, has_face = Detection(frame, recognizer, label_names) #执行多人脸形心检测函数
    
    if(X<10000):
        print('X = ')
        print(X)
        print('Y =')
        print(Y)
        #按照协议将形心坐标打包并发送至串口
        data = '#'+str(X)+'$'+str(Y)+'\r\n'
        print('Send: ' + data)  # 调试：打印实际发送的数据
        ser.write(data.encode())

    if has_face:
        send_name = recog_name if recog_name else UNKNOWN_LABEL
    else:
        send_name = 'No Face'  # 没检测到人脸
    
    if send_name != last_name:
        name_data = 'NAME:' + send_name + '\r\n'
        ser.write(name_data.encode('gbk'))
        last_name = send_name
    
    k = cv2.waitKey(5) & 0xFF
    if k==27:   #按“Esc”退出
        break

ser.close()                                     # 关闭串口
cv2.destroyAllWindows()
cap.release()





