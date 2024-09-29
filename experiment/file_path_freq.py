import os
import matplotlib.pyplot as plt
from collections import defaultdict

def collect_path_bytes(root_dir):
    path_bytes_count = defaultdict(int)
    total_files = 0

    for root, dirs, files in os.walk(root_dir):
        for filename in files:
            # 获取文件的绝对路径
            file_path = os.path.abspath(os.path.join(root, filename))
            # 计算路径字节数
            path_bytes = len(file_path.encode('utf-8'))  # 使用utf-8编码计算字节数
            # 统计路径字节数的频率
            path_bytes_count[path_bytes] += 1
            total_files += 1

    return path_bytes_count, total_files

# 指定需要统计的根目录
# root_directory = '/'
root_directory = '/home/zxy'

# 收集路径字节数信息
path_bytes_count, total_files = collect_path_bytes(root_directory)

# 输出频率统计结果
print(f"Total files: {total_files}")
print("Path bytes frequency:")
for path_bytes, count in sorted(path_bytes_count.items()):
    print(f"Bytes: {path_bytes}, Count: {count}")

# 将统计结果转换为两个列表，用于绘图
bytes_sizes = list(path_bytes_count.keys())
counts = list(path_bytes_count.values())

# 绘制频率分布图
plt.figure(figsize=(10, 6))
plt.bar(bytes_sizes, counts, color='blue', alpha=0.7)
plt.xlabel('Path Byte Size')
plt.ylabel('Frequency')
plt.title('Frequency Distribution of Path Byte Sizes')
plt.grid(True)
plt.tight_layout()
plt.show()