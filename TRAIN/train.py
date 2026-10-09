# import tensorflow as tf
# # xây dựng mô hình tuyến tính (layer chồng layer).
# from tensorflow.keras.models import Sequential
# from tensorflow.keras.layers import Conv2D, MaxPooling2D, Flatten, Dense, Dropout
# from tensorflow.keras.preprocessing.image import ImageDataGenerator

# # Tạo bộ dữ liệu với ImageDataGenerator
# datagen = ImageDataGenerator(
#     rescale=1./255, validation_split=0.2,  # Chia 80% train, 20% val
#     # rescale =1./255 chuẩn hóa pixel từ [0-255] về [0-1] giúp tăng độ ổn định moo hình
#     rotation_range=15,        # Xoay ảnh ngẫu nhiên trong khoảng ±15 độ
#     shear_range=0.1,          # Biến dạng hình ảnh (shear) trong khoảng 10%
#     zoom_range=0.1,           # Phóng to/thu nhỏ ảnh trong khoảng 10%
#     horizontal_flip=True,    # Lật ngang ngẫu nhiên
#     width_shift_range=0.1,
#     height_shift_range=0.1
# )

# # Load dữ liệu huấn luyện
# train_data = datagen.flow_from_directory(
#     'processed_dataset/',
#     target_size=(24, 24),  # Kích thước ảnh đầu vào
#     color_mode='grayscale',  # chuyển thành ảnh xám
#     batch_size=32,
#     class_mode='binary',  # bài toán phân loại 2 lớp
#     subset='training'  # lấy đúng 80% của train
# )
# # Chuẩn hóa giá trị pixel + xử lý theo chuẩn Keras
# # Load dữ liệu validation
# val_data = datagen.flow_from_directory(
#     'processed_dataset/',
#     target_size=(24, 24),
#     color_mode='grayscale',
#     batch_size=32,
#     class_mode='binary',
#     subset='validation'
# )

# # Xây dựng mô hình CNN
# model = Sequential([
#     Conv2D(32, (3, 3), activation='relu',  # 32 bộ lọc 3x3, học đặc trưng mắt (edge, patterns), relu giúp mô hình học phi tuyến nhanh
#            input_shape=(24, 24, 1)),  # 32 bộ lọc
#     # Lớp giảm kích thước đi 1 nửa, số chiều sâu vẵn giữ nguyên
#     MaxPooling2D(2, 2),
#     # 64 bộ lọc, tầng sâu hơn, học đặc trưng phức tạp hơn (nếp mí, độ cong của mắt)
#     Conv2D(64, (3, 3), activation='relu'),
#     MaxPooling2D(2, 2),
#     Conv2D(128, (3, 3), activation='relu'),
#     MaxPooling2D(2, 2),
#     Flatten(),  # Biến tensor 3D thành vector 1D để đưa vào lớp kết nối tổng hợp
#     # Fully connected layer, học mối quan hệ phi tuyến giữu đặc trưng ảnh và trạng thái mắt
#     Dense(128, activation='relu'),
#     Dropout(0.5),  # Tránh overfitting, tăng tổng quát hóa
#     # 1 output: mắt mở (1) hoặc nhắm (0), sigmoid là hàm kích hoạt-> chuyển giá trị đầu ra thành xs(0-1)
#     Dense(1, activation='sigmoid') # lớp đầu ra, "1" là số neuron đầu ra, biểu diễn xác suất
# ])

# # Biên dịch mô hình
# model.compile(optimizer='adam', loss='binary_crossentropy',
#               metrics=['accuracy'])  # tối ưu hóa nhanh, dùng cho bài toán phân loại 2 lớp, đánh giá độ chính xác

# # Huấn luyện mô hình
# # huấn luyện 10 vòng, học trên train và val để đánh giá
# model.fit(train_data, validation_data=val_data, epochs=30)

# # Lưu mô hình sau khi huấn luyện
# model.save('eye_model.h5')

# print("✔ Huấn luyện hoàn tất, mô hình đã được lưu vào eye_model.h5!")
import cv2
from ultralytics import YOLO

# Load model (nhẹ → nhanh)
model = YOLO("besthuman.pt")

# Mở webcam
cap = cv2.VideoCapture(0)

while True:
    ret, frame = cap.read()
    if not ret:
        break

    # Detect
    results = model(frame)

    # Duyệt từng object
    for r in results:
        for box in r.boxes:
            cls_id = int(box.cls[0])
            conf = float(box.conf[0])

            # Chỉ lấy class "person" (class 0 trong COCO)
            if cls_id == 0:
                x1, y1, x2, y2 = map(int, box.xyxy[0])

                # Vẽ bounding box
                cv2.rectangle(frame, (x1, y1), (x2, y2), (0, 255, 0), 2)
                cv2.putText(frame, f"Person {conf:.2f}",
                            (x1, y1 - 10),
                            cv2.FONT_HERSHEY_SIMPLEX,
                            0.5, (0, 255, 0), 2)

    cv2.imshow("YOLO Person Detection", frame)

    # Nhấn q để thoát
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

cap.release()
cv2.destroyAllWindows()
