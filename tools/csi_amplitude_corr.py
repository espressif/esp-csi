from sklearn.decomposition import PCA
import serial
import sys
import time
import math
import csv
import numpy as np
import matplotlib.pyplot as plt
from math import sqrt, atan2
import pandas as pd
import seaborn as sns

# 计算两点之间的距离


def eucli_distance(A, B):
    return np.sqrt(sum(np.power((np.mat(A) - np.mat(B)), 2)))

# 中值滤波


def median_filter(src, k=50):
    # print(src)
    edge = int((k - 1) / 2)
    dst = []
    if len(src) - 1 - edge <= edge:
        print("The parameter k is to large.")
        return None

    for i in range(len(src)):
        if i <= edge - 1:
            dst.append(src[i])
        else:
            # nm:neighbour matrix
            nm = src[i - edge: i + edge + 1]

            if src[i] == np.max(nm) or src[i] == np.min(nm):
                dst.append(np.median(nm))
            else:
                dst.append(src[i])
    return dst


# 获取 CSI 振幅
def get_amplitude(amplitude_list, data_list, data_len):
    data_imaginary_list, data_real_list = [], []
    try:
        for i in range(0, data_len, 2):
            data_imaginary_list.append(int(data_list[i]))
            data_real_list.append(int(data_list[i + 1]))
    except Exception as e:
        print(e)
        return None

    for i in range(data_len // 2):
        amplitude_list[i].append(
            sqrt(data_imaginary_list[i]**2+data_real_list[i]**2))

        # phase_list[i].append(
        # atan2(data_imaginary_list[i], data_real_list[i]))
        # amplitude_list[i].append(
        # atan2(data_imaginary_list[i], data_real_list[i]))

    return None


def get_data_file(file_name, data_len_filter=56, max_line=10240):
    csv_file = csv.reader(open(file_name, 'r'))
    amplitude_len = 0
    amplitude_list = [[]for i in range(data_len_filter//2)]
    rssi_list = []

    for row in csv_file:
        if len(row) < 26:
            continue

        if "CSI_DATA" not in row[0]:
            continue

        data_len = int(row[23])
        if data_len != data_len_filter:
            continue

        data_list = (row[25].split('[ ')[1]).split(' ')
        get_amplitude(amplitude_list, data_list, data_len)
        amplitude_len += 1

        rssi_list.append(int(row[4]))

        if(amplitude_len > max_line):
            break

    return amplitude_len, amplitude_list, rssi_list


def data_corr(data):
    data_frame = pd.DataFrame(data)
    return data_frame.corr()


# 函数：计算相关系数
def calc_corr(a, b):
    a_avg = sum(a)/len(a)
    b_avg = sum(b)/len(b)

    # 计算分子，协方差————按照协方差公式，本来要除以n的，由于在相关系数中上下同时约去了n，于是可以不除以n
    cov_ab = sum([(x - a_avg)*(y - b_avg) for x, y in zip(a, b)])
    # 计算分母，方差乘积————方差本来也要除以n，在相关系数中上下同时约去了n，于是可以不除以n
    sq = math.sqrt(sum([(x - a_avg)**2 for x in a])
                   * sum([(x - b_avg)**2 for x in b]))
    corr_factor = cov_ab/sq

    return corr_factor

    # X1=pd.Series(a)
    # Y1=pd.Series(b)

    # x1=X1.dropna()
    # y1=Y1.dropna()
    # n=x1.count()
    # x1.index=np.arange(n)
    # y1.index=np.arange(n)

    # data = x1.corr(y1, method='spearman')
    # return data


def list_spin(data):
    spin_data = []
    for i in range(len(data[0])):
        b = []
        for j in range(len(data)):
            b.append(data[j][i])
        spin_data.append(b)
    return spin_data


'''
# 核密度
plt.figure(figsize=(30, 20), dpi=100)

amplitude_len, amplitude_list = get_data_file('../test/test_static.csv')

for i in range(2):
    amplitude_list_index = [x[i*500:(i+1)*500]for x in amplitude_list]
    amplitude_corr_list = data_corr(amplitude_list_index)

    sns.distplot(amplitude_corr_list, color='r', label=str(i), hist=False)


amplitude_len, amplitude_list = get_data_file('../test/test_dynamic.csv')

for i in range(2):
    amplitude_list_index = [x[i*500:(i+1)*500]for x in amplitude_list]
    amplitude_corr_list = data_corr(amplitude_list_index)

    sns.distplot(amplitude_corr_list, color='b', label=str(i), hist=False)

plt.show()
'''

'''
file_name = ['../test/2020-10-28_static.csv',
             '../test/2020-10-28_man.csv',
             '../test/2020-10-28_dync.csv']

for i in range(len(file_name)):
    rssi_static_std = []
    amplitude_len, amplitude_list, rssi_list = get_data_file(file_name[i])

    for j in range(0, 20):
        rssi_filter_list = median_filter(rssi_list[j*200:(j+1)*200],k=50)
        rssi_static_std.append(np.var(rssi_filter_list))

    plt.plot(rssi_static_std, label=str(i))

plt.legend()
plt.savefig("../data/rssi_std.jpg", transparent=True, dpi=2000)
plt.show()
'''

file_name = ['../test/2020-10-28_static.csv',
             '../test/2020-10-28_man.csv',
             '../test/2020-10-28_dync.csv']


color_list = ["r", "g", "b", "y", "m"]


# #载入库
# import numpy as np
# from scipy.optimize import leastsq
# import pylab as pl
 
# #定义函数形式和误差
# def func(x,p):
#     A,k,theta=p
#     return A*np.sin(2*np.pi*k*x+theta)
# def residuals(p,y,x):
#     return y-func(x,p)


# #生成训练数据
# x=np.linspace(0,-2*np.pi,100)
# A,k,theta=10,0.34,np.pi/6
# y0=func(x,[A,k,theta])
# y1=y0+2*np.random.randn(len(x))



# #trian the para
# p0=[7,0.2,0]#在非线性拟合中，初始参数对结果的好坏有很大的影响
# Para=leastsq(residuals,p0,args=(y1,x))
# a1,a2,a3=Para[0]
# #plot
# import matplotlib.pyplot as plt
# plt.figure(figsize=(8,6))
# plt.scatter(x,y1,color="red",label="Sample Point",linewidth=3) #画样本点
# y=a1*np.sin(2*np.pi*a2*x+a3)
# plt.plot(x,y,color="orange",label="Fitting Line",linewidth=2) #画拟合直线


from scipy import stats

amplitude_corr_list = [[]for x in range(3)]
# for i in range(1):
for i in range(len(file_name)):
    amplitude_len, amplitude_list, rssi_list = get_data_file(file_name[i], max_line=100)
    # for j in range(0, 25):
    #     plt.plot(amplitude_list[j], label = str(i), color = color_list[i])


    X = np.array(list_spin(amplitude_list))
    pca = PCA(n_components=1)
    pca.fit(X)
    amplitude_pca_list = list_spin(pca.transform(X))

    x = []
    for j in range(len(amplitude_pca_list[0])):
        x.append(j)
    plt.plot(amplitude_pca_list[0], label = str(i), color = color_list[i])
    plt.scatter(x, amplitude_pca_list[0], label = str(i), color = color_list[i])

    sum = 0
    for k in range(len(amplitude_pca_list[0]) - 1):
        for l in range(k, len(amplitude_pca_list[0])):
            sum += np.abs(amplitude_pca_list[0][l] - amplitude_pca_list[0][k])
    # print("std: ", sum / len(amplitude_pca_list[0]))



    # for j in range(0, 25):
    #     plt.plot(amplitude_list[j], color = color_list[i])

    # for j in range(0, 50):
    #     amplitude_list_index = [x[j*200:(j+1)*200 - 1] for x in amplitude_list]
    #     for k in range(0, 20):
    #         for l in range(k+1, 21):
    #             # print(amplitude_list_index[k])
    #             amplitude_corr = calc_corr(
    #                 amplitude_list_index[k], amplitude_list_index[l])
    #             # print("=================", amplitude_corr, i, j, k, l)
    #             amplitude_corr_list[i].append(amplitude_corr)


plt.legend()
# plt.savefig("../data/amplitude_corr.jpg", transparent=True, dpi=2000)
plt.show()

while True:
    continue




# for i in range(0, 3):
#     plt.plot(amplitude_corr_list[i], label=str(i))

# plt.legend()
# # plt.savefig("../data/amplitude_corr_all.jpg", transparent=True, dpi=2000)
# plt.show()

list_len = len(amplitude_corr_list[0])//50
print(list_len)

for i in range(1, 2):
    mean_list = []
    for k in range(0, 50):
        # plt.plot(amplitude_corr_list[i], label=str(i))
        # for j in range(i + 1, 3):
        #     print("=======", i, j,
        #           amplitude_corr_list[i][k*list_len:(k+1)*list_len])
        plt.plot(amplitude_corr_list[i]
                 [k*list_len:(k+1)*list_len], label=str(k))

        # mean_list.append(sum_data)

        # mean_list.append(np.sqrt(np.sum(np.square(amplitude_corr_list[i][k*list_len:(k+1)*list_len])-amplitude_corr_list[i][0:list_len])))

        # print("distance: ", np.mean(amplitude_corr_list[i][k*list_len:(k+1)*list_len]))
        # distance = np.sqrt(
        #     sum(np.power(amplitude_corr_list[i][k*list_len:(k+1)*list_len], amplitude_corr_list[j][k*list_len:(k+1)*list_len]), 2))
        # print("distance: ", distance)

    plt.plot(mean_list, label=str(i))

# for k in range(0, list_len):
#     for i in range(0, 2):
#         # plt.plot(amplitude_corr_list[i], label=str(i))
#         for j in range(i + 1, 3):
#             distance = np.sqrt(
#                 sum(np.power(amplitude_corr_list[i][k*list_len:(k+1)*list_len], amplitude_corr_list[j][k*list_len:(k+1)*list_len]), 2))
#             print("distance: ", distance)

plt.legend()
# plt.savefig("../data/amplitude_corr.jpg", transparent=True, dpi=2000)
plt.show()

while True:
    continue

amplitude_list_index = [x[0:200 - 1]for x in amplitude_list]
# amplitude_static_corr_list = data_corr(amplitude_list_index)

# print(amplitude_list_index)


plt.subplot(3, 1, 1)
plt.title("amplitude")
# c=['r','g','b','y','b']
for i in range(0, 25):
    plt.plot(amplitude_list_index[i])

# c=['r','g','b','y','b']

X = np.array(list_spin(amplitude_list_index))
pca = PCA(n_components=7)
pca.fit(X)
# print(pca.transform(X))
amplitude_pca_list = list_spin(pca.transform(X))

plt.subplot(3, 1, 2)
plt.title("amplitude")
for i in range(0, 6):
    plt.plot(amplitude_pca_list[i])


# from sklearn.decomposition import PCA
# import numpy as np
# X = np.array(amplitude_list_index)
# pca=PCA(n_components=5)
# pca.fit(X)
# # print(pca.transform(X))
# amplitude_pca_list = pca.transform(X)

plt.subplot(3, 1, 3)
plt.title("amplitude")
for i in range(2, 6):
    plt.plot(amplitude_pca_list[i])


# test_list = [[]for i in range(32)]

# for j in range (0, len(test_list)):
#     X = np.array([x[j:(j+1)*50]for x in amplitude_list])
#     pca = PCA(n_components=5)
#     pca.fit(X)
#     arr = pca.transform(X)
#     for i in range(len(arr)):
#         test_list[i].append(arr)

# plt.subplot(3, 1, 3)
# plt.title("amplitude")
# for i in range(0, len(test_list)):
#     plt.plot(test_list[i])


# amplitude_len, amplitude_list, rssi_list = get_data_file(
#     '../test/test_dynamic.csv')

# amplitude_list_index = [x[0:1000 - 1]for x in amplitude_list]

# from sklearn.decomposition import PCA
# import numpy as np
# X = np.array(list_spin(amplitude_list_index))
# pca=PCA(n_components=7)
# pca.fit(X)
# # print(pca.transform(X))
# amplitude_pca_list = list_spin(pca.transform(X))

# plt.subplot(3, 1, 3)
# plt.title("amplitude")
# for i in range(2, 6):
#     plt.plot(amplitude_pca_list[i])

plt.legend()
plt.show()


plt.figure(figsize=(16, 10), dpi=90)
color_list = ['r', 'g', 'b', 'y', 'm']


plt.grid(True)  # 添加网格
plt.ion()  # interactive mode
plt.figure(1)
plt.xlabel('times')
plt.ylabel('data')
plt.title('Diagram of UART data by Python')

time_test_list = []
distance_test_list = []


for i in range(1, 20):
    amplitude_list_index = [x[i*500:(i+1)*500 - 1]for x in amplitude_list]
    amplitude_corr_list = data_corr(amplitude_list_index)
    d = eucli_distance(amplitude_static_corr_list, amplitude_corr_list)
    test_mat = np.mat(amplitude_corr_list)
    print(d.mean(), d.var(), np.std(
        rssi_list[i*500:(i+1)*500 - 1]), test_mat.sum())

    distance_test_list.append(d.mean())
    time_test_list.append(i)
    # sns.distplot(amplitude_corr_list,kde_kws={"lw": 1.5, 'linestyle':'--'}, hist=False,color=color_list[i], label=str(i))

plt.plot(time_test_list, distance_test_list, '.-g')
plt.draw()
plt.pause(0.001)

print('------------------')

'''
amplitude_len, amplitude_list,rssi_list = get_data_file(
    '../test/test_dynamic.csv')

for i in range(3):
    amplitude_list_index = [x[i*500:(i+1)*500 -1]for x in amplitude_list]
    amplitude_corr_list = data_corr(amplitude_list_index)
    d = eucli_distance(amplitude_static_corr_list, amplitude_corr_list)
    test_mat = np.mat(amplitude_corr_list)
    print(d.mean(),d.var(),np.std(
        rssi_list[i*500:(i+1)*500 -1]), test_mat.sum())
    # sns.distplot(amplitude_corr_list, hist=False,color=color_list[i], label=str(i))

# plt.legend()
# plt.show()
'''


# def get_data_uart(port="/dev/ttyUSB0", baud=921600, data_len_filter=384):
ser = serial.Serial("/dev/ttyUSB0", 921600, timeout=0.1)

data_len_filter = 384
amplitude_len = 0
amplitude_list = [[]for i in range(data_len_filter//2)]
rssi_list = []
index_offset = 0
time_list = []
distance_list = []

i = 0

while True:
    row = ser.readline().decode('utf-8').split(',')
    if len(row) < 26:
        continue

    if "CSI_DATA" not in row:
        continue

    # print('len(row)', len(row))

    data_len = int(row[23])
    if data_len != data_len_filter:
        continue

    data_list = (row[25].split('[ ')[1]).split(' ')
    get_amplitude(amplitude_list, data_list, data_len)
    amplitude_len += 1

    rssi_list.append(int(row[4]))

    print('aplitude_len', amplitude_len)

    if amplitude_len < 500:
        continue

    amplitude_list_index = [
        x[index_offset: (500 - 1) + index_offset]for x in amplitude_list]
    amplitude_corr_list = data_corr(amplitude_list_index)

    if(len(amplitude_corr_list) != (500-1)):
        print("len fail", len(amplitude_corr_list))
        index_offset += 500
        amplitude_len = 0
        continue

    d = eucli_distance(amplitude_static_corr_list, amplitude_corr_list)
    distance_list.append(d.mean())
    time_list.append(i)
    i += 1

    plt.plot(time_list, distance_list, '*-b')
    plt.draw()
    plt.pause(0.001)

    index_offset += 100
    amplitude_len -= 100

# amplitude_list = []
# ser = serial.Serial("/dev/ttyUSB0", 115200, timeout=0.5)

# plt.grid(True)  # 添加网格
# plt.ion()  # interactive mode
# plt.figure(1)
# plt.xlabel('times')
# plt.ylabel('data')
# plt.title('Diagram of UART data by Python')

# t = []
# m = []
# x = []
# i = 0

# while True:
#     row = ser.readline().decode('utf-8').split(',')
#     i = i+1
#     t.append(i)
#     m.append(int(row[3]))
#     x.append(int(row[4]))
#     plt.plot(t, m, '.-r')
#     plt.plot(t, x, '.-b')
#     plt.draw()
#     plt.pause(0.001)

#     continueecode byte 0x80
