#!/usr/bin/env python3
# -*-coding:utf-8-*-
import sys
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from IPython import embed


select_list = []
# select_list += [i for i in range(6, 32)]
select_list += [i for i in range(33, 59)]
# select_list += [i for i in range(66, 123)]
# select_list += [i for i in range(123, 192)]

if __name__ == '__main__':
    file_name = sys.argv[1]

    df = pd.read_csv(file_name)

    df_rssi = df.loc[:, ['rssi']]
    df_rssi.plot(y=['rssi'])
    plt.axis([0, len(df_rssi.index), -100, 0])

    df_csi = df.loc[:, ['len', 'data']]
    size_x = len(df_csi.index)
    size_y = df_csi.iloc[0]['len'] // 2
    array_csi = np.zeros([size_x, size_y], dtype=np.complex64)
    for x, csi in enumerate(df_csi.iloc):
        data_list = csi['data'].split(' ')

        csi_raw_data = []
        for item in data_list:
            try:
                csi_raw_data.append(int(item))
            except ValueError:
                continue
        for y in range(0, len(csi_raw_data), 2):
            array_csi[x][y //
                         2] = complex(csi_raw_data[y], csi_raw_data[y + 1])

    array_csi_modulus = abs(array_csi)
    columns = [f"subcarrier{i}" for i in range(0, size_y)]
    df_csi_modulus = pd.DataFrame(array_csi_modulus, columns=columns)

    select_list.remove(57)
    df_csi_modulus.plot(y = [f"subcarrier{i}" for i in select_list])
    plt.show()
    # embed(colors='neutral')
