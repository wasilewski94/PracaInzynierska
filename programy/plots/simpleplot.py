#!/usr/bin/python
# -*- coding: utf-8 -*-
import numpy as np
import matplotlib.pyplot as plt
from scipy.fftpack import fft
from scipy.signal import blackman
from numpy import hamming
import csv
import argparse

parser = argparse.ArgumentParser()
parser.add_argument("path", default='dane.txt')
args = parser.parse_args()

t=[]
dane=[]

with open(args.path, 'r') as csvfile:
	plots = csv.reader(csvfile, delimiter=',')
	for row in plots:
		try:
			t.append(int(row[0]))
			dane.append(float(row[1]))
		except:
			pass

#make x table start from zero	
t0 = t[0]
for i in range(len(t)):
	t[i] = (t[i]-t0)

#fig, axes = plt.subplots(2,1)


plt.xlabel('Czas [us]')
plt.ylabel(u'Wartość ciśnienia atmosferycznego [hPa]')
plt.title(u'Wykres zależności ciśnienia atmosferycznego od zmian wysokości w czasie')
plt.plot(t, dane, '-o', marker = '.')
plt.show()
